/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenCodeGenModule.h"
#include "EstimateFunctionSize.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Argument.h"
#include "llvm/IR/Attributes.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DIBuilder.h"
#include "common/LLVMWarningsPop.hpp"
#include "DebugInfo/VISADebugEmitter.hpp"
#include "llvmWrapper/Transforms/IPO/InlineHelper.h"
#include <numeric>
#include <utility>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

char GenXCodeGenModule::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(GenXCodeGenModule, "GenXCodeGenModule", "GenXCodeGenModule", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(GenXFunctionGroupAnalysis)
IGC_INITIALIZE_PASS_END(GenXCodeGenModule, "GenXCodeGenModule", "GenXCodeGenModule", false, false)

llvm::ModulePass *IGC::createGenXCodeGenModulePass() {
  initializeGenXCodeGenModulePass(*PassRegistry::getPassRegistry());
  return new GenXCodeGenModule;
}

GenXCodeGenModule::GenXCodeGenModule() : llvm::ModulePass(ID), FGA(nullptr), pMdUtils(nullptr), Modified(false) {
  initializeGenXCodeGenModulePass(*PassRegistry::getPassRegistry());
}

GenXCodeGenModule::~GenXCodeGenModule() {}

void GenXCodeGenModule::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.addRequired<GenXFunctionGroupAnalysis>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<llvm::CallGraphWrapperPass>();
}

// Update cloned function's metadata.
static inline void CloneFuncMetadata(IGCMD::MetaDataUtils *pM, llvm::Function *ClonedF, llvm::Function *F) {
  using namespace IGC::IGCMD;
  auto Info = pM->getFunctionsInfoItem(F);
  auto NewInfo = FunctionInfoMetaDataHandle(new FunctionInfoMetaData());

  // Copy function type info.
  if (Info->isTypeHasValue()) {
    NewInfo->setType(Info->getType());
  }

  // Copy explicit argument info, if any.
  unsigned i = 0;
  for (auto AI = Info->begin_ArgInfoList(), AE = Info->begin_ArgInfoList(); AI != AE; ++AI) {
    NewInfo->addArgInfoListItem(Info->getArgInfoListItem(i));
    i++;
  }

  // Copy implicit argument info, if any.
  i = 0;
  for (auto AI = Info->begin_ImplicitArgInfoList(), AE = Info->end_ImplicitArgInfoList(); AI != AE; ++AI) {
    NewInfo->addImplicitArgInfoListItem(Info->getImplicitArgInfoListItem(i));
    i++;
  }

  pM->setFunctionsInfoItem(ClonedF, Info);
}

static Function *cloneFunc(IGCMD::MetaDataUtils *pM, Function *F, string prefix = "", string postfix = "_GenXClone") {
  ValueToValueMapTy VMap;

  Function *ClonedFunc = CloneFunction(F, VMap);
  ClonedFunc->setName(prefix + F->getName().str() + postfix);
  // if the function is not added as part of clone, add it
  if (!F->getParent()->getFunction(ClonedFunc->getName()))
    F->getParent()->getFunctionList().push_back(ClonedFunc);
  CloneFuncMetadata(pM, ClonedFunc, F);

  return ClonedFunc;
}

inline Function *getCallerFunc(Value *user) {
  IGC_ASSERT(nullptr != user);
  Function *caller = nullptr;
  if (CallInst *CI = dyn_cast<CallInst>(user)) {
    IGC_ASSERT(nullptr != CI->getParent());
    caller = CI->getParent()->getParent();
  }
  IGC_ASSERT_MESSAGE(caller, "cannot be indirect call");
  return caller;
}

void GenXCodeGenModule::detectUnpromotableFunctions(Module *pM) {
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  SmallSet<Function *, 32> tempFuncs;

  // Find functions that have uses of "localSLM" globals
  for (auto gi = pM->global_begin(), ge = pM->global_end(); gi != ge; gi++) {
    GlobalVariable *GV = dyn_cast<GlobalVariable>(gi);
    if (GV && GV->hasSection() && GV->getSection().equals("localSLM")) {
      for (auto user : GV->users()) {
        if (Instruction *U = dyn_cast<Instruction>(user)) {
          Function *pF = U->getParent()->getParent();
          if (!isEntryFunc(pMdUtils, pF))
            tempFuncs.insert(pF);
        }
      }
    }
  }

  // Find functions that uses instructions that can't be handled for indirect call
  for (auto &F : *pM) {
    // Only look at non-entry functions
    if (F.isDeclaration() || isEntryFunc(pMdUtils, &F) || tempFuncs.count(&F) != 0)
      continue;

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (auto *GII = dyn_cast<GenIntrinsicInst>(&*I)) {
        // Can't make indirect if threadgroupbarrier intrinsic is set
        if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_threadgroupbarrier) {
          tempFuncs.insert(&F);
          break;
        }
      }
    }
  }

  // Recursively add callers, as the whole chain of calls cannot be promoted
  std::function<void(Function *)> AddCallerRecursive = [&](Function *F) {
    if (isEntryFunc(pMdUtils, F))
      return;

    m_UnpromotableFuncs.insert(F);
    for (auto user : F->users()) {
      if (Instruction *I = dyn_cast<Instruction>(user)) {
        Function *parentF = I->getParent()->getParent();
        AddCallerRecursive(parentF);
      }
    }
  };

  for (auto F : tempFuncs) {
    AddCallerRecursive(F);
  }
}

void GenXCodeGenModule::processFunction(Function &F) {
  // See what FunctionGroups this Function is called from.
  SetVector<std::pair<FunctionGroup *, Function *>> CallerFGs;

  std::vector<llvm::Function *> Callers;
  if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
    if (F.getName().equals("__stackoverflow_detection")) {
      // Mark all stack calls as users of this detection function.
      // It will be used as a subroutine, so it needs to be cloned for
      // each of stack call functions.
      for (auto &Func : F.getParent()->getFunctionList()) {
        if (Func.hasFnAttribute("visaStackCall")) {
          Callers.push_back(&Func);
        }
      }
    }
  }
  for (auto U : F.users()) {
    Callers.push_back(getCallerFunc(U));
  }

  for (auto Caller : Callers) {
    FunctionGroup *FG = FGA->getGroup(Caller);
    Function *SubGrpH = FGA->useStackCall(&F) ? (&F) : FGA->getSubGroupMap(Caller);
    if (FG == nullptr || SubGrpH == nullptr)
      continue;
    CallerFGs.insert(std::make_pair(FG, SubGrpH));
  }

  IGC_ASSERT(CallerFGs.size() >= 1);

  auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  auto CanMakeIndirectFunc = [&](llvm::Function *F) {
    // Don't convert subroutines, builtins, or invoke_simd_target
    if (F->hasFnAttribute("visaStackCall") == false || F->hasFnAttribute("OclBuiltin") ||
        F->hasFnAttribute("invoke_simd_target"))
      return false;

    // If SIMD Variant Compilation is not enabled, we have to make sure all callers
    // have the same SIMD sizes, otherwise we cannot make it an indirect call
    int simd_size = 0;
    unsigned CallersPerSIMD[3] = {0, 0, 0};
    for (const auto &iter : CallerFGs) {
      Function *callerKernel = iter.first->getHead();
      auto funcInfoMD = pMdUtils->getFunctionsInfoItem(callerKernel);
      int sz = funcInfoMD->getSubGroupSize()->getSIMDSize();
      if (sz != 0) {
        if (simd_size == 0)
          simd_size = sz;
        CallersPerSIMD[(sz >> 4)]++;
      }

      // Callers have varying SIMD size requirements, do not promote
      if (IGC_IS_FLAG_DISABLED(EnableSIMDVariantCompilation) && simd_size != sz)
        return false;
    }

    if (IGC_IS_FLAG_ENABLED(EnableSIMDVariantCompilation)) {
      // For MultiSIMD compile, check profitability.
      // Since we need to clone to each SIMD variant kernel, only promote if the
      // number of FGs per SIMD is greater than the threshold.
      if (m_FunctionCloningThreshold > 0 && CallersPerSIMD[0] <= m_FunctionCloningThreshold &&
          CallersPerSIMD[1] <= m_FunctionCloningThreshold && CallersPerSIMD[2] <= m_FunctionCloningThreshold) {
        return false;
      }
    }

    // If CallWA is needed, we should not convert to indirect call when requiring SIMD32,
    // as we can potentially avoid CallWA if there are no indirect calls.
    if (pCtx->platform.requireCallWA() /* && simd_size == 32*/) {
      return false;
    }

    // Detect if function is part of the unpromotable set
    if (m_UnpromotableFuncs.count(F) != 0) {
      return false;
    }

    return true;
  };

  // Make the function indirect if cloning is required to decrease compile time
  if (m_FunctionCloningThreshold > 0 && CallerFGs.size() > m_FunctionCloningThreshold && CanMakeIndirectFunc(&F)) {
    auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    auto IFG = FGA->getOrCreateIndirectCallGroup(F.getParent());
    if (IFG) {
      // If EnableSIMDVariantCompilation=0, we only compile a single variant of the dummy kernel.
      // By default, getOrCreateIndirectCallGroup will create a dummy kernel with the lowest allowed SIMD size,
      // but if only a single variant exists, and we determine that the caller has a required subgroup size
      // different from the default size, we need to change the default subgroup size here to match the caller.
      if (IGC_IS_FLAG_DISABLED(EnableSIMDVariantCompilation)) {
        int req_subgroup = 0;
        for (const auto &FG : CallerFGs) {
          auto FHead = FG.first->getHead();
          auto subGrpSz = pMdUtils->getFunctionsInfoItem(FHead)->getSubGroupSize();
          if (subGrpSz->isSIMDSizeHasValue()) {
            // We can assume all callers have the same subgroup size requirement, otherwise
            // CanMakeIndirectFunc will return false when EnableSIMDVariantCompilation=0
            req_subgroup = subGrpSz->getSIMDSize();
            break;
          }
        }
        if (req_subgroup) {
          auto defaultKernel = IFG->getHead();
          pMdUtils->getFunctionsInfoItem(defaultKernel)->getSubGroupSize()->setSIMDSize(req_subgroup);
          pMdUtils->save(pCtx->getModule()->getContext());
        }
      }
      if (IGC_IS_FLAG_ENABLED(PrintStackCallDebugInfo)) {
        dbgs() << "Make Indirect: " << F.getName().str() << "\n";
      }
      F.addFnAttr("referenced-indirectly");
      pCtx->m_enableFunctionPointer = true;
      FGA->addToFunctionGroup(&F, IFG, &F);
      return;
    }
  }

  bool FirstPair = true;
  for (const auto &FGPair : CallerFGs) {
    if (FirstPair) {
      FGA->addToFunctionGroup(&F, FGPair.first, FGPair.second);
      FirstPair = false;
    } else {
      // clone the function, add to this function group
      auto FCloned = cloneFunc(pMdUtils, &F);

      // Copy F's property over
      copyFuncProperties(FCloned, &F);

      Function *SubGrpH = FGA->useStackCall(&F) ? FCloned : FGPair.second;
      FGA->addToFunctionGroup(FCloned, FGPair.first, SubGrpH);
      Modified = true;
      // update the edge after clone, it can handle self-recursion
      for (auto UI = F.use_begin(), UE = F.use_end(); UI != UE; /*empty*/) {
        // Increment iterator after setting U to change the use.
        Use *U = &*UI++;
        Function *Caller = cast<CallInst>(U->getUser())->getParent()->getParent();
        FunctionGroup *InFG = FGA->getGroup(Caller);
        Function *InSubGrpH = FGA->useStackCall(&F) ? FCloned : FGA->getSubGroupMap(Caller);
        if (InFG == FGPair.first && InSubGrpH == SubGrpH) {
          *U = FCloned;
        }
      }
    }
  }
}

void GenXCodeGenModule::processSCC(std::vector<llvm::CallGraphNode *> *SCCNodes) {
  // force stack-call for every function in SCC
  for (CallGraphNode *Node : (*SCCNodes)) {
    Function *F = Node->getFunction();
    F->addFnAttr("visaStackCall");
  }

  // See what FunctionGroups this SCC is called from.
  SetVector<FunctionGroup *> CallerFGs;
  for (CallGraphNode *Node : (*SCCNodes)) {
    Function *F = Node->getFunction();
    for (auto U : F->users()) {
      Function *Caller = getCallerFunc(U);
      FunctionGroup *FG = FGA->getGroup(Caller);
      if (FG == nullptr)
        continue;
      CallerFGs.insert(FG);
    }
  }
  IGC_ASSERT(CallerFGs.size() >= 1);

  bool FirstPair = true;
  for (auto FG : CallerFGs) {
    if (FirstPair) {
      for (CallGraphNode *Node : (*SCCNodes)) {
        Function *F = Node->getFunction();
        FGA->addToFunctionGroup(F, FG, F);
      }
      FirstPair = false;
    } else {
      // clone the functions in scc, add to this function group
      llvm::DenseMap<Function *, Function *> CloneMap;
      for (CallGraphNode *Node : (*SCCNodes)) {
        Function *F = Node->getFunction();
        auto FCloned = cloneFunc(pMdUtils, F);

        // Copy properties
        copyFuncProperties(FCloned, F);

        FGA->addToFunctionGroup(FCloned, FG, FCloned);
        CloneMap.insert(std::make_pair(F, FCloned));
      }
      Modified = true;
      // update the call-edges for every function in SCC,
      // move edges to the cloned SCC, including the recursion edge
      for (CallGraphNode *Node : (*SCCNodes)) {
        Function *F = Node->getFunction();
        for (auto UI = F->use_begin(), UE = F->use_end(); UI != UE; /*empty*/) {
          // Increment iterator after setting U to change the use.
          Use *U = &*UI++;
          Function *Caller = cast<CallInst>(U->getUser())->getParent()->getParent();
          FunctionGroup *InFG = FGA->getGroup(Caller);
          if (InFG == FG) {
            *U = CloneMap[F];
          }
        }
      }
    }
  }
}

void GenXCodeGenModule::setFuncProperties(CallGraph &CG) {
  for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I) {
    const std::vector<CallGraphNode *> &SCCNodes = (*I);
    for (CallGraphNode *Node : SCCNodes) {
      Function *F = Node->getFunction();
      if (F != nullptr && !F->isDeclaration()) {
        if (Node->empty()) {
          FGA->setLeafFunc(F);
        }
      }
    }
  }
}

void GenXCodeGenModule::copyFuncProperties(llvm::Function *To, llvm::Function *From) {
  FGA->copyFuncProperties(To, From);
}

/// Deduce non-null argument attributes for subroutines.
static bool DeduceNonNullAttribute(Module &M) {
  bool Modifided = false;
  for (auto &F : M.getFunctionList()) {
    if (F.isDeclaration() || F.hasExternalLinkage() || F.hasAddressTaken())
      continue;

    bool Skip = false;
    for (auto U : F.users()) {
      if (!isa<CallInst>(U)) {
        Skip = true;
        break;
      }
    }
    if (Skip)
      continue;

    for (auto &Arg : F.args()) {
      // Only for used pointer arguments.
      if (Arg.use_empty() || !Arg.getType()->isPointerTy())
        continue;

      // If all call sites are passing a non-null value to this argument, then
      // this argument cannot be a null ptr.
      bool NotNull = true;
      for (auto U : F.users()) {
        auto CI = cast<CallInst>(U);
        Value *V = CI->getArgOperand(Arg.getArgNo());
        auto DL = F.getParent()->getDataLayout();
        if (!isKnownNonZero(V, DL)) {
          NotNull = false;
          break;
        }
      }

      if (NotNull) {
        Arg.addAttr(llvm::Attribute::NonNull);
        Modifided = true;
      }
    }
  }
  return Modifided;
}

bool GenXCodeGenModule::runOnModule(Module &M) {
  FGA = &getAnalysis<GenXFunctionGroupAnalysis>();

  // Already built.
  if (FGA->getModule())
    return false;

  // Set the cloning threshold. This determins the number of times a function called from multiple
  // function groups can be cloned. If the number exceeds the threshold, instead of cloning the
  // function N times, make it an indirect call and use relocation instead. The function will only be
  // compiled once and runtime must relocate its address for each caller.
  m_FunctionCloningThreshold = 0;
  if (IGC_IS_FLAG_ENABLED(EnableFunctionCloningControl)) {
    if (IGC_GET_FLAG_VALUE(FunctionCloningThreshold) != 0) {
      // Overwrite with debug flag
      m_FunctionCloningThreshold = IGC_GET_FLAG_VALUE(FunctionCloningThreshold);
    } else {
      // Avoid cloning by default on zebin
      m_FunctionCloningThreshold = 1;
    }
  }

  pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

  setFuncProperties(CG);

  std::vector<std::vector<CallGraphNode *> *> SCCVec;
  for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I) {
    std::vector<CallGraphNode *> *SCCNodes = new std::vector<CallGraphNode *>((*I));
    SCCVec.push_back(SCCNodes);
  }

  // Add all indirect functions to the default kernel group
  FGA->addIndirectFuncsToKernelGroup(&M);

  // If FunctionCloningThreshold is enabled, detect functions that cannot be promoted to indirect
  if (m_FunctionCloningThreshold > 0) {
    detectUnpromotableFunctions(&M);
  }

  for (auto I = SCCVec.rbegin(), IE = SCCVec.rend(); I != IE; ++I) {
    std::vector<CallGraphNode *> *SCCNodes = (*I);
    for (CallGraphNode *Node : (*SCCNodes)) {
      Function *F = Node->getFunction();
      if (!F || F->isDeclaration())
        continue;
      // skip functions belonging to the indirect call group
      if (FGA->isIndirectCallGroup(F))
        continue;

      if (isEntryFunc(pMdUtils, F)) {
        FGA->setSubGroupMap(F, F);
        FGA->createFunctionGroup(F);
      } else if (SCCNodes->size() == 1) {
        processFunction(*F);
      } else {
        processSCC(SCCNodes);
        break;
      }
    }
    delete SCCNodes;
  }

  // Clone indirect funcs if SIMD variants are required
  FGA->CloneFunctionGroupForMultiSIMDCompile(&M);

  this->pMdUtils->save(M.getContext());

  // Check and set FG attribute flags
  FGA->setGroupAttributes();

  // By swapping, we sort the function list to ensure codegen order for
  // functions. This relies on llvm module pass manager's implementation detail.
  SmallVector<Function *, 16> OrderedList;
  for (auto GI = FGA->begin(), GE = FGA->end(); GI != GE; ++GI) {
    for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end(); SubGI != SubGE; ++SubGI) {
      for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI) {
        OrderedList.push_back(*FI);
      }
    }
  }

  //  Input L1 = [A, B, C, D, E]       // Functions in groups
  //  Input L2 = [A, C, G, B, D, E, F] // Functions in the module
  // Output L2 = [A, B, C, D, E, G, F] // Ordered functions in the module
  IGC_ASSERT_MESSAGE(OrderedList.size() <= M.size(), "out of sync");
  Function *CurF = &(*M.begin());
  for (auto I = OrderedList.begin(), E = OrderedList.end(); I != E; ++I) {
    IGC_ASSERT(nullptr != CurF);
    Function *F = *I;
    if (CurF != F)
      // Move F before CurF. This just works See BasicBlock::moveBefore.
      // CurF remains the same for the next iteration.
      M.getFunctionList().splice(CurF->getIterator(), M.getFunctionList(), F);
    else {
      auto it = CurF->getIterator();
      CurF = &*(++it);
    }
  }

  IGC_ASSERT(FGA->verify());

  FGA->setModule(&M);
  Modified |= DeduceNonNullAttribute(M);

  return Modified;
}

////////////////////////////////////////////////////////////////////////////////
/// GenXFunctionGroupAnalysis implementation detail
////////////////////////////////////////////////////////////////////////////////

char GenXFunctionGroupAnalysis::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(GenXFunctionGroupAnalysis, "GenXFunctionGroupAnalysis", "GenXFunctionGroupAnalysis", false,
                          true)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(GenXFunctionGroupAnalysis, "GenXFunctionGroupAnalysis", "GenXFunctionGroupAnalysis", false,
                        true)

llvm::ImmutablePass *IGC::createGenXFunctionGroupAnalysisPass() {
  initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());
  return new GenXFunctionGroupAnalysis;
}

GenXFunctionGroupAnalysis::GenXFunctionGroupAnalysis() : ImmutablePass(ID), M(nullptr) {
  initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());
}

void GenXFunctionGroupAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<MetaDataUtilsWrapper>();
  AU.setPreservesAll();
}

bool GenXFunctionGroupAnalysis::verify() {
  for (auto GI = begin(), GE = end(); GI != GE; ++GI) {
    for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end(); SubGI != SubGE; ++SubGI) {
      for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI) {
        Function *F = *FI;
        if (F->hasFnAttribute("referenced-indirectly")
        ) {
          continue;
        }
        // If F is an unused non-kernel function, although it should have been
        // deleted, that is fine.
        for (auto U : F->users()) {
          Function *Caller = getCallerFunc(U);
          FunctionGroup *CallerFG = getGroup(Caller);
          // Caller's FG should be the same as FG. Otherwise, something is wrong.
          if (CallerFG != (*GI)) {
            printf("%s\n", F->getName().data());
            printf("%s\n", Caller->getName().data());
            return false;
          }
          Function *SubGrpH = getSubGroupMap(F);
          // Caller's sub-group header must be the first element of the sub-vector
          if (SubGrpH != (*SubGI)->front())
            return false;
        }
      }
    }
  }
  // Everything is OK.
  return true;
}

// Returns the default SIMD size to compile Indirect Functions to
static inline int getDefaultSIMDSize(CodeGenContext *ctx) {
  if (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 0) {
    // Forcing SIMD size
    return ctx->getModuleMetaData()->csInfo.forcedSIMDSize;
  }

  // By default, use the minimum allowed SIMD for the HW platform
  SIMDMode simdMode = ctx->platform.getMinDispatchMode();
  int defaultSz = numLanes(simdMode);

  if (ctx->getModuleMetaData()->compOpt.IsLibraryCompilation) {
    unsigned sz = ctx->getModuleMetaData()->compOpt.LibraryCompileSIMDSize;
    if (sz > 0) {
      IGC_ASSERT(sz == 8 || sz == 16 || sz == 32);
      defaultSz = sz;
    }
  }
  return defaultSz;
}

FunctionGroup *GenXFunctionGroupAnalysis::getOrCreateIndirectCallGroup(Module *pModule, int SimdSize) {
  CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto pMdUtil = ctx->getMetaDataUtils();
  auto pModMD = ctx->getModuleMetaData();
  int defaultSimd = getDefaultSIMDSize(ctx);

  if (SimdSize == 0) {
    SimdSize = defaultSimd;
  }

  IGC_ASSERT(SimdSize == 8 || SimdSize == 16 || SimdSize == 32);
  int idx = SimdSize >> 4;

  if (IndirectCallGroup[idx] != nullptr)
    return IndirectCallGroup[idx];

  llvm::Function *defaultKernel = IGC::getIntelSymbolTableVoidProgram(pModule);

  // No default kernel found
  if (!defaultKernel)
    return nullptr;

  if (SimdSize != defaultSimd) {
    // Require a SIMD variant version of the dummy kernel.
    // Create a new dummy kernel clone here to attach all functions with the required SimdSize.
    std::string fName = std::string(IGC::INTEL_SYMBOL_TABLE_VOID_PROGRAM) + "_GenXSIMD" + std::to_string(SimdSize);
    Function *pNewFunc =
        Function::Create(defaultKernel->getFunctionType(), GlobalValue::ExternalLinkage, fName, pModule);
    BasicBlock *entry = BasicBlock::Create(pModule->getContext(), "entry", pNewFunc);
    IRBuilder<> builder(entry);
    builder.CreateRetVoid();

    // Set spirv calling convention and kernel metadata
    pNewFunc->setCallingConv(llvm::CallingConv::SPIR_KERNEL);
    IGCMD::FunctionInfoMetaDataHandle fHandle = IGCMD::FunctionInfoMetaDataHandle(new IGCMD::FunctionInfoMetaData());
    FunctionMetaData *funcMD = &pModMD->FuncMD[pNewFunc];
    funcMD->functionType = IGC::FunctionTypeMD::KernelFunction;
    fHandle->setType(FunctionTypeMD::KernelFunction);
    pMdUtil->setFunctionsInfoItem(pNewFunc, fHandle);
    defaultKernel = pNewFunc;
  }
  // Set the requested sub_group_size value for this kernel
  pMdUtil->getFunctionsInfoItem(defaultKernel)->getSubGroupSize()->setSIMDSize(SimdSize);
  pMdUtil->save(pModule->getContext());

  auto FG = getGroup(defaultKernel);
  if (!FG) {
    setSubGroupMap(defaultKernel, defaultKernel);
    FG = createFunctionGroup(defaultKernel);
  }
  IndirectCallGroup[idx] = FG;
  return FG;
}

bool GenXFunctionGroupAnalysis::useStackCall(llvm::Function *F) { return (F->hasFnAttribute("visaStackCall")); }

void GenXFunctionGroupAnalysis::setGroupAttributes() {
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  for (auto FG : Groups) {
    if (isIndirectCallGroup(FG)) {
      // The dummy kernel group is not a true function group, in that the functions in this group does not have
      // a valid callgraph that connects them. It's a dummy group where all indirectly called functions are contained.
      // Therefore, the group attributes are not valid here, since they are not connected to the real groupHead, which
      // is the caller kernel. We don't set any of the FG attribute flags for this group.
      //
      // Note, indirect functions in this group can still directly call stackcalls or subroutines, which may also belong
      // to this group due to cloning. However we still can't associate all functions in this group with a single
      // callgraph.
      continue;
    }

    for (const Function *F : *FG) {
      if (F->hasFnAttribute("referenced-indirectly")) {
        IGC_ASSERT_MESSAGE(0, "Indirectly referenced function not moved to IndirectCallGroup!");
        continue;
      }

      if (F->hasFnAttribute("visaStackCall")) {
        FG->m_hasStackCall = true;
        if (!isLeafFunc(F)) {
          FG->m_hasNestedCall = true;
        }
      } else if (!isEntryFunc(pMdUtils, F)) {
        FG->m_hasSubroutine = true;
      }

      // check all functions in the group to see if there's an vla alloca
      // function attribute "hasVLA" should be set at ProcessFuncAttributes pass
      if (F->hasFnAttribute("hasVLA")) {
        FG->m_hasVariableLengthAlloca = true;
      }

      // check if FG uses recursion. The "hasRecursion" attribute is set in
      // ProcessFuncAttributes pass by using Tarjan's algorithm to find recursion.
      if (F->hasFnAttribute("hasRecursion")) {
        FG->m_hasRecursion = true;
      }

      // For the remaining attributes we need to loop through all the call instructions
      for (auto ii = inst_begin(F), ei = inst_end(F); ii != ei; ii++) {
        if (const CallInst *call = dyn_cast<CallInst>(&*ii)) {
          Function *calledF = call->getCalledFunction();
          bool hasStackCall = calledF && calledF->hasFnAttribute("visaStackCall");
          if (call->isInlineAsm()) {
            // Uses inline asm call
            FG->m_hasInlineAsm = true;
          } else if (!calledF || (calledF->isDeclaration() && calledF->hasFnAttribute("referenced-indirectly"))) {
            // This is the true indirect call case, where either the callee's address is taken, or it belongs
            // to an external module. We do not know the callgraph in this case.
            hasStackCall = !call->doesNotReturn();
            FG->m_hasIndirectCall = true;
            FG->m_hasPartialCallGraph = true;
          } else if (calledF && calledF->hasFnAttribute("referenced-indirectly")) {
            // This is the case where the callee has the "referenced-indirectly" attribute, but we still
            // see the callgraph. The callee may not belong to the same FG as the caller, but it's CG is still
            // available.
            hasStackCall = true;
            FG->m_hasIndirectCall = true;
          } else if (calledF && calledF->isDeclaration() && calledF->hasFnAttribute("invoke_simd_target")) {
            // Invoke_simd targets use stack call by convention.
            // Calling a func decl indicates unknown CG
            hasStackCall = true;
            FG->m_hasPartialCallGraph = true;
          }

          FG->m_hasStackCall |= hasStackCall;
          FG->m_hasNestedCall |= (!isEntryFunc(pMdUtils, F) && hasStackCall);
        }
      }
    }
  }
}

void GenXFunctionGroupAnalysis::addIndirectFuncsToKernelGroup(llvm::Module *pModule) {
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  // Find all indirectly called functions that require a symbol
  SmallVector<Function *, 16> indirectFunctions;
  for (auto I = pModule->begin(), E = pModule->end(); I != E; ++I) {
    Function *F = &(*I);
    if (F->isDeclaration() || isEntryFunc(pMdUtils, F))
      continue;

    if (F->hasFnAttribute("referenced-indirectly") || F->hasFnAttribute("variant-function-def")) {
      IGC_ASSERT(getGroup(F) == nullptr);
      indirectFunctions.push_back(F);
    }
  }
  // Add them to the indirect call group
  if (!indirectFunctions.empty()) {
    FunctionGroup *IFG = nullptr;
    for (auto F : indirectFunctions) {
      if (F->hasFnAttribute("variant-function-def")) {
        // If function variant is already created (meaning this is a rebuild), add them to the correct SIMD group
        auto [symStr, fName, vecLen] = IGC::ParseVectorVariantFunctionString(F->getName().str());
        IFG = getOrCreateIndirectCallGroup(pModule, vecLen);
      } else {
        IFG = getOrCreateIndirectCallGroup(pModule);
      }
      IGC_ASSERT(IFG);
      addToFunctionGroup(F, IFG, F);
    }
  }
}

void GenXFunctionGroupAnalysis::CloneFunctionGroupForMultiSIMDCompile(llvm::Module *pModule) {
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  // Don't clone if forcing SIMD size
  if (pCtx->getModuleMetaData()->csInfo.forcedSIMDSize != 0)
    return;

  int default_size = getDefaultSIMDSize(pCtx);

  // All "referenced-indirectly" funcs should already be added to the default group
  FunctionGroup *ICG = getOrCreateIndirectCallGroup(pModule);
  if (!ICG || ICG->isSingle())
    return;

  // For the first iteration, clone only functions called from other function groups
  // that have multiple SIMD size requirements. This is to get a baseline of how many variant
  // dummy kernels are required.
  for (Function *F : *ICG) {
    // Check if there are multiple callers with variant SIMD size requirements.
    int hasReqdSIMD = 0;
    if (!F->hasFnAttribute("referenced-indirectly"))
      continue;

    // Assume that if a FG uses the address of a function in the IndirectCallGroup,
    // then we should compile the IndirectCallGroup with the subgroup size of the user FG.
    DenseMap<Instruction *, int> UsersMap;
    for (auto U : F->users()) {
      if (Instruction *inst = dyn_cast<Instruction>(U)) {
        Function *callerF = inst->getParent()->getParent();
        if (!isIndirectCallGroup(callerF)) {
          auto FG = getGroup(callerF);
          auto funcInfoMD = pMdUtils->getFunctionsInfoItem(FG->getHead());
          int sz = funcInfoMD->getSubGroupSize()->getSIMDSize();
          if (sz != 0) {
            IGC_ASSERT(sz == 8 || sz == 16 || sz == 32);
            hasReqdSIMD |= sz;
            UsersMap[inst] = sz;
          }
        }
      }
    }

    if (hasReqdSIMD > 0) {
      bool ReqMultipleSIMD = hasReqdSIMD != 8 && hasReqdSIMD != 16 && hasReqdSIMD != 32;
      if (IGC_IS_FLAG_ENABLED(EnableSIMDVariantCompilation) && ReqMultipleSIMD) {
        for (int i = 0; i < 3; i++) {
          int simdsz = 1 << (i + 3);
          if ((hasReqdSIMD & simdsz) && simdsz != default_size) {
            // Clone the function
            // Use the function vector variant syntax for mangling func name
            string prefix = "_ZGVxN" + std::to_string(simdsz) + "_";
            Function *FCloned = cloneFunc(pMdUtils, F, prefix, "");
            copyFuncProperties(FCloned, F);
            FCloned->addFnAttr("variant-function-def");
            auto pNewICG = getOrCreateIndirectCallGroup(pModule, simdsz);
            addToFunctionGroup(FCloned, pNewICG, FCloned);

            for (const auto &iter : UsersMap) {
              if (iter.second == simdsz) {
                iter.first->replaceUsesOfWith(F, FCloned);
              }
            }
          }
        }
      } else {
        IGC_ASSERT_MESSAGE(!ReqMultipleSIMD, "SIMD variant compilation not supported");
        continue;
      }
    }
  }

  auto hasIndirectCaller = [this](Function *F, FunctionGroup *ICG) {
    for (auto U : F->users()) {
      if (Instruction *I = dyn_cast<Instruction>(U)) {
        Function *callerF = I->getParent()->getParent();
        if (getGroup(callerF) == ICG)
          return true;
      }
    }
    return false;
  };

  // In second iteration, clone functions that have callers inside the indirect call group.
  // These functions may have been missed in the first iteration if they are not used outside
  // the indirect call group, but need to be cloned to each variant dummy kernel we create.
  for (Function *F : *ICG) {
    if (F == ICG->getHead() || !hasIndirectCaller(F, ICG))
      continue;

    for (int i = 0; i < 3; i++) {
      int simdsz = 1 << (i + 3);
      auto ICG = IndirectCallGroup[i];
      if (ICG && simdsz != default_size) {
        string prefix = "_ZGVxN" + std::to_string(simdsz) + "_";

        // If mangled function already exist, don't re-clone it
        if (pModule->getFunction(prefix + F->getName().str()) != nullptr)
          continue;

        Function *FCloned = cloneFunc(pMdUtils, F, prefix, "");
        copyFuncProperties(FCloned, F);
        FCloned->addFnAttr("variant-function-def");

        Function *SubGrpH = useStackCall(F) ? FCloned : ICG->getHead();
        addToFunctionGroup(FCloned, ICG, SubGrpH);

        // update the edge after clone
        for (auto UI = F->use_begin(), UE = F->use_end(); UI != UE; /*empty*/) {
          // Increment iterator after setting U to change the use.
          Use *U = &*UI++;
          Function *Caller = cast<CallInst>(U->getUser())->getParent()->getParent();
          FunctionGroup *InFG = getGroup(Caller);
          Function *InSubGrpH = useStackCall(F) ? FCloned : getSubGroupMap(Caller);
          if (InFG == ICG && InSubGrpH == SubGrpH) {
            *U = FCloned;
          }
        }
      }
    }
  }
}

bool GenXFunctionGroupAnalysis::rebuild(llvm::Module *Mod) {
  clear();
  auto pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

  // Re-add all indirect functions to the default kernel group
  addIndirectFuncsToKernelGroup(Mod);

  // Build and verify function list layout.
  // Given a list of functions, [K1, A, B, K2, C, K3, D, E, F], we build groups
  // [K1, A, B], [K2, C], [K3, D, E, F] and verify that none of CallInst escapes
  // from its group. It is rather cheap to build and verify when there is no
  // subroutine in this module.
  //
  FunctionGroup *CurFG = nullptr;
  Function *CurSubGrpH = nullptr;
  for (auto I = Mod->begin(), E = Mod->end(); I != E; ++I) {
    Function *F = &(*I);

    // Skip declarations.
    if (F->empty())
      continue;
    // Skip functions belonging to the indirect call group
    if (isIndirectCallGroup(F))
      continue;

    if (isEntryFunc(pMdUtils, F)) {
      CurFG = createFunctionGroup(F);
      CurSubGrpH = F;
    } else {
      if (useStackCall(F))
        CurSubGrpH = F;
      if (CurFG && CurSubGrpH)
        addToFunctionGroup(F, CurFG, CurSubGrpH);
      else {
        clear();
        return false;
      }
    }
  }

  // Reset attribute flags
  setGroupAttributes();

  // Verification.
  if (!verify()) {
    clear();
    return false;
  }

  // Commit.
  M = Mod;
  return true;
}

void GenXFunctionGroupAnalysis::replaceEntryFunc(Function *OF, Function *NF) {
  auto groupMapIter = GroupMap.find(OF);
  FunctionGroup *FG = groupMapIter->second;
  GroupMap.erase(groupMapIter);
  GroupMap.insert(std::make_pair(NF, FG));

  FG->replaceGroupHead(OF, NF);

  // For Entry func, SubGroupMap needs to be updated as well
  auto SGIter = SubGroupMap.find(OF);
  if (SGIter != SubGroupMap.end()) {
    SubGroupMap.erase(SGIter);
    SubGroupMap.insert(std::make_pair(NF, NF));
  }
  DenseMap<const Function *, Function *>::iterator SGII, SGIE;
  for (SGII = SubGroupMap.begin(), SGIE = SubGroupMap.end(); SGII != SGIE; ++SGII) {
    Function *SGH = SGII->second;
    if (SGH == OF) {
      SGII->second = NF;
    }
  }
}

void GenXFunctionGroupAnalysis::clear() {
  GroupMap.clear();
  SubGroupMap.clear();
  for (auto I = begin(), E = end(); I != E; ++I)
    delete *I;
  Groups.clear();
  for (auto FG : IndirectCallGroup)
    FG = nullptr;
  M = nullptr;
}

FunctionGroup *GenXFunctionGroupAnalysis::getGroup(const Function *F) {
  auto I = GroupMap.find(F);
  if (I == GroupMap.end())
    return nullptr;
  return I->second;
}

FunctionGroup *GenXFunctionGroupAnalysis::getGroupForHead(const Function *F) {
  auto FG = getGroup(F);
  if (FG->getHead() == F)
    return FG;
  return nullptr;
}

void GenXFunctionGroupAnalysis::addToFunctionGroup(Function *F, FunctionGroup *FG, Function *SubGrpH) {
  IGC_ASSERT_MESSAGE(!GroupMap[F], "Function already attached to FunctionGroup");
  GroupMap[F] = FG;
  setSubGroupMap(F, SubGrpH);
  if (F == SubGrpH) {
    auto *SubGrp = new llvm::SmallVector<llvm::AssertingVH<llvm::Function>, 8>();
    SubGrp->push_back(F);
    FG->Functions.push_back(SubGrp);
  } else {
    for (auto I = FG->Functions.begin(), E = FG->Functions.end(); I != E; I++) {
      auto *SubGrp = (*I);
      IGC_ASSERT(nullptr != SubGrp);
      if (SubGrp->front() == SubGrpH) {
        SubGrp->push_back(F);
        return;
      }
    }
    IGC_ASSERT(0);
  }
}

FunctionGroup *GenXFunctionGroupAnalysis::createFunctionGroup(Function *F) {
  auto FG = new FunctionGroup;
  Groups.push_back(FG);
  addToFunctionGroup(F, FG, F);
  return FG;
}

void GenXFunctionGroupAnalysis::print(raw_ostream &os) {
  if (!M) {
    os << "(nil)\n";
    return;
  }

  unsigned TotalSize = 0;
  for (auto GI = begin(), GE = end(); GI != GE; ++GI) {
    for (auto SubGI = (*GI)->Functions.begin(), SubGE = (*GI)->Functions.end(); SubGI != SubGE; ++SubGI) {
      for (auto FI = (*SubGI)->begin(), FE = (*SubGI)->end(); FI != FE; ++FI) {
        Function *F = *FI;
        unsigned Size = std::accumulate(F->begin(), F->end(), 0,
                                        [](unsigned s, BasicBlock &BB) { return (unsigned)BB.size() + s; });
        TotalSize += Size;
        if (F == (*GI)->getHead())
          os << "K: " << F->getName() << " [" << Size << "]\n";
        else if (F == (*SubGI)->front())
          os << "  F: " << F->getName() << " [" << Size << "]\n";
        else
          os << "     " << F->getName() << " [" << Size << "]\n";
      }
    }
  }
  os << "Module: " << M->getModuleIdentifier() << " [" << TotalSize << "]\n";
}

#if defined(_DEBUG)
void GenXFunctionGroupAnalysis::dump() { print(llvm::errs()); }
#endif

using InlinedArrayAllocasTy = DenseMap<ArrayType *, std::vector<AllocaInst *>>;
namespace {

#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
AAResults createLegacyPMAAResults(Pass &P, Function &F, BasicAAResult &BAR) {
  AAResults AAR(P.getAnalysis<TargetLibraryInfoWrapperPass>().getTLI());
  AAR.addAAResult(BAR);

  // Populate the results with the other currently available AAs.
  if (auto *WrapperPass = P.getAnalysisIfAvailable<ScopedNoAliasAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<TypeBasedAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<GlobalsAAWrapperPass>())
    AAR.addAAResult(WrapperPass->getResult());
  if (auto *WrapperPass = P.getAnalysisIfAvailable<ExternalAAWrapperPass>())
    if (WrapperPass->CB)
      WrapperPass->CB(P, F, AAR);

  return AAR;
}

BasicAAResult createLegacyPMBasicAAResult(Pass &P, Function &F) {
  return BasicAAResult(F.getParent()->getDataLayout(), F, P.getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(),
                       P.getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F));
}

class LegacyAARGetter {
  Pass &P;
  std::optional<BasicAAResult> BAR;
  std::optional<AAResults> AAR;

public:
  LegacyAARGetter(Pass &P) : P(P) {}
  AAResults &operator()(Function &F) {
    BAR.emplace(createLegacyPMBasicAAResult(P, F));
    AAR.emplace(createLegacyPMAAResults(P, F, *BAR));
    return *AAR;
  }
};
#endif

/// \brief Custom inliner for subroutines.
#if LLVM_VERSION_MAJOR >= 16
class SubroutineInliner : public llvm::CallGraphSCCPass, public llvm::InstVisitor<SubroutineInliner> {

protected:
  llvm::AssumptionCacheTracker *ACT = nullptr;
  llvm::ProfileSummaryInfo *PSI = nullptr;
  std::function<const TargetLibraryInfo &(Function &)> GetTLI{};
  bool InsertLifetime = true;
  llvm::ImportedFunctionsInliningStatistics ImportedFunctionsStats{};
#else
class SubroutineInliner : public LegacyInlinerBase, public llvm::InstVisitor<SubroutineInliner> {
#endif
  EstimateFunctionSize *FSA = nullptr;
  MetaDataUtilsWrapper *MDUW = nullptr;

public:
  static char ID; // Pass identification, replacement for typeid

  // Use extremely low threshold.
#if LLVM_VERSION_MAJOR >= 16
  SubroutineInliner();
  InlineCost getInlineCost(IGCLLVM::CallSiteRef CS);
  bool inlineCalls(CallGraphSCC &SCC);

#else
  SubroutineInliner() : LegacyInlinerBase(ID, /*InsertLifetime*/ false), FSA(nullptr) {}
  InlineCost getInlineCost(IGCLLVM::CallSiteRef CS) override;
#endif
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnSCC(CallGraphSCC &SCC) override;
  void verifyAddrSpaceMismatch(CallGraphSCC &SCC);
  void visitGetElementPtrInst(GetElementPtrInst &I);
  void visitMemCpyInst(MemCpyInst &I);

  using llvm::Pass::doFinalization;
  bool doFinalization(CallGraph &CG) override {
#if LLVM_VERSION_MAJOR >= 16
    bool Changed = IGCLLVM::removeDeadFunctions(CG);
#else
    bool Changed = LegacyInlinerBase::removeDeadFunctions(CG);
#endif
    Changed |= purgeMetaDataUtils(CG.getModule(), MDUW);
    return Changed;
  }
};

} // namespace

IGC_INITIALIZE_PASS_BEGIN(SubroutineInliner, "SubroutineInliner", "SubroutineInliner", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(EstimateFunctionSize)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(SubroutineInliner, "SubroutineInliner", "SubroutineInliner", false, false)

char SubroutineInliner::ID = 0;

void SubroutineInliner::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<EstimateFunctionSize>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<MetaDataUtilsWrapper>();
#if LLVM_VERSION_MAJOR >= 16
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addUsedIfAvailable<ScopedNoAliasAAWrapperPass>();
  AU.addUsedIfAvailable<TypeBasedAAWrapperPass>();
  AU.addUsedIfAvailable<GlobalsAAWrapperPass>();
  AU.addUsedIfAvailable<ExternalAAWrapperPass>();
  CallGraphSCCPass::getAnalysisUsage(AU);
#else
  LegacyInlinerBase::getAnalysisUsage(AU);
#endif
}

#if LLVM_VERSION_MAJOR >= 16
SubroutineInliner::SubroutineInliner() : CallGraphSCCPass(ID), FSA(nullptr) {}

bool SubroutineInliner::inlineCalls(CallGraphSCC &SCC) {
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  ACT = &getAnalysis<AssumptionCacheTracker>();
  PSI = &getAnalysis<ProfileSummaryInfoWrapperPass>().getPSI();
  GetTLI = [&](Function &F) -> const TargetLibraryInfo & {
    return getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  };
  auto GetAssumptionCache = [&](Function &F) -> AssumptionCache & { return ACT->getAssumptionCache(F); };
  return IGCLLVM::inlineCallsImpl(
      SCC, CG, GetAssumptionCache, PSI, GetTLI, InsertLifetime, [&](CallBase &CB) { return getInlineCost(CB); },
      LegacyAARGetter(*this), ImportedFunctionsStats);
}
#endif

void SubroutineInliner::visitGetElementPtrInst(GetElementPtrInst &GEPI) {
  for (auto *useOfGEPI : GEPI.users()) {
    if (LoadInst *loadInst = dyn_cast<LoadInst>(useOfGEPI)) {
      auto GepReturnValueAS = GEPI.getPointerAddressSpace();
      auto LoadOperandAS = loadInst->getPointerAddressSpace();
      if (GepReturnValueAS != LoadOperandAS) {
        auto *GEPIPointerOperand = GEPI.getPointerOperand();
        SmallVector<llvm::Value *, 8> Idx(GEPI.idx_begin(), GEPI.idx_end());
        // we need to create a new GEPI because the old one has coded old AS,
        // and we can not create new load instruction with the old GEPI with the correct AS
        // This is WA for a bug in LLVM 11.
        GetElementPtrInst *newGEPI =
            GetElementPtrInst::Create(GEPI.getSourceElementType(), GEPIPointerOperand, Idx, "", &GEPI);
        newGEPI->setIsInBounds(GEPI.isInBounds());
        newGEPI->setDebugLoc(GEPI.getDebugLoc());

        auto *newLoad = new LoadInst(loadInst->getType(), newGEPI, "", loadInst);
        newLoad->setAlignment(IGCLLVM::getAlign(*loadInst));
        loadInst->replaceAllUsesWith(newLoad);
        newLoad->setDebugLoc(loadInst->getDebugLoc());
      }
    }
  }
}

void SubroutineInliner::visitMemCpyInst(MemCpyInst &I) {
  Value *Src = I.getRawSource();
  Value *Dst = I.getRawDest();
  Value *origSrc = I.getSource();
  Value *origDst = I.getDest();
  // Copying from alloca to alloca, but has addrspace mismatch due to incorrect bitcast
  if (isa<AllocaInst>(origSrc) && isa<AllocaInst>(origDst)) {
    if (origSrc->getType()->getPointerAddressSpace() != Src->getType()->getPointerAddressSpace()) {
      Value *SrcCast = BitCastInst::Create(
          Instruction::BitCast, origSrc,
          IGCLLVM::get(dyn_cast<PointerType>(Src->getType()), origSrc->getType()->getPointerAddressSpace()), "", &I);
      I.replaceUsesOfWith(Src, SrcCast);
    }
    if (origDst->getType()->getPointerAddressSpace() != Dst->getType()->getPointerAddressSpace()) {
      Value *DstCast = BitCastInst::Create(
          Instruction::BitCast, origDst,
          IGCLLVM::get(dyn_cast<PointerType>(Dst->getType()), origDst->getType()->getPointerAddressSpace()), "", &I);
      I.replaceUsesOfWith(Dst, DstCast);
    }
  }
}

// When this pass encounters a byVal argument, it creates an alloca to then copy the data from global memory to local
// memory. When creating a new alloca, it replaces all occurrences of the argument in the function with that alloca.
// Problems arises when the pointer operant (or more precisely its address space) is replaced:
// 1. In GetElementPtrInst, the resulting pointer of this instruction is in a different address space.
//    On the other hand, a load instruction that uses the returned GetElementPtrInst pointer still operates on the old
//    address space. By which we are referring to the wrong area of ​​memory. The resolution for this problem is to
//    create new load instruction.
// 2. In MemCpyInst, specifically generated for structs used in loops, where two allocas of the same struct type are
// created used
//    to save and restore struct values. When one is copied to another, this pass incorrectly uses the addrspace of the
//    ByVal argument instead of the local addrspace of the alloca. We fix this by casting the src and dst of the memcpy
//    to the correct addrspace.
// This is WA for a bug in LLVM 11.
void SubroutineInliner::verifyAddrSpaceMismatch(CallGraphSCC &SCC) {
  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (F)
      visit(F);
  }
}

bool SubroutineInliner::runOnSCC(CallGraphSCC &SCC) {
  FSA = &getAnalysis<EstimateFunctionSize>();
  MDUW = &getAnalysis<MetaDataUtilsWrapper>();
#if LLVM_VERSION_MAJOR >= 16
  if (skipSCC(SCC))
    return false;
  bool changed = inlineCalls(SCC);
#else
  bool changed = LegacyInlinerBase::runOnSCC(SCC);
#endif
  if (changed)
    verifyAddrSpaceMismatch(SCC);

  return changed;
}

/// \brief Get the inline cost for the subroutine-inliner.
///
InlineCost SubroutineInliner::getInlineCost(IGCLLVM::CallSiteRef CS) {
  Function *Callee = CS.getCalledFunction();
  Function *Caller = CS.getCaller();
  CodeGenContext *pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  // Inline direct calls to functions with always inline attribute or a function
  // whose estimated size is under certain predefined limit.
  if (Callee && !Callee->isDeclaration() && isInlineViable(*Callee).isSuccess()) {
    if (CS.hasFnAttr(llvm::Attribute::AlwaysInline))
      return llvm::InlineCost::getAlways("Per AlwaysInline function attribute");

    int FCtrl = getFunctionControl(pCtx);

    if (IGC::ForceAlwaysInline(pCtx))
      return llvm::InlineCost::getAlways("IGC set force always inline");

    if (pCtx->m_enableSubroutine == false)
      return llvm::InlineCost::getAlways("Disabled subroutines/stackcalls");

    if (Callee->hasFnAttribute(llvm::Attribute::NoInline))
      return llvm::InlineCost::getNever("Per NoInline function attribute");

    if (Callee->hasFnAttribute("KMPLOCK"))
      return llvm::InlineCost::getNever("Has KMPLOCK function attribute");

    if (Callee->hasFnAttribute("igc-force-stackcall"))
      return llvm::InlineCost::getNever("Has igc-force-stackcall function attribute");

    if (FCtrl == FLAG_FCALL_DEFAULT) {
      std::size_t PerFuncThreshold = IGC_GET_FLAG_VALUE(SubroutineInlinerThreshold);

      // A single block function containing only a few instructions.
      auto isTrivialCall = [](const llvm::Function *F) {
        if (!F->empty() && F->size() == 1)
          return F->front().size() <= 5;
        return false;
      };

      if (FSA->getExpandedSize(Caller) <= PerFuncThreshold) {
        return llvm::InlineCost::getAlways("Caller size smaller than per func. threshold");
      }
      if (isTrivialCall(Callee) || FSA->onlyCalledOnce(Callee, Caller)) {
        return llvm::InlineCost::getAlways("Callee is called only once");
      }
      if (!FSA->shouldEnableSubroutine()) {
        // This function returns true if the estimated total inlining size exceeds some module threshold.
        // If we don't exceed it, and there's no preference on inline vs noinline, we just inline.
        return llvm::InlineCost::getAlways("Did not meet inline per function size threshold");
      }
    }
  }

  return llvm::InlineCost::getNever("Did not meet any inlining conditions");
}

Pass *IGC::createSubroutineInlinerPass() {
  initializeSubroutineInlinerPass(*PassRegistry::getPassRegistry());
  return new SubroutineInliner();
}

bool FunctionGroup::checkSimdModeValid(SIMDMode Mode) const {
  switch (Mode) {
  default:
    break;
  case SIMDMode::SIMD8:
    return SIMDModeValid[0];
  case SIMDMode::SIMD16:
    return SIMDModeValid[1];
  case SIMDMode::SIMD32:
    return SIMDModeValid[2];
  }
  return true;
}

void FunctionGroup::setSimdModeInvalid(SIMDMode Mode) {
  switch (Mode) {
  default:
    break;
  case SIMDMode::SIMD8:
    SIMDModeValid[0] = false;
    break;
  case SIMDMode::SIMD16:
    SIMDModeValid[1] = false;
    break;
  case SIMDMode::SIMD32:
    SIMDModeValid[2] = false;
    break;
  }
}
