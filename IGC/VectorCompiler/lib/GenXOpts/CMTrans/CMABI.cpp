/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
/// CMABI
/// -----
///
/// This pass fixes ABI issues for the genx backend. Currently, it
///
/// - transforms pass by pointer argument into copy-in and copy-out;
///
/// - localizes global scalar or vector variables into copy-in and copy-out;
///
/// - passes bool arguments as i8 (matches cm-icl's hehavior).
///
//===----------------------------------------------------------------------===//


#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/Attributes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/Alignment.h"

#include "Probe/Assertion.h"

#include "vc/GenXOpts/GenXOpts.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/BreakConst.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/GenX/TransformArgCopy.h"
#include "vc/Utils/GenX/TypeSize.h"
#include "vc/Utils/General/DebugInfo.h"
#include "vc/Utils/General/FunctionAttrs.h"
#include "vc/Utils/General/InstRebuilder.h"
#include "vc/Utils/General/STLExtras.h"
#include "vc/Utils/General/Types.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define DEBUG_TYPE "cmabi"

using namespace llvm;

static cl::opt<unsigned>
    MaxCMABIByvalSize("vc-max-cmabi-byval-size", cl::init(128), cl::Hidden,
                      cl::desc("Maximum struct size to be passed by value for "
                               "internal functions in bytes."));

STATISTIC(NumArgumentsTransformed, "Number of pointer arguments transformed");

namespace llvm {
void initializeCMABIAnalysisPass(PassRegistry &);
void initializeCMABIPass(PassRegistry &);
void initializeCMLowerVLoadVStorePass(PassRegistry &);
}

/// Localizing global variables
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// General idea of localizing global variables into locals. Globals used in
/// different kernels get a seperate copy and they are always invisiable to
/// other kernels and we can safely localize all globals used (including
/// indirectly) in a kernel. For example,
///
/// .. code-block:: text
///
///   @gv1 = global <8 x float> zeroinitializer, align 32
///   @gv2 = global <8 x float> zeroinitializer, align 32
///   @gv3 = global <8 x float> zeroinitializer, align 32
///
///   define dllexport void @f0() {
///     call @f1()
///     call @f2()
///     call @f3()
///   }
///
///   define internal void @f1() {
///     ; ...
///     store <8 x float> %splat1, <8 x float>* @gv1, align 32
///   }
///
///   define internal void @f2() {
///     ; ...
///     store <8 x float> %splat2, <8 x float>* @gv2, align 32
///   }
///
///   define internal void @f3() {
///     %1 = <8 x float>* @gv1, align 32
///     %2 = <8 x float>* @gv2, align 32
///     %3 = fadd <8 x float> %1, <8 x float> %2
///     store <8 x float> %3, <8 x float>* @gv3, align 32
///   }
///
/// will be transformed into
///
/// .. code-block:: text
///
///   define dllexport void @f0() {
///     %v1 = alloca <8 x float>, align 32
///     %v2 = alloca <8 x float>, align 32
///     %v3 = alloca <8 x float>, align 32
///
///     %0 = load <8 x float> * %v1, align 32
///     %1 = { <8 x float> } call @f1_transformed(<8 x float> %0)
///     %2 = extractvalue { <8 x float> } %1, 0
///     store <8  x float> %2, <8 x float>* %v1, align 32
///
///     %3 = load <8 x float> * %v2, align 32
///     %4 = { <8 x float> } call @f2_transformed(<8 x float> %3)
///     %5 = extractvalue { <8 x float> } %4, 0
///     store <8  x float> %5, <8 x float>* %v1, align 32
///
///     %6 = load <8 x float> * %v1, align 32
///     %7 = load <8 x float> * %v2, align 32
///     %8 = load <8 x float> * %v3, align 32
///
///     %9 = { <8 x float>, <8 x float>, <8 x float> }
///          call @f3_transformed(<8 x float> %6, <8 x float> %7, <8 x float> %8)
///
///     %10 = extractvalue { <8 x float>, <8 x float>, <8 x float> } %9, 0
///     store <8  x float> %10, <8 x float>* %v1, align 32
///     %11 = extractvalue { <8 x float>, <8 x float>, <8 x float> } %9, 1
///     store <8  x float> %11, <8 x float>* %v2, align 32
///     %12 = extractvalue { <8 x float>, <8 x float>, <8 x float> } %9, 2
///     store <8  x float> %12, <8 x float>* %v3, align 32
///   }
///
/// All callees will be updated accordingly, E.g. f1_transformed becomes
///
/// .. code-block:: text
///
///   define internal { <8 x float> } @f1_transformed(<8 x float> %v1) {
///     %0 = alloca <8 x float>, align 32
///     store <8 x float> %v1, <8 x float>* %0, align 32
///     ; ...
///     store <8 x float> %splat1, <8 x float>* @0, align 32
///     ; ...
///     %1 = load <8 x float>* %0, align 32
///     %2 = insertvalue { <8 x float> } undef, <8 x float> %1, 0
///     ret { <8 x float> } %2
///   }
///
namespace {

// \brief Collect necessary information for global variable localization.
class LocalizationInfo {
public:
  typedef SetVector<GlobalVariable *> GlobalSetTy;

  explicit LocalizationInfo(Function *F) : Fn(F) {}
  LocalizationInfo() : Fn(0) {}

  Function *getFunction() const { return Fn; }
  bool empty() const { return Globals.empty(); }
  GlobalSetTy &getGlobals() { return Globals; }

  // \brief Add a global.
  void addGlobal(GlobalVariable *GV) {
    Globals.insert(GV);
  }

  // \brief Add all globals from callee.
  void addGlobals(LocalizationInfo &LI) {
    Globals.insert(LI.getGlobals().begin(), LI.getGlobals().end());
  }

private:
  // \brief The function being analyzed.
  Function *Fn;

  // \brief Global variables that are used directly or indirectly.
  GlobalSetTy Globals;
};

class CMABIAnalysis : public ModulePass {
  // This map captures all global variables to be localized.
  std::vector<LocalizationInfo *> LocalizationInfoObjs;

public:
  static char ID;

  // Kernels in the module being processed.
  SmallPtrSet<Function *, 8> Kernels;

  // Map from function to the index of its LI in LI storage
  SmallDenseMap<Function *, LocalizationInfo *> GlobalInfo;

  CMABIAnalysis() : ModulePass{ID} {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<CallGraphWrapperPass>();
    AU.setPreservesAll();
  }

  StringRef getPassName() const override { return "GenX CMABI analysis"; }

  bool runOnModule(Module &M) override;

  void releaseMemory() override {
    for (auto *LI : LocalizationInfoObjs)
      delete LI;
    LocalizationInfoObjs.clear();
    Kernels.clear();
    GlobalInfo.clear();
  }

  // \brief Returns the localization info associated to a function.
  LocalizationInfo &getLocalizationInfo(Function *F) {
    if (GlobalInfo.count(F))
      return *GlobalInfo[F];
    LocalizationInfo *LI = new LocalizationInfo{F};
    LocalizationInfoObjs.push_back(LI);
    GlobalInfo[F] = LI;
    return *LI;
  }

private:
  bool runOnCallGraph(CallGraph &CG);
  void analyzeGlobals(CallGraph &CG);

  void addDirectGlobal(Function *F, GlobalVariable *GV) {
    getLocalizationInfo(F).addGlobal(GV);
  }

  // \brief Add all globals from callee to caller.
  void addIndirectGlobal(Function *F, Function *Callee) {
    getLocalizationInfo(F).addGlobals(getLocalizationInfo(Callee));
  }

  void defineGVDirectUsers(GlobalVariable &GV);
};

struct CMABI : public CallGraphSCCPass {
  static char ID;

  CMABI() : CallGraphSCCPass(ID) {
    initializeCMABIPass(*PassRegistry::getPassRegistry());
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    CallGraphSCCPass::getAnalysisUsage(AU);
    AU.addRequired<CMABIAnalysis>();
  }

  bool runOnSCC(CallGraphSCC &SCC) override;

private:

  CallGraphNode *ProcessNode(CallGraphNode *CGN);

  // Fix argument passing for kernels.
  CallGraphNode *TransformKernel(Function *F);

  // Major work is done in this method.
  CallGraphNode *TransformNode(Function &F,
                               SmallPtrSet<Argument *, 8> &ArgsToTransform,
                               LocalizationInfo &LI);

  // \brief Create allocas for globals and replace their uses.
  void LocalizeGlobals(LocalizationInfo &LI);

  // \brief Diagnose illegal overlapping by-ref args.
  void diagnoseOverlappingArgs(CallInst *CI);

  // Already visited functions.
  SmallPtrSet<Function *, 8> AlreadyVisited;
  CMABIAnalysis *Info;
};

} // namespace

char CMABIAnalysis::ID = 0;
INITIALIZE_PASS_BEGIN(CMABIAnalysis, "cmabi-analysis",
                      "helper analysis pass to get info for CMABI", false, true)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(CMABIAnalysis, "cmabi-analysis",
                    "Fix ABI issues for the genx backend", false, true)

bool CMABIAnalysis::runOnModule(Module &M) {
  runOnCallGraph(getAnalysis<CallGraphWrapperPass>().getCallGraph());
  return false;
}

bool CMABIAnalysis::runOnCallGraph(CallGraph &CG) {
  // Analyze global variable usages and for each function attaches global
  // variables to be copy-in and copy-out.
  analyzeGlobals(CG);

  auto getValue = [](Metadata *M) -> Value * {
    if (auto VM = dyn_cast<ValueAsMetadata>(M))
      return VM->getValue();
    return nullptr;
  };

  // Collect all CM kernels from named metadata.
  if (NamedMDNode *Named =
          CG.getModule().getNamedMetadata(genx::FunctionMD::GenXKernels)) {
    IGC_ASSERT(Named);
    for (unsigned I = 0, E = Named->getNumOperands(); I != E; ++I) {
      MDNode *Node = Named->getOperand(I);
      if (Function *F =
              dyn_cast_or_null<Function>(getValue(Node->getOperand(0))))
        Kernels.insert(F);
    }
  }

  // no change.
  return false;
}

bool CMABI::runOnSCC(CallGraphSCC &SCC) {
  Info = &getAnalysis<CMABIAnalysis>();
  bool Changed = false;
  bool LocalChange;

  // Diagnose overlapping by-ref args.
  for (auto i = SCC.begin(), e = SCC.end(); i != e; ++i) {
    Function *F = (*i)->getFunction();
    if (!F || F->empty())
      continue;
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui) {
      auto CI = dyn_cast<CallInst>(ui->getUser());
      if (CI && IGCLLVM::getNumArgOperands(CI) == ui->getOperandNo())
        diagnoseOverlappingArgs(CI);
    }
  }

  // Iterate until we stop transforming from this SCC.
  do {
    LocalChange = false;
    for (CallGraphSCC::iterator I = SCC.begin(), E = SCC.end(); I != E; ++I) {
      if (CallGraphNode *CGN = ProcessNode(*I)) {
        LocalChange = true;
        SCC.ReplaceNode(*I, CGN);
      }
    }
    Changed |= LocalChange;
  } while (LocalChange);

  return Changed;
}

// \brief Create allocas for globals directly used in this kernel and
// replace all uses.
//
// FIXME: it is not always posible to localize globals with addrspace different
// from private. In some cases type info link is lost - casts, stores of
// pointers.
void CMABI::LocalizeGlobals(LocalizationInfo &LI) {
  const LocalizationInfo::GlobalSetTy &Globals = LI.getGlobals();
  typedef LocalizationInfo::GlobalSetTy::const_iterator IteratorTy;

  SmallDenseMap<Value *, Value *> GlobalsToReplace;
  Function *Fn = LI.getFunction();
  for (IteratorTy I = Globals.begin(), E = Globals.end(); I != E; ++I) {
    GlobalVariable *GV = (*I);
    LLVM_DEBUG(dbgs() << "Localizing global: " << *GV << "\n  ");

    Instruction &FirstI = *Fn->getEntryBlock().begin();
    Type *ElemTy = GV->getType()->getElementType();
    IGCLLVM::Align GVAlign = IGCLLVM::getCorrectAlign(GV->getAlignment());
    AllocaInst *Alloca = new AllocaInst(ElemTy, vc::AddrSpace::Private,
                                        /*ArraySize=*/nullptr, GVAlign,
                                        GV->getName() + ".local", &FirstI);

    if (!isa<UndefValue>(GV->getInitializer()))
      new StoreInst(GV->getInitializer(), Alloca, /*isVolatile=*/false,
                    GVAlign, &FirstI);

    vc::DIBuilder::createDbgDeclareForLocalizedGlobal(*Alloca, *GV, FirstI);

    GlobalsToReplace.insert(std::make_pair(GV, Alloca));
  }

  // Replaces all globals uses within this function.
  vc::replaceUsesWithinFunction(GlobalsToReplace, Fn);
}

CallGraphNode *CMABI::ProcessNode(CallGraphNode *CGN) {
  Function *F = CGN->getFunction();

  // Nothing to do for declarations or already visited functions.
  if (!F || F->isDeclaration() || AlreadyVisited.count(F))
    return 0;

  vc::breakConstantExprs(F, vc::LegalizationStage::NotLegalized);

  // Variables to be localized.
  LocalizationInfo &LI = Info->getLocalizationInfo(F);

  // This is a kernel.
  if (Info->Kernels.count(F)) {
    // Localize globals for kernels.
    if (!LI.getGlobals().empty())
      LocalizeGlobals(LI);

    // Check whether there are i1 or vxi1 kernel arguments.
    for (auto AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI)
      if (AI->getType()->getScalarType()->isIntegerTy(1))
        return TransformKernel(F);

    // No changes to this kernel's prototype.
    return 0;
  }

  // Have to localize implicit arg globals in functions with fixed signature.
  // FIXME: There's no verification that globals are for implicit args. General
  //        private globals may be localized here, but it is not possible to
  //        use them in such functions at all. A nice place for diagnostics.
  if (vc::isFixedSignatureFunc(*F)) {
    if (!LI.getGlobals().empty())
      LocalizeGlobals(LI);
    return nullptr;
  }

  // Check transformable arguments.
  vc::TypeSizeWrapper MaxStructSize = vc::ByteSize * MaxCMABIByvalSize;
  SmallPtrSet<Argument *, 8> ArgsToTransform =
      vc::collectArgsToTransform(*F, MaxStructSize);

  if (ArgsToTransform.empty() && LI.empty())
    return 0;

  return TransformNode(*F, ArgsToTransform, LI);
}

// \brief Fix argument passing for kernels: i1 -> i8.
CallGraphNode *CMABI::TransformKernel(Function *F) {
  IGC_ASSERT(F->getReturnType()->isVoidTy());
  LLVMContext &Context = F->getContext();

  AttributeList AttrVec;
  const AttributeList &PAL = F->getAttributes();

  // First, determine the new argument list
  SmallVector<Type *, 8> ArgTys;
  unsigned ArgIndex = 0;
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
       ++I, ++ArgIndex) {
    Type *ArgTy = I->getType();
    // Change i1 to i8 and vxi1 to vxi8
    if (ArgTy->getScalarType()->isIntegerTy(1)) {
      Type *Ty = IntegerType::get(F->getContext(), 8);
      if (ArgTy->isVectorTy())
        ArgTys.push_back(IGCLLVM::FixedVectorType::get(
            Ty, dyn_cast<IGCLLVM::FixedVectorType>(ArgTy)->getNumElements()));
      else
        ArgTys.push_back(Ty);
    } else {
      // Unchanged argument
      AttributeSet attrs = IGCLLVM::getParamAttrs(PAL, ArgIndex);
      if (attrs.hasAttributes()) {
        IGCLLVM::AttrBuilder B{ Context, attrs };
        AttrVec = AttrVec.addParamAttributes(Context, ArgTys.size(), B);
      }
      ArgTys.push_back(I->getType());
    }
  }

  FunctionType *NFTy = FunctionType::get(F->getReturnType(), ArgTys, false);
  IGC_ASSERT_MESSAGE((NFTy != F->getFunctionType()),
    "type out of sync, expect bool arguments");

  // Add any function attributes.
  AttributeSet FnAttrs = IGCLLVM::getFnAttrs(PAL);
  if (FnAttrs.hasAttributes()) {
    IGCLLVM::AttrBuilder B(Context, FnAttrs);
    AttrVec = IGCLLVM::addAttributesAtIndex(AttrVec, Context, AttributeList::FunctionIndex, B);
  }

  // Create the new function body and insert it into the module.
  Function *NF = Function::Create(NFTy, F->getLinkage(), F->getName());

  LLVM_DEBUG(dbgs() << "\nCMABI: Transforming From:" << *F);
  vc::transferNameAndCCWithNewAttr(AttrVec, *F, *NF);
  F->getParent()->getFunctionList().insert(F->getIterator(), NF);
  vc::transferDISubprogram(*F, *NF);
  LLVM_DEBUG(dbgs() << "  --> To: " << *NF << "\n");

  // Since we have now created the new function, splice the body of the old
  // function right into the new function.
  NF->getBasicBlockList().splice(NF->begin(), F->getBasicBlockList());

  // Loop over the argument list, transferring uses of the old arguments over to
  // the new arguments, also transferring over the names as well.
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(),
                              I2 = NF->arg_begin();
       I != E; ++I, ++I2) {
    // For an unmodified argument, move the name and users over.
    if (!I->getType()->getScalarType()->isIntegerTy(1)) {
      I->replaceAllUsesWith(I2);
      I2->takeName(I);
    } else {
      Instruction *InsertPt = &*(NF->begin()->begin());
      Instruction *Conv = new TruncInst(I2, I->getType(), "tobool", InsertPt);
      I->replaceAllUsesWith(Conv);
      I2->takeName(I);
    }
  }

  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  CallGraphNode *NF_CGN = CG.getOrInsertFunction(NF);

  // Update the metadata entry.
  if (F->hasDLLExportStorageClass())
    NF->setDLLStorageClass(F->getDLLStorageClass());

  vc::replaceFunctionRefMD(*F, *NF);

  // Now that the old function is dead, delete it. If there is a dangling
  // reference to the CallgraphNode, just leave the dead function around.
  NF_CGN->stealCalledFunctionsFrom(CG[F]);
  CallGraphNode *CGN = CG[F];
  if (CGN->getNumReferences() == 0)
    delete CG.removeFunctionFromModule(CGN);
  else
    F->setLinkage(Function::ExternalLinkage);

  return NF_CGN;
}

// \brief Actually performs the transformation of the specified arguments, and
// returns the new function.
//
// Note this transformation does change the semantics as a C function, due to
// possible pointer aliasing. But it is allowed as a CM function.
//
// The pass-by-reference scheme is useful to copy-out values from the
// subprogram back to the caller. It also may be useful to convey large inputs
// to subprograms, as the amount of parameter conveying code will be reduced.
// There is a restriction imposed on arguments passed by reference in order to
// allow for an efficient CM implementation. Specifically the restriction is
// that for a subprogram that uses pass-by-reference, the behavior must be the
// same as if we use a copy-in/copy-out semantic to convey the
// pass-by-reference argument; otherwise the CM program is said to be erroneous
// and may produce incorrect results. Such errors are not caught by the
// compiler and it is up to the user to guarantee safety.
//
// The implication of the above stated restriction is that no pass-by-reference
// argument that is written to in a subprogram (either directly or transitively
// by means of a nested subprogram call pass-by-reference argument) may overlap
// with another pass-by-reference parameter or a global variable that is
// referenced in the subprogram; in addition no pass-by-reference subprogram
// argument that is referenced may overlap with a global variable that is
// written to in the subprogram.
//
CallGraphNode *CMABI::TransformNode(Function &OrigFunc,
                                    SmallPtrSet<Argument *, 8> &ArgsToTransform,
                                    LocalizationInfo &LI) {
  NumArgumentsTransformed += ArgsToTransform.size();
  vc::TransformedFuncInfo NewFuncInfo{OrigFunc, ArgsToTransform};
  NewFuncInfo.appendGlobals(LI.getGlobals());

  // Create the new function declaration and insert it into the module.
  Function *NewFunc = vc::createTransformedFuncDecl(OrigFunc, NewFuncInfo);

  // Get a new callgraph node for NF.
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  CallGraphNode *NewFuncCGN = CG.getOrInsertFunction(NewFunc);

  vc::FuncUsersUpdater{OrigFunc, *NewFunc, NewFuncInfo, *NewFuncCGN, CG}.run();
  vc::FuncBodyTransfer{OrigFunc, *NewFunc, NewFuncInfo}.run();

  // It turns out sometimes llvm will recycle function pointers which confuses
  // this pass. We delete its localization info and mark this function as
  // already visited.
  Info->GlobalInfo.erase(&OrigFunc);
  AlreadyVisited.insert(&OrigFunc);

  NewFuncCGN->stealCalledFunctionsFrom(CG[&OrigFunc]);

  // Now that the old function is dead, delete it. If there is a dangling
  // reference to the CallgraphNode, just leave the dead function around.
  CallGraphNode *CGN = CG[&OrigFunc];
  if (CGN->getNumReferences() == 0)
    delete CG.removeFunctionFromModule(CGN);
  else
    OrigFunc.setLinkage(Function::ExternalLinkage);

  return NewFuncCGN;
}

static void fillStackWithUsers(std::stack<User *> &Stack, User &CurUser) {
  for (User *Usr : CurUser.users())
    Stack.push(Usr);
}

// Traverse in depth through GV constant users to find instruction users.
// When instruction user is found, it is clear in which function GV is used.
void CMABIAnalysis::defineGVDirectUsers(GlobalVariable &GV) {
  std::stack<User *> Stack;
  Stack.push(&GV);
  while (!Stack.empty()) {
    auto *CurUser = Stack.top();
    Stack.pop();

    // Continue go in depth when a constant is met.
    if (isa<Constant>(CurUser)) {
      fillStackWithUsers(Stack, *CurUser);
      continue;
    }

    // We've got what we looked for.
    auto *Inst = cast<Instruction>(CurUser);
    addDirectGlobal(Inst->getFunction(), &GV);
  }
}

// For each function, compute the list of globals that need to be passed as
// copy-in and copy-out arguments.
void CMABIAnalysis::analyzeGlobals(CallGraph &CG) {
  Module &M = CG.getModule();

  // No global variables.
  if (M.global_empty())
    return;

  // FIXME: String constants must be localized too. Excluding them there
  //        to WA legacy printf implementation in CM FE (printf strings are
  //        not in constant addrspace in legacy printf).
  auto ToLocalize =
      make_filter_range(M.globals(), [](const GlobalVariable &GV) {
        return GV.getAddressSpace() == vc::AddrSpace::Private &&
               vc::isRealGlobalVariable(GV) && !vc::isConstantString(GV);
      });

  // Collect direct and indirect (GV is used in a called function)
  // uses of globals.
  for (GlobalVariable &GV : ToLocalize)
    defineGVDirectUsers(GV);
  for (const std::vector<CallGraphNode *> &SCCNodes :
       make_range(scc_begin(&CG), scc_end(&CG)))
    for (const CallGraphNode *Caller : SCCNodes)
      for (const IGCLLVM::CallGraphNode::CallRecord &Callee : *Caller) {
        Function *CalleeF = Callee.second->getFunction();
        if (CalleeF && !vc::isFixedSignatureFunc(*CalleeF))
          addIndirectGlobal(Caller->getFunction(), CalleeF);
      }
}

/***********************************************************************
 * diagnoseOverlappingArgs : attempt to diagnose overlapping by-ref args
 *
 * The CM language spec says you are not allowed a call with two by-ref args
 * that overlap. This is to give the compiler the freedom to implement with
 * copy-in copy-out semantics or with an address register.
 *
 * This function attempts to diagnose code that breaks this restriction. For
 * pointer args to the call, it attempts to track how values are loaded using
 * the pointer (assumed to be an alloca of the temporary used for copy-in
 * copy-out semantics), and how those values then get propagated through
 * wrregions and stores. If any vector element in a wrregion or store is found
 * that comes from more than one pointer arg, it is reported.
 *
 * This ignores variable index wrregions, and only traces through instructions
 * with the same debug location as the call, so does not work with -g0.
 */
void CMABI::diagnoseOverlappingArgs(CallInst *CI)
{
  LLVM_DEBUG(dbgs() << "diagnoseOverlappingArgs " << *CI << "\n");
  auto DL = CI->getDebugLoc();
  if (!DL)
    return;
  std::map<Value *, SmallVector<uint8_t, 16>> ValMap;
  SmallVector<Instruction *, 8> WorkList;
  std::set<Instruction *> InWorkList;
  std::set<std::pair<unsigned, unsigned>> Reported;
  // Using ArgIndex starting at 1 so we can reserve 0 to mean "element does not
  // come from any by-ref arg".
  for (unsigned ArgIndex = 1, NumArgs = IGCLLVM::getNumArgOperands(CI);
      ArgIndex <= NumArgs; ++ArgIndex) {
    Value *Arg = CI->getOperand(ArgIndex - 1);
    if (!Arg->getType()->isPointerTy())
      continue;
    LLVM_DEBUG(dbgs() << "arg " << ArgIndex << ": " << *Arg << "\n");
    // Got a pointer arg. Find its loads (with the same debug loc).
    for (auto ui = Arg->use_begin(), ue = Arg->use_end(); ui != ue; ++ui) {
      auto LI = dyn_cast<LoadInst>(ui->getUser());
      if (!LI || LI->getDebugLoc() != DL)
        continue;
      LLVM_DEBUG(dbgs() << "  " << *LI << "\n");
      // For a load, create a map entry that says that every vector element
      // comes from this arg.
      unsigned NumElements = 1;
      if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(LI->getType()))
        NumElements = VT->getNumElements();
      auto Entry = &ValMap[LI];
      Entry->resize(NumElements, ArgIndex);
      // Add its users (with the same debug location) to the work list.
      for (auto ui = LI->use_begin(), ue = LI->use_end(); ui != ue; ++ui) {
        auto Inst = cast<Instruction>(ui->getUser());
        if (Inst->getDebugLoc() == DL)
          if (InWorkList.insert(Inst).second)
            WorkList.push_back(Inst);
      }
    }
  }
  // Process the work list.
  while (!WorkList.empty()) {
    auto Inst = WorkList.back();
    WorkList.pop_back();
    InWorkList.erase(Inst);
    LLVM_DEBUG(dbgs() << "From worklist: " << *Inst << "\n");
    Value *Key = nullptr;
    SmallVector<uint8_t, 8> TempVector;
    SmallVectorImpl<uint8_t> *VectorToMerge = nullptr;
    if (auto SI = dyn_cast<StoreInst>(Inst)) {
      // Store: set the map entry using the store pointer as the key. It might
      // be an alloca of a local variable, or a global variable.
      // Strictly speaking this is not properly keeping track of what is being
      // merged using load-wrregion-store for a non-SROAd local variable or a
      // global variable. Instead it is just merging at the store itself, which
      // is good enough for our purposes.
      Key = SI->getPointerOperand();
      VectorToMerge = &ValMap[SI->getValueOperand()];
    } else if (auto BC = dyn_cast<BitCastInst>(Inst)) {
      // Bitcast: calculate the new map entry.
      Key = BC;
      uint64_t OutElementSize =
          BC->getType()->getScalarType()->getPrimitiveSizeInBits();
      uint64_t InElementSize = BC->getOperand(0)
                                   ->getType()
                                   ->getScalarType()
                                   ->getPrimitiveSizeInBits();
      int LogRatio = countTrailingZeros(OutElementSize, ZB_Undefined) -
                     countTrailingZeros(InElementSize, ZB_Undefined);
      auto OpndEntry = &ValMap[BC->getOperand(0)];
      if (!LogRatio)
        VectorToMerge = OpndEntry;
      else if (LogRatio > 0) {
        // Result element type is bigger than input element type, so there are
        // fewer result elements. Just use an arbitrarily chosen non-zero entry
        // of the N input elements to set the 1 result element.
        IGC_ASSERT(!(OpndEntry->size() & ((1U << LogRatio) - 1)));
        for (unsigned i = 0, e = OpndEntry->size(); i != e; i += 1U << LogRatio) {
          unsigned FoundArgIndex = 0;
          for (unsigned j = 0; j != 1U << LogRatio; ++j)
            FoundArgIndex = std::max(FoundArgIndex, (unsigned)(*OpndEntry)[i + j]);
          TempVector.push_back(FoundArgIndex);
        }
        VectorToMerge = &TempVector;
      } else {
        // Result element type is smaller than input element type, so there are
        // multiple result elements per input element.
        for (unsigned i = 0, e = OpndEntry->size(); i != e; ++i)
          for (unsigned j = 0; j != 1U << -LogRatio; ++j)
            TempVector.push_back((*OpndEntry)[i]);
        VectorToMerge = &TempVector;
      }
    } else if (auto CI = dyn_cast<CallInst>(Inst)) {
      if (auto CF = CI->getCalledFunction()) {
        switch (GenXIntrinsic::getGenXIntrinsicID(CF)) {
          default:
            break;
          case GenXIntrinsic::genx_wrregionf:
          case GenXIntrinsic::genx_wrregioni:
            // wrregion: As long as it is constant index, propagate the argument
            // indices into the appropriate elements of the result.
            if (auto IdxC = dyn_cast<Constant>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrIndexOperandNum))) {
              unsigned Idx = 0;
              if (!IdxC->isNullValue()) {
                auto IdxCI = dyn_cast<ConstantInt>(IdxC);
                if (!IdxCI) {
                  LLVM_DEBUG(dbgs() << "Ignoring variable index wrregion\n");
                  break;
                }
                Idx = IdxCI->getZExtValue();
              }
              Idx /= (CI->getType()->getScalarType()->getPrimitiveSizeInBits() / 8U);
              // First copy the "old value" input to the map entry.
              auto OpndEntry = &ValMap[CI->getOperand(
                    GenXIntrinsic::GenXRegion::OldValueOperandNum)];
              auto Entry = &ValMap[CI];
              Entry->clear();
              Entry->insert(Entry->begin(), OpndEntry->begin(), OpndEntry->end());
              // Then copy the "new value" elements according to the region.
              TempVector.resize(
                  dyn_cast<IGCLLVM::FixedVectorType>(CI->getType())->getNumElements(), 0);
              int VStride = cast<ConstantInt>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrVStrideOperandNum))->getSExtValue();
              unsigned Width = cast<ConstantInt>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrWidthOperandNum))->getZExtValue();
              IGC_ASSERT_MESSAGE((Width > 0), "Width of a region must be non-zero");
              int Stride = cast<ConstantInt>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrStrideOperandNum))->getSExtValue();
              OpndEntry = &ValMap[CI->getOperand(
                    GenXIntrinsic::GenXRegion::NewValueOperandNum)];
              unsigned NumElements = OpndEntry->size();
              if (!NumElements)
                break;
              for (unsigned RowIdx = Idx, Row = 0, Col = 0,
                    NumRows = NumElements / Width;; Idx += Stride, ++Col) {
                if (Col == Width) {
                  Col = 0;
                  if (++Row == NumRows)
                    break;
                  Idx = RowIdx += VStride;
                }
                TempVector[Idx] = (*OpndEntry)[Row * Width + Col];
              }
              VectorToMerge = &TempVector;
              Key = CI;
            }
            break;
        }
      }
    }
    if (!VectorToMerge)
      continue;
    auto Entry = &ValMap[Key];
    LLVM_DEBUG(dbgs() << "Merging :";
      for (unsigned i = 0; i != VectorToMerge->size(); ++i)
        dbgs() << " " << (unsigned)(*VectorToMerge)[i];
      dbgs() << "\ninto " << Key->getName() << ":";
      for (unsigned i = 0; i != Entry->size(); ++i)
        dbgs() << " " << (unsigned)(*Entry)[i];
      dbgs() << "\n");
    if (Entry->empty())
      Entry->insert(Entry->end(), VectorToMerge->begin(), VectorToMerge->end());
    else {
      IGC_ASSERT(VectorToMerge->size() == Entry->size());
      for (unsigned i = 0; i != VectorToMerge->size(); ++i) {
        unsigned ArgIdx1 = (*VectorToMerge)[i];
        unsigned ArgIdx2 = (*Entry)[i];
        if (ArgIdx1 && ArgIdx2 && ArgIdx1 != ArgIdx2) {
          LLVM_DEBUG(dbgs() << "By ref args overlap: args " << ArgIdx1 << " and " << ArgIdx2 << "\n");
          if (ArgIdx1 > ArgIdx2)
            std::swap(ArgIdx1, ArgIdx2);
          if (Reported.insert(std::pair<unsigned, unsigned>(ArgIdx1, ArgIdx2))
                .second) {
            // Not already reported.
            vc::fatal(Inst->getContext(), "CMABI",
                      "by reference arguments " + Twine(ArgIdx1) + " and " +
                          Twine(ArgIdx2) + " overlap",
                      CI);
          }
        }
        (*Entry)[i] = std::max((*Entry)[i], (*VectorToMerge)[i]);
      }
    }
    LLVM_DEBUG(dbgs() << "giving:";
      for (unsigned i = 0; i != Entry->size(); ++i)
        dbgs() << " " << (unsigned)(*Entry)[i];
      dbgs() << "\n");
    if (Key == Inst) {
      // Not the case that we have a store and we are using the pointer as
      // the key. In ther other cases that do a merge (bitcast and wrregion),
      // add users to the work list as long as they have the same debug loc.
      for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
        auto User = cast<Instruction>(ui->getUser());
        if (User->getDebugLoc() == DL)
          if (InWorkList.insert(Inst).second)
            WorkList.push_back(User);
      }
    }
  }
}

char CMABI::ID = 0;
INITIALIZE_PASS_BEGIN(CMABI, "cmabi", "Fix ABI issues for the genx backend", false, false)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_DEPENDENCY(CMABIAnalysis)
INITIALIZE_PASS_END(CMABI, "cmabi", "Fix ABI issues for the genx backend", false, false)

Pass *llvm::createCMABIPass() { return new CMABI(); }

namespace {

// A well-formed passing argument by reference pattern.
//
// (Alloca)
// %argref1 = alloca <8 x float>, align 32
//
// (CopyInRegion/CopyInStore)
// %rdr = tail call <8 x float> @llvm.genx.rdregionf(<960 x float> %m, i32 0, i32 8, i32 1, i16 0, i32 undef)
// call void @llvm.genx.vstore(<8 x float> %rdr, <8 x float>* %argref)
//
// (CopyOutRegion/CopyOutLoad)
// %ld = call <8 x float> @llvm.genx.vload(<8 x float>* %argref)
// %wr = call <960 x float> @llvm.genx.wrregionf(<960 x float> %m, <8 x float> %ld, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
//
struct ArgRefPattern {
  // Alloca of this reference argument.
  AllocaInst *Alloca;

  // The input value
  CallInst *CopyInRegion;
  CallInst *CopyInStore;

  // The output value
  CallInst *CopyOutLoad;
  CallInst *CopyOutRegion;

  // Load and store instructions on arg alloca.
  SmallVector<CallInst *, 8> VLoads;
  SmallVector<CallInst *, 8> VStores;

  explicit ArgRefPattern(AllocaInst *AI)
      : Alloca(AI), CopyInRegion(nullptr), CopyInStore(nullptr),
        CopyOutLoad(nullptr), CopyOutRegion(nullptr) {}

  // Match a copy-in and copy-out pattern. Return true on success.
  bool match(DominatorTree &DT, PostDominatorTree &PDT);
  void process(DominatorTree &DT);
};

struct CMLowerVLoadVStore : public FunctionPass {
  static char ID;
  CMLowerVLoadVStore() : FunctionPass(ID) {
    initializeCMLowerVLoadVStorePass(*PassRegistry::getPassRegistry());
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;

private:
  bool promoteAllocas(Function &F);
  bool lowerLoadStore(Function &F);
};

} // namespace

char CMLowerVLoadVStore::ID = 0;
INITIALIZE_PASS_BEGIN(CMLowerVLoadVStore, "CMLowerVLoadVStore",
                      "Lower CM reference vector loads and stores", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(CMLowerVLoadVStore, "CMLowerVLoadVStore",
                    "Lower CM reference vector loads and stores", false, false)


bool CMLowerVLoadVStore::runOnFunction(Function &F) {
  bool Changed = false;
  Changed |= promoteAllocas(F);
  Changed |= lowerLoadStore(F);
  return Changed;
}

// Lower remaining vector load/store intrinsic calls into normal load/store
// instructions.
bool CMLowerVLoadVStore::lowerLoadStore(Function &F) {
  auto M = F.getParent();
  DenseMap<AllocaInst*, GlobalVariable*> AllocaMap;
  // collect all the allocas that store the address of genx-volatile variable
  for (auto& G : M->getGlobalList()) {
    if (!G.hasAttribute("genx_volatile"))
      continue;
    std::vector<User*> WL;
    for (auto UI = G.user_begin(); UI != G.user_end();) {
      auto U = *UI++;
      WL.push_back(U);
    }

    while (!WL.empty()) {
      auto Inst = WL.back();
      WL.pop_back();
      if (auto CE = dyn_cast<ConstantExpr>(Inst)) {
        for (auto UI = CE->user_begin(); UI != CE->user_end();) {
          auto U = *UI++;
          WL.push_back(U);
        }
      }
      else if (auto CI = dyn_cast<CastInst>(Inst)) {
        for (auto UI = CI->user_begin(); UI != CI->user_end();) {
          auto U = *UI++;
          WL.push_back(U);
        }
      }
      else if (auto SI = dyn_cast<StoreInst>(Inst)) {
        auto Ptr = SI->getPointerOperand()->stripPointerCasts();
        if (auto PI = dyn_cast<AllocaInst>(Ptr)) {
          AllocaMap[PI] = &G;
        }
      }
    }
  }

  // lower all vload/vstore into normal load/store.
  std::vector<Instruction *> ToErase;
  for (Instruction &Inst : instructions(F)) {
    if (GenXIntrinsic::isVLoadStore(&Inst)) {
      auto *Ptr = Inst.getOperand(0);
      if (GenXIntrinsic::isVStore(&Inst))
        Ptr = Inst.getOperand(1);
      auto AS0 = cast<PointerType>(Ptr->getType())->getAddressSpace();
      Ptr = Ptr->stripPointerCasts();
      auto GV = dyn_cast<GlobalVariable>(Ptr);
      if (GV) {
        if (!GV->hasAttribute("genx_volatile"))
          GV = nullptr;
      }
      else if (auto LI = dyn_cast<LoadInst>(Ptr)) {
        auto PV = LI->getPointerOperand()->stripPointerCasts();
        if (auto PI = dyn_cast<AllocaInst>(PV)) {
          if (AllocaMap.find(PI) != AllocaMap.end()) {
            GV = AllocaMap[PI];
          }
        }
      }
      if (GV == nullptr) {
        // change to load/store
        IRBuilder<> Builder(&Inst);
        if (GenXIntrinsic::isVStore(&Inst))
          Builder.CreateStore(Inst.getOperand(0), Inst.getOperand(1));
        else {
          Value *Op0 = Inst.getOperand(0);
          auto LI = Builder.CreateLoad(Op0->getType()->getPointerElementType(),
                                       Op0, Inst.getName());
          LI->setDebugLoc(Inst.getDebugLoc());
          Inst.replaceAllUsesWith(LI);
        }
        ToErase.push_back(&Inst);
      }
      else {
        // change to vload/vstore that has the same address space as
        // the global-var in order to clean up unnecessary addr-cast.
        auto AS1 = GV->getType()->getAddressSpace();
        if (AS0 != AS1) {
          IRBuilder<> Builder(&Inst);
          if (GenXIntrinsic::isVStore(&Inst)) {
            auto PtrTy = cast<PointerType>(Inst.getOperand(1)->getType());
            PtrTy = PointerType::get(PtrTy->getElementType(), AS1);
            auto PtrCast = Builder.CreateAddrSpaceCast(Inst.getOperand(1), PtrTy);
            Type* Tys[] = { Inst.getOperand(0)->getType(),
                           PtrCast->getType() };
            Value* Args[] = { Inst.getOperand(0), PtrCast };
            Function* Fn = GenXIntrinsic::getGenXDeclaration(
              F.getParent(), GenXIntrinsic::genx_vstore, Tys);
            Builder.CreateCall(Fn, Args, Inst.getName());
          }
          else {
            auto PtrTy = cast<PointerType>(Inst.getOperand(0)->getType());
            PtrTy = PointerType::get(PtrTy->getElementType(), AS1);
            auto PtrCast = Builder.CreateAddrSpaceCast(Inst.getOperand(0), PtrTy);
            Type* Tys[] = { Inst.getType(), PtrCast->getType() };
            Function* Fn = GenXIntrinsic::getGenXDeclaration(
              F.getParent(), GenXIntrinsic::genx_vload, Tys);
            Value* VLoad = Builder.CreateCall(Fn, PtrCast, Inst.getName());
            Inst.replaceAllUsesWith(VLoad);
          }
          ToErase.push_back(&Inst);
        }
      }
    }
  }

  for (auto Inst : ToErase) {
    Inst->eraseFromParent();
  }

  return !ToErase.empty();
}

static bool isBitCastForLifetimeMarker(Value *V) {
  if (!V || !isa<BitCastInst>(V))
    return false;
  for (auto U : V->users()) {
    unsigned IntrinsicID = GenXIntrinsic::getAnyIntrinsicID(U);
    if (IntrinsicID != Intrinsic::lifetime_start &&
        IntrinsicID != Intrinsic::lifetime_end)
      return false;
  }
  return true;
}

// Check whether two values are bitwise identical.
static bool isBitwiseIdentical(Value *V1, Value *V2) {
  IGC_ASSERT_MESSAGE(V1, "null value");
  IGC_ASSERT_MESSAGE(V2, "null value");
  if (V1 == V2)
    return true;
  if (BitCastInst *BI = dyn_cast<BitCastInst>(V1))
    V1 = BI->getOperand(0);
  if (BitCastInst *BI = dyn_cast<BitCastInst>(V2))
    V2 = BI->getOperand(0);

  // Special case arises from vload/vstore.
  if (GenXIntrinsic::isVLoad(V1) && GenXIntrinsic::isVLoad(V2)) {
    auto L1 = cast<CallInst>(V1);
    auto L2 = cast<CallInst>(V2);
    // Check if loading from the same location.
    if (L1->getOperand(0) != L2->getOperand(0))
      return false;

    // Check if this pointer is local and only used in vload/vstore.
    Value *Addr = L1->getOperand(0);
    if (!isa<AllocaInst>(Addr))
      return false;
    for (auto UI : Addr->users()) {
      if (isa<BitCastInst>(UI)) {
        for (auto U : UI->users()) {
          unsigned IntrinsicID = GenXIntrinsic::getAnyIntrinsicID(U);
          if (IntrinsicID != Intrinsic::lifetime_start &&
              IntrinsicID != Intrinsic::lifetime_end)
            return false;
        }
      } else {
        if (!GenXIntrinsic::isVLoadStore(UI))
          return false;
      }
    }

    // Check if there is no store to the same location in between.
    if (L1->getParent() != L2->getParent())
      return false;
    BasicBlock::iterator I = L1->getParent()->begin();
    for (; &*I != L1 && &*I != L2; ++I)
      /*empty*/;
    IGC_ASSERT(&*I == L1 || &*I == L2);
    auto IEnd = (&*I == L1) ? L2->getIterator() : L1->getIterator();
    for (; I != IEnd; ++I) {
      Instruction *Inst = &*I;
      if (GenXIntrinsic::isVStore(Inst) && Inst->getOperand(1) == Addr)
        return false;
    }

    // OK.
    return true;
  }

  // Cannot prove.
  return false;
}

bool ArgRefPattern::match(DominatorTree &DT, PostDominatorTree &PDT) {
  IGC_ASSERT(Alloca);
  if (Alloca->use_empty())
    return false;

  // check if all users are load/store.
  SmallVector<CallInst *, 8> Loads;
  SmallVector<CallInst *, 8> Stores;
  for (auto U : Alloca->users())
    if (GenXIntrinsic::isVLoad(U))
      Loads.push_back(cast<CallInst>(U));
    else if (GenXIntrinsic::isVStore(U))
      Stores.push_back(cast<CallInst>(U));
    else if (isBitCastForLifetimeMarker(U))
      continue;
    else
      return false;

  if (Loads.empty() || Stores.empty())
    return false;

  // find a unique store that dominates all other users if exists.
  auto Cmp = [&](CallInst *L, CallInst *R) { return DT.dominates(L, R); };
  CopyInStore = *std::min_element(Stores.begin(), Stores.end(), Cmp);
  CopyInRegion = dyn_cast<CallInst>(CopyInStore->getArgOperand(0));
  if (!CopyInRegion || !CopyInRegion->hasOneUse() || !GenXIntrinsic::isRdRegion(CopyInRegion))
    return false;

  for (auto SI : Stores)
    if (SI != CopyInStore && !Cmp(CopyInStore, SI))
      return false;
  for (auto LI : Loads)
    if (LI != CopyInStore && !Cmp(CopyInStore, LI))
      return false;

  // find a unique load that post-dominates all other users if exists.
  auto PostCmp = [&](CallInst *L, CallInst *R) {
      BasicBlock *LBB = L->getParent();
      BasicBlock *RBB = R->getParent();
      if (LBB != RBB)
          return PDT.dominates(LBB, RBB);

      // Loop through the basic block until we find L or R.
      BasicBlock::const_iterator I = LBB->begin();
      for (; &*I != L && &*I != R; ++I)
          /*empty*/;

      return &*I == R;
  };
  CopyOutLoad = *std::min_element(Loads.begin(), Loads.end(), PostCmp);

  // Expect copy-out load has one or zero use. It is possible there
  // is no use as the region becomes dead after this subroutine call.
  //
  if (!CopyOutLoad->use_empty()) {
    if (!CopyOutLoad->hasOneUse())
      return false;
    CopyOutRegion = dyn_cast<CallInst>(CopyOutLoad->user_back());
    if (!GenXIntrinsic::isWrRegion(CopyOutRegion))
      return false;
  }

  for (auto SI : Stores)
    if (SI != CopyOutLoad && !PostCmp(CopyOutLoad, SI))
      return false;
  for (auto LI : Loads)
    if (LI != CopyOutLoad && !PostCmp(CopyOutLoad, LI))
      return false;

  // Ensure read-in and write-out to the same region. It is possible that region
  // collasping does not simplify region accesses completely.
  // Probably we should use an assertion statement on region descriptors.
  if (CopyOutRegion &&
      !isBitwiseIdentical(CopyInRegion->getOperand(0),
                          CopyOutRegion->getOperand(0)))
    return false;

  // It should be OK to rewrite all loads and stores into the argref.
  VLoads.swap(Loads);
  VStores.swap(Stores);
  return true;
}

void ArgRefPattern::process(DominatorTree &DT) {
  // 'Spill' the base region into memory during rewriting.
  IRBuilder<> Builder(Alloca);
  Function *RdFn = CopyInRegion->getCalledFunction();
  IGC_ASSERT(RdFn);
  Type *BaseAllocaTy = RdFn->getFunctionType()->getParamType(0);
  AllocaInst *BaseAlloca = Builder.CreateAlloca(BaseAllocaTy, nullptr,
                                                Alloca->getName() + ".refprom");

  Builder.SetInsertPoint(CopyInRegion);
  Builder.CreateStore(CopyInRegion->getArgOperand(0), BaseAlloca);

  if (CopyOutRegion) {
    Builder.SetInsertPoint(CopyOutRegion);
    CopyOutRegion->setArgOperand(
        0, Builder.CreateLoad(BaseAlloca->getType()->getPointerElementType(),
                              BaseAlloca));
  }

  // Rewrite all stores.
  for (auto ST : VStores) {
    Builder.SetInsertPoint(ST);
    Value *OldVal = Builder.CreateLoad(
        BaseAlloca->getType()->getPointerElementType(), BaseAlloca);
    // Always use copy-in region arguments as copy-out region
    // arguments do not dominate this store.
    auto M = ST->getParent()->getParent()->getParent();
    Value *Args[] = {OldVal,
                     ST->getArgOperand(0),
                     CopyInRegion->getArgOperand(1), // vstride
                     CopyInRegion->getArgOperand(2), // width
                     CopyInRegion->getArgOperand(3), // hstride
                     CopyInRegion->getArgOperand(4), // offset
                     CopyInRegion->getArgOperand(5), // parent width
                     ConstantInt::getTrue(Type::getInt1Ty(M->getContext()))};
    auto ID = OldVal->getType()->isFPOrFPVectorTy() ? GenXIntrinsic::genx_wrregionf
                                                    : GenXIntrinsic::genx_wrregioni;
    Type *Tys[] = {Args[0]->getType(), Args[1]->getType(), Args[5]->getType(),
                   Args[7]->getType()};
    auto WrFn = GenXIntrinsic::getGenXDeclaration(M, ID, Tys);
    Value *NewVal = Builder.CreateCall(WrFn, Args);
    Builder.CreateStore(NewVal, BaseAlloca);
    ST->eraseFromParent();
  }

  // Rewrite all loads
  for (auto LI : VLoads) {
    if (LI->use_empty())
      continue;

    Builder.SetInsertPoint(LI);
    Value *SrcVal = Builder.CreateLoad(
        BaseAlloca->getType()->getPointerElementType(), BaseAlloca);
    SmallVector<Value *, 8> Args(IGCLLVM::args(CopyInRegion));
    Args[0] = SrcVal;
    Value *Val = Builder.CreateCall(RdFn, Args);
    LI->replaceAllUsesWith(Val);
    LI->eraseFromParent();
  }
  // BaseAlloca created manually, w/o RAUW, need fix debug-info for it
  llvm::replaceAllDbgUsesWith(*Alloca, *BaseAlloca, *BaseAlloca, DT);
}

// Allocas that are used in reference argument passing may be promoted into the
// base region.
bool CMLowerVLoadVStore::promoteAllocas(Function &F) {
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  auto &PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
  bool Modified = false;

  SmallVector<AllocaInst *, 8> Allocas;
  for (auto &Inst : F.front().getInstList()) {
    if (auto AI = dyn_cast<AllocaInst>(&Inst))
      Allocas.push_back(AI);
  }

  for (auto AI : Allocas) {
    ArgRefPattern ArgRef(AI);
    if (ArgRef.match(DT, PDT)) {
      ArgRef.process(DT);
      Modified = true;
    }
  }

  return Modified;
}

Pass *llvm::createCMLowerVLoadVStorePass() { return new CMLowerVLoadVStore; }
