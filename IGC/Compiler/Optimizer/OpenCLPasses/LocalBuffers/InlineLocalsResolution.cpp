/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryToSLM.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/DebugInfo/Utils.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvmWrapper/Support/Alignment.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <unordered_set>

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-resolve-inline-locals"
#define PASS_DESCRIPTION "Resolve inline local variables/buffers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InlineLocalsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(InlineLocalsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char InlineLocalsResolution::ID = 0;
const llvm::StringRef BUILTIN_MEMPOOL = "__builtin_IB_AllocLocalMemPool";

InlineLocalsResolution::InlineLocalsResolution() : ModulePass(ID), m_pGV(nullptr) {
  initializeInlineLocalsResolutionPass(*PassRegistry::getPassRegistry());
}

static bool useAsPointerOnly(Value *V) {
  IGC_ASSERT_MESSAGE(V->getType()->isPointerTy(), "Expect the input value is a pointer!");

  SmallSet<PHINode *, 8> VisitedPHIs;
  SmallVector<Value *, 16> WorkList;
  WorkList.push_back(V);

  StoreInst *ST = nullptr;
  PHINode *PN = nullptr;
  while (!WorkList.empty()) {
    Value *Val = WorkList.pop_back_val();
    for (auto *U : Val->users()) {
      Operator *Op = dyn_cast<Operator>(U);
      if (!Op)
        continue;
      switch (Op->getOpcode()) {
      default:
        // Bail out for unknown operations.
        return false;
      case Instruction::Store:
        ST = cast<StoreInst>(U);
        // Bail out if it's used as the value operand.
        if (ST->getValueOperand() == Val)
          return false;
        // FALL THROUGH
      case Instruction::Load:
        // Safe use in LD/ST as pointer only.
        continue;
      case Instruction::PHI:
        PN = cast<PHINode>(U);
        // Skip if it's already visited.
        if (!VisitedPHIs.insert(PN).second)
          continue;
        // FALL THROUGH
      case Instruction::BitCast:
      case Instruction::Select:
      case Instruction::GetElementPtr:
        // Need to check their usage further.
        break;
      }
      WorkList.push_back(U);
    }
  }

  return true;
}

bool InlineLocalsResolution::runOnModule(Module &M) {
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  if (!modMD->compOpt.OptDisable)
    filterGlobals(M);
  // Compute the offset of each inline local in the kernel,
  // and their total size.
  llvm::MapVector<Function *, unsigned int> sizeMap;
  collectInfoOnSharedLocalMem(M);
  computeOffsetList(M, sizeMap);

  LLVMContext &C = M.getContext();

  for (Function &F : M) {
    if (F.isDeclaration() || !isEntryFunc(pMdUtils, &F)) {
      continue;
    }

    unsigned int totalSize = 0;

    // Get the offset at which local arguments start
    auto sizeIter = sizeMap.find(&F);
    if (sizeIter != sizeMap.end()) {
      totalSize += sizeIter->second;
    }

    // Set the high bits to a non-0 value.
    totalSize = (totalSize & LOW_BITS_MASK);

    bool IsFirstSLMArgument = true;
    for (Function::arg_iterator A = F.arg_begin(), AE = F.arg_end(); A != AE; ++A) {
      Argument *arg = &(*A);
      PointerType *ptrType = dyn_cast<PointerType>(arg->getType());
      // Check that this is a pointer
      if (!ptrType) {
        continue;
      }

      // To the local address space
      if (ptrType->getAddressSpace() != ADDRESS_SPACE_LOCAL) {
        continue;
      }

      // Which is used
      if (arg->use_empty()) {
        continue;
      }

      bool UseAsPointerOnly = useAsPointerOnly(arg);
      unsigned Offset = totalSize;
      if (!UseAsPointerOnly)
        Offset |= VALID_LOCAL_HIGH_BITS;

      if (IsFirstSLMArgument) {
        auto BufType = ArrayType::get(Type::getInt8Ty(M.getContext()), 0);
        auto ExtSLM =
            new GlobalVariable(M, BufType, false, GlobalVariable::ExternalLinkage, nullptr, F.getName() + "-ExtSLM",
                               nullptr, GlobalVariable::ThreadLocalMode::NotThreadLocal, ADDRESS_SPACE_LOCAL);
        auto NewPtr = ConstantExpr::getBitCast(ExtSLM, arg->getType());
        arg->replaceAllUsesWith(NewPtr);
        // Update MD.
        LocalOffsetMD localOffset;
        localOffset.m_Var = ExtSLM;
        localOffset.m_Offset = Offset;
        modMD->FuncMD[&F].localOffsets.push_back(localOffset);

        IGC::appendToUsed(M, ExtSLM);
        IsFirstSLMArgument = false;
      } else {
        // FIXME: The following code should be removed as well by
        // populating similar adjustment in prolog during code
        // emission.
        // Ok, now we need to add an offset, in bytes, which is equal to totalSize.
        // Bitcast to i8*, GEP, bitcast back to original type.
        Value *sizeConstant = ConstantInt::get(Type::getInt32Ty(C), Offset);
        SmallVector<Value *, 1> idx(1, sizeConstant);
        Instruction *pInsertBefore = &(*F.begin()->getFirstInsertionPt());
        Type *pCharType = Type::getInt8Ty(C);
        Type *pLocalCharPtrType = pCharType->getPointerTo(ADDRESS_SPACE_LOCAL);
        Instruction *pCharPtr = BitCastInst::CreatePointerCast(arg, pLocalCharPtrType, "localToChar", pInsertBefore);
        Value *pMovedCharPtr = GetElementPtrInst::Create(pCharType, pCharPtr, idx, "movedLocal", pInsertBefore);

        Value *pMovedPtr = CastInst::CreatePointerCast(pMovedCharPtr, ptrType, "charToLocal", pInsertBefore);

        // Running over arg users and use replaceUsesOfWith to fix them is not enough,
        // because it does not cover the usage of arg in metadata (e.g. for debug info intrinsic).
        // Thus, replaceAllUsesWith should be used in order to fix also debug info.
        arg->replaceAllUsesWith(pMovedPtr);
        // The above operation changed also the "arg" operand in "charPtr" to "movedPtr"
        // Thus, we need to fix it back (otherwise the LLVM IR will be invalid)
        pCharPtr->replaceUsesOfWith(pMovedPtr, arg);
      }
    }
  }

  return true;
}

void InlineLocalsResolution::filterGlobals(Module &M) {
  // This data structure saves all the unused nodes,
  // including the global variable definition itself, as well as all successive recursive user nodes,
  // in all the def-use trees corresponding to all the global variables in the entire Module.
  std::unordered_set<Value *> unusedNodes_forModule;

  // let's loop all global variables
  for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
    // We only care about global variables, not other globals.
    GlobalVariable *globalVar = dyn_cast<GlobalVariable>(&*I);
    if (!globalVar) {
      continue;
    }

    PointerType *ptrType = cast<PointerType>(globalVar->getType());
    // We only care about local address space here.
    if (ptrType->getAddressSpace() != ADDRESS_SPACE_LOCAL) {
      continue;
    }

    // If the globalVar is determined to be unused,
    // this data structure saves the globalVar,
    // as well as all successive recursive user nodes in that def-use tree.
    std::unordered_set<Value *> unusedNodes_forOne;
    if (unusedGlobal(globalVar, unusedNodes_forOne))
      unusedNodes_forModule.insert(unusedNodes_forOne.begin(), unusedNodes_forOne.end());
  }

  // We only remove all the unused nodes for this Module,
  // after we are done processing all the global variables for the entire Module,
  // to prevent iterators becoming invalidated when elements get removed from the ilist.
  for (auto &element : unusedNodes_forModule) {
    // for all unused Values,
    //   replace all uses with undefs
    //   delete the values
    if (Instruction *node = dyn_cast<Instruction>(element)) {
      Type *Ty = node->getType();
      if (!Ty->isVoidTy())
        node->replaceAllUsesWith(UndefValue::get(Ty));
      node->eraseFromParent();
    } else if (GlobalVariable *node = dyn_cast<GlobalVariable>(element)) {
      Type *Ty = node->getType();
      if (!Ty->isVoidTy())
        node->replaceAllUsesWith(UndefValue::get(Ty));
      node->eraseFromParent();
    }
    // All other types of nodes are ignored.
  }
}

bool InlineLocalsResolution::unusedGlobal(Value *V, std::unordered_set<Value *> &unusedNodes) {
  for (Value::user_iterator U = V->user_begin(), UE = V->user_end(); U != UE; ++U) {
    if (GlobalVariable *globalVar = dyn_cast<GlobalVariable>(*U)) {
      if (!unusedGlobal(*U, unusedNodes))
        return false;
    } else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*U)) {
      if (!unusedGlobal(*U, unusedNodes))
        return false;
    } else if (BitCastInst *bitcast = dyn_cast<BitCastInst>(*U)) {
      if (!unusedGlobal(*U, unusedNodes))
        return false;
    } else if (StoreInst *store = dyn_cast<StoreInst>(*U)) {
      if (store->isUnordered()) {
        if (store->getPointerOperand() == V) {
          if (!unusedGlobal(*U, unusedNodes))
            return false;
        } else if (store->getValueOperand() == V) {
          return false;
        }
      } else {
        return false;
      }
    } else { // some other instruction
      return false;
    }
  }
  // add an unused node to the data structure
  unusedNodes.insert(V);
  return true;
}

void InlineLocalsResolution::collectInfoOnSharedLocalMem(Module &M) {
  const auto pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  // first we collect SLM usage on GET_MEMPOOL_PTR
  if (M.getFunction(BUILTIN_MEMPOOL) != nullptr) {
    const GT_SYSTEM_INFO platform = pCtx->platform.GetGTSystemInfo();

    SmallVector<CallInst *, 8> callsToReplace;
    unsigned maxBytesOnModule = 0;
    unsigned maxAlignOnModule = 0;

    unsigned int maxWorkGroupSize = 448;
    maxWorkGroupSize = pCtx->platform.getOfflineCompilerMaxWorkGroupSize();
    if (pCtx->platform.enableMaxWorkGroupSizeCalculation() && platform.EUCount != 0 && platform.SubSliceCount != 0) {
      unsigned int maxNumEUsPerSubSlice = platform.EuCountPerPoolMin;
      if (platform.EuCountPerPoolMin == 0 || pCtx->platform.supportPooledEU()) {
        maxNumEUsPerSubSlice = platform.EUCount / platform.SubSliceCount;
      }
      const unsigned int numThreadsPerEU = platform.ThreadCount / platform.EUCount;
      unsigned int simdSizeUsed = 8;
      simdSizeUsed = numLanes(pCtx->platform.getMinDispatchMode());
      unsigned int maxWS = maxNumEUsPerSubSlice * numThreadsPerEU * simdSizeUsed;
      if (!iSTD::IsPowerOfTwo(maxWS)) {
        maxWS = iSTD::RoundPower2((DWORD)maxWS) >> 1;
      }
      maxWorkGroupSize = std::min(maxWS, 1024u);
    }

    // scan inst to collect all call instructions

    for (Function &F : M) {
      if (F.isDeclaration()) {
        continue;
      }

      unsigned maxBytesOnFunc = 0;
      for (auto I = inst_begin(&F), IE = inst_end(&F); I != IE; ++I) {
        Instruction *inst = &(*I);
        if (CallInst *CI = dyn_cast<CallInst>(inst)) {
          Function *pFunc = CI->getCalledFunction();
          if (pFunc && pFunc->getName().equals(BUILTIN_MEMPOOL)) {
            // should always be called with constant operands
            IGC_ASSERT(isa<ConstantInt>(CI->getArgOperand(0)));
            IGC_ASSERT(isa<ConstantInt>(CI->getArgOperand(1)));
            IGC_ASSERT(isa<ConstantInt>(CI->getArgOperand(2)));

            const unsigned int allocAllWorkgroups = unsigned(cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue());
            const unsigned int numAdditionalElements =
                unsigned(cast<ConstantInt>(CI->getArgOperand(1))->getZExtValue());
            const unsigned int elementSize = unsigned(cast<ConstantInt>(CI->getArgOperand(2))->getZExtValue());

            unsigned int numElements = numAdditionalElements;
            if (allocAllWorkgroups) {
              numElements += maxWorkGroupSize;
            }
            const unsigned int size = numElements * elementSize;
            const unsigned int align = elementSize;

            maxBytesOnFunc = std::max(maxBytesOnFunc, size);
            maxBytesOnModule = std::max(maxBytesOnModule, size);
            maxAlignOnModule = std::max(maxAlignOnModule, align);

            callsToReplace.push_back(CI);
          }
        }
      }
      if (maxBytesOnFunc != 0) {
        m_FuncToMemPoolSizeMap[&F] = maxBytesOnFunc;
      }
    }

    if (!callsToReplace.empty()) {

      Type *bufType = ArrayType::get(Type::getInt8Ty(M.getContext()), uint64_t(maxBytesOnModule));

      m_pGV = new GlobalVariable(M, bufType, false, GlobalVariable::ExternalLinkage,
                                 ConstantAggregateZero::get(bufType), "GenSLM.LocalMemPoolOnGetMemPoolPtr", nullptr,
                                 GlobalVariable::ThreadLocalMode::NotThreadLocal, ADDRESS_SPACE_LOCAL);

      m_pGV->setAlignment(IGCLLVM::getCorrectAlign(maxAlignOnModule));

      for (auto call : callsToReplace) {
        CastInst *cast = new BitCastInst(m_pGV, call->getCalledFunction()->getReturnType(), "mempoolcast", call);

        cast->setDebugLoc(call->getDebugLoc());

        call->replaceAllUsesWith(cast);
        call->eraseFromParent();
      }
    }
  }

  // let's loop all global variables
  for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E; ++I) {
    // We only care about global variables, not other globals.
    GlobalVariable *globalVar = dyn_cast<GlobalVariable>(&*I);
    if (!globalVar) {
      continue;
    }

    PointerType *ptrType = dyn_cast<PointerType>(globalVar->getType());
    IGC_ASSERT_MESSAGE(ptrType, "The type of a global variable must be a pointer type");
    if (!ptrType) {
      continue;
    }

    // We only care about local address space here.
    if (ptrType->getAddressSpace() != ADDRESS_SPACE_LOCAL) {
      continue;
    }

    // For each SLM buffer, set section to avoid alignment changing by llvm.
    // Add external linkage and DSO scope information.
    globalVar->setLinkage(GlobalValue::ExternalLinkage);
    globalVar->setDSOLocal(false);
    globalVar->setSection("localSLM");

    // Find the functions which this globalVar belongs to....
    for (Value::user_iterator U = globalVar->user_begin(), UE = globalVar->user_end(); U != UE; ++U) {
      Instruction *user = dyn_cast<Instruction>(*U);
      if (!user) {
        continue;
      }

      Function *parentF = user->getParent()->getParent();
      bool emitError = false;
      if (pCtx->type == ShaderType::OPENCL_SHADER) {
        // If this option is passed, emit error when extern functions use local SLM
        auto ClContext = static_cast<OpenCLProgramContext *>(pCtx);
        emitError = ClContext->m_Options.EmitErrorsForLibCompilation;
      }
      if (parentF->hasFnAttribute("referenced-indirectly") && emitError) {
        IGC_ASSERT_MESSAGE(0, "Cannot reference localSLM in indirectly-called functions");
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->EmitError(
            "Cannot reference localSLM in indirectly-called functions", globalVar);
        return;
      }
      m_FuncToVarsMap[parentF].insert(globalVar);
    }
  }

  // set debugging info, and insert mov inst.
  for (const auto &I : m_FuncToVarsMap) {
    Function *userFunc = I.first;
    for (auto *G : I.second) {
      Instruction *pInsertBefore = &(*userFunc->begin()->getFirstNonPHIOrDbg());
      TODO("Should inline local buffer points to origin offset 'globalVar' or to fixed offset 'pMovedPtr'?");
      Utils::UpdateGlobalVarDebugInfo(G, G, pInsertBefore, true);
    }
  }
}

void InlineLocalsResolution::computeOffsetList(Module &M, llvm::MapVector<Function *, unsigned int> &sizeMap) {
  llvm::MapVector<Function *, llvm::MapVector<GlobalVariable *, unsigned int>> offsetMap;
  MetaDataUtils *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  ModuleMetaData *modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  DataLayout DL = M.getDataLayout();
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

  if (m_FuncToVarsMap.empty()) {
    return;
  }

  // let's traverse the CallGraph to calculate the local
  // variables of kernel from all user functions.
  m_chkSet.clear();
  for (auto &N : CG) {
    Function *f = N.second->getFunction();
    if (!f || f->isDeclaration() || m_chkSet.find(f) != m_chkSet.end())
      continue;
    traverseCGN(*N.second);
  }

  // set up the offsetMap;
  for (const auto &I : m_FuncToVarsMap) {
    Function *F = I.first;

    // loop through all global variables
    for (auto G : I.second) {
      auto itr = sizeMap.find(F);
      unsigned int offset = itr == sizeMap.end() ? 0 : itr->second;
      offset = iSTD::Align(offset, (unsigned)DL.getPreferredAlign(G).value());
      // Save the offset of the current local
      // (set the high bits to be non-0 here too)
      offsetMap[F][G] = (offset & LOW_BITS_MASK);

      // And the total size after this local is added
      Type *varType = G->getValueType();
      if (G == m_pGV) {
        // it is GetMemPoolPtr usage
        offset += m_FuncToMemPoolSizeMap[F];
      } else {
        offset += (unsigned int)DL.getTypeAllocSize(varType);
      }
      sizeMap[F] = offset;
    }
  }

  // Ok, we've collected the information, now write it into the MD.
  for (auto &iter : sizeMap) {
    // ignore non-entry functions.
    if (!isEntryFunc(pMdUtils, iter.first)) {
      continue;
    }

    // If this function doesn't have any locals, no need for MD.
    if (iter.second == 0) {
      continue;
    }

    // We need the total size to have at least 32-byte alignment.
    // This is because right after the space allocated to the inline locals,
    // we are going to have inline parameters. So, we need to make sure the
    // first local parameter is appropriately aligned, which, at worst,
    // can be 256 bits.
    iter.second = iSTD::Align(iter.second, 32);

    // Add the size information of this function
    modMD->FuncMD[iter.first].localSize = iter.second;

    // And now the offsets.
    for (const auto &offsetIter : offsetMap[iter.first]) {
      unsigned Offset = offsetIter.second;
      if (!useAsPointerOnly(offsetIter.first))
        Offset |= VALID_LOCAL_HIGH_BITS;

      LocalOffsetMD localOffset;
      localOffset.m_Var = offsetIter.first;
      localOffset.m_Offset = Offset;
      modMD->FuncMD[iter.first].localOffsets.push_back(localOffset);
    }
  }
  pMdUtils->save(M.getContext());
}

void InlineLocalsResolution::traverseCGN(const llvm::CallGraphNode &CGN) {
  Function *f = CGN.getFunction();

  // mark this function
  m_chkSet.insert(f);

  for (const auto &N : CGN) {
    Function *sub = N.second->getFunction();
    if (!sub || sub->isDeclaration())
      continue;

    // we reach here, because there is sub-function inside the node
    if (m_chkSet.find(sub) == m_chkSet.end()) {
      // this sub-routine is not visited before.
      // visit it first
      traverseCGN(*N.second);
    }

    // the sub-routine was visited before, collect information

    // count each global on this sub-routine
    GlobalVariableSet &GS_f = m_FuncToVarsMap[f];
    const GlobalVariableSet &GS_sub = m_FuncToVarsMap[sub];
    GS_f.insert(GS_sub.begin(), GS_sub.end());

    // automatic storages
    if (m_FuncToMemPoolSizeMap.find(sub) != m_FuncToMemPoolSizeMap.end()) {
      // this sub-function has automatic storage
      if (m_FuncToMemPoolSizeMap.find(f) != m_FuncToMemPoolSizeMap.end()) {
        // caller has its own memory pool size, choose the max
        m_FuncToMemPoolSizeMap[f] = std::max(m_FuncToMemPoolSizeMap[f], m_FuncToMemPoolSizeMap[sub]);
      } else {
        m_FuncToMemPoolSizeMap[f] = m_FuncToMemPoolSizeMap[sub];
      }
    }
  }
}
