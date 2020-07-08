/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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

#define DEBUG_TYPE "cmabi"

#include "llvmWrapper/Support/Alignment.h"

#include "llvm/ADT/DenseMap.h"
#include "vc/GenXOpts/GenXOpts.h"
#include "vc/GenXOpts/Utils/GenXSTLExtras.h"
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
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include "llvmWrapper/Analysis/CallGraph.h"

#include <iterator>

using namespace llvm;

using LocalizationLimitT = int32_t;
static constexpr auto LocalizeAll = std::numeric_limits<LocalizationLimitT>::max();
static cl::opt<LocalizationLimitT>
    LocalizationLimit("cm-abi-issues-localization-limit",
                   cl::desc("maximum size (in bytes) used to localize global variables"),
                   cl::init(LocalizeAll));

STATISTIC(NumArgumentsTransformed, "Number of pointer arguments transformed");
STATISTIC(NumArgumentsDead       , "Number of dead pointer args eliminated");

namespace llvm {
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

  void setArgIndex(GlobalVariable *GV, unsigned ArgIndex) {
    assert(!IndexMap.count(GV));
    IndexMap[GV] = ArgIndex;
  }
  unsigned getArgIndex(GlobalVariable *GV) const {
    assert(IndexMap.count(GV));
    return IndexMap.lookup(GV);
  }

private:
  // \brief The function being analyzed.
  Function *Fn;

  // \brief Global variables that are used directly or indirectly.
  GlobalSetTy Globals;

  // This map keeps track of argument index for a global variable.
  SmallDenseMap<GlobalVariable *, unsigned> IndexMap;
};

// Diagnostic information for error/warning for overlapping arg
class DiagnosticInfoOverlappingArgs : public DiagnosticInfo {
private:
  std::string Description;
  StringRef Filename;
  unsigned Line;
  unsigned Col;
  static int KindID;
  static int getKindID() {
    if (KindID == 0)
      KindID = llvm::getNextAvailablePluginDiagnosticKind();
    return KindID;
  }
public:
  // Initialize from an Instruction and an Argument.
  DiagnosticInfoOverlappingArgs(Instruction *Inst,
      const Twine &Desc, DiagnosticSeverity Severity = DS_Error);
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};
int DiagnosticInfoOverlappingArgs::KindID = 0;



struct CMABI : public CallGraphSCCPass {
  static char ID;

  CMABI() : CallGraphSCCPass(ID) {
    initializeCMABIPass(*PassRegistry::getPassRegistry());
  }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    CallGraphSCCPass::getAnalysisUsage(AU);
  }

  virtual bool runOnSCC(CallGraphSCC &SCC);

  virtual bool doInitialization(CallGraph &CG);
  virtual bool doFinalization(CallGraph &CG);

private:

  CallGraphNode *ProcessNode(CallGraphNode *CGN);

  // Fix argument passing for kernels.
  CallGraphNode *TransformKernel(Function *F);

  // Major work is done in this method.
  CallGraphNode *TransformNode(Function *F,
                               SmallPtrSet<Argument *, 8> &ArgsToTransform,
                               LocalizationInfo &LI);

  // \brief Create allocas for globals and replace their uses.
  void LocalizeGlobals(LocalizationInfo &LI);

  // \brief Compute the localized global variables for each function.
  void AnalyzeGlobals(CallGraph &CG);

  // \brief Returns the localization info associated to a function.
  LocalizationInfo &getLocalizationInfo(Function *F) {
    if (!GlobalInfo.count(F)) {
      LocalizationInfo *LI = new LocalizationInfo(F);
      LocalizationInfoObjs.push_back(LI);
      GlobalInfo[F] = LI;
      return *LI;
    }
    return *GlobalInfo[F];
  }

  void addDirectGlobal(Function *F, GlobalVariable *GV) {
    getLocalizationInfo(F).addGlobal(GV);
  }

  // \brief Add all globals from callee to caller.
  void addIndirectGlobal(Function *F, Function *Callee) {
    getLocalizationInfo(F).addGlobals(getLocalizationInfo(Callee));
  }

  // Return true if pointer type argument appears in a
  // store instruction. This helps decide whether it is safe
  // to convert ptr arg to byvalue arg without adding
  // a return value
  bool IsPtrArgModified(Value * Arg);
  // Return true if pointer type arugment is only used to
  // load or store a simple value. This helps decide whehter
  // it is safe to convert ptr arg to by-value arg or
  // simple-value copy-in-copy-out.
  bool OnlyUsedBySimpleValueLoadStore(Value *Arg);

  // \brief Diagnose illegal overlapping by-ref args.
  void diagnoseOverlappingArgs(CallInst *CI);

  // This map captures all global variables to be localized.
  SmallDenseMap<Function *, LocalizationInfo *> GlobalInfo;

  // Kernels in the module being processed.
  SmallPtrSet<Function *, 8> Kernels;

  // Already visited functions.
  SmallPtrSet<Function *, 8> AlreadyVisited;

  // LocalizationInfo objects created.
  SmallVector<LocalizationInfo *, 8> LocalizationInfoObjs;
};

} // namespace

// Currently weight of the global defines by its size
static int calcGVWeight(const GlobalVariable &GV, const DataLayout &DL) {
  return DL.getTypeAllocSize(GV.getValueType());
}

/* selectGlobalsToLocalize - chooses which globals to localize
 * Returns vector of pointers to such globals.
 *
 * Algorithm: exclude globals that definitely should not be localized
 * sort globals by weight, choose first smallest ones, sum of which is under \p
 * Bound
 *
 * \p Globals - range of globals to choose from
 * \p Bound - bound not to overcome
 * \p ExcludePred - functor : GVRef -> bool, true if global should not be
 * localized \p WeightCalculator - functor : GVRef -> decltype(Bound), returns
 * weight of global
 */
template <typename ForwardRange, typename ExcludePredT, typename T,
          typename WeightCalculatorT>
auto selectGlobalsToLocalize(ForwardRange Globals, T Bound,
                             ExcludePredT ExcludePred,
                             WeightCalculatorT WeightCalculator)
    -> std::vector<genx::ranges::range_pointer_t<ForwardRange>> {
  assert(Bound >= 0 && "bound must be nonnegative");
  using GVPtr = genx::ranges::range_pointer_t<ForwardRange>;
  using GVRef = genx::ranges::range_reference_t<ForwardRange>;
  if (Bound == 0)
    return std::vector<GVPtr>();

  // filter out those, that we must exclude
  auto Unexcluded = make_filter_range(
      Globals, [ExcludePred](GVRef GV) { return !ExcludePred(GV); });
  using GVWithWeightT = std::pair<GVPtr, int>;

  if (Bound == LocalizeAll) {
    std::vector<GVPtr> ToLocalize;
    transform(Unexcluded, std::back_inserter(ToLocalize),
              [](GVRef GV) { return &GV; });
    return ToLocalize;
  }

  std::vector<GVWithWeightT> ToLocalizeWithWeight;
  transform(Unexcluded, std::back_inserter(ToLocalizeWithWeight),
            [WeightCalculator](GVRef GV) {
              return std::make_pair(&GV, WeightCalculator(GV));
            });

  // sort globals by weight
  std::sort(ToLocalizeWithWeight.begin(), ToLocalizeWithWeight.end(),
            [](GVWithWeightT LHS, GVWithWeightT RHS) {
              return LHS.second < RHS.second;
            });

  // filter max number of lightest ones, which weight sum is under the bound
  auto FirstNotToLocalize = genx::upper_partial_sum_bound(
      ToLocalizeWithWeight.begin(), ToLocalizeWithWeight.end(), Bound,
      [](decltype(Bound) Base, GVWithWeightT Inc) {
        return Base + Inc.second;
      });

  // collect them back to ToLocalize
  std::vector<GVPtr> ToLocalize;
  ToLocalize.reserve(FirstNotToLocalize - ToLocalizeWithWeight.begin());
  std::transform(ToLocalizeWithWeight.begin(), FirstNotToLocalize,
                 std::back_inserter(ToLocalize),
                 [](GVWithWeightT GV) { return GV.first; });

  return ToLocalize;
}

bool CMABI::doInitialization(CallGraph &CG) {
  // Analyze global variable usages and for each function attaches global
  // variables to be copy-in and copy-out.
  AnalyzeGlobals(CG);

  auto getValue = [](Metadata *M) -> Value * {
    if (auto VM = dyn_cast<ValueAsMetadata>(M))
      return VM->getValue();
    return nullptr;
  };

  // Collect all CM kernels from named metadata.
  if (NamedMDNode *Named =
          CG.getModule().getNamedMetadata(genx::FunctionMD::GenXKernels)) {
    assert(Named);
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

bool CMABI::doFinalization(CallGraph &CG) {
  bool Changed = false;
  for (Module::global_iterator I = CG.getModule().global_begin();
       I != CG.getModule().global_end();
       /*empty*/) {
    GlobalVariable *GV = &*I++;
    if (GV->use_empty()) {
      GV->eraseFromParent();
      Changed = true;
    }
  }

  for (LocalizationInfo *Obj : LocalizationInfoObjs)
    delete Obj;

  return Changed;
}

bool CMABI::runOnSCC(CallGraphSCC &SCC) {
  bool Changed = false, LocalChange;

  // Diagnose overlapping by-ref args.
  for (auto i = SCC.begin(), e = SCC.end(); i != e; ++i) {
    Function *F = (*i)->getFunction();
    if (!F || F->empty())
      continue;
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui) {
      auto CI = dyn_cast<CallInst>(ui->getUser());
      if (CI && CI->getNumArgOperands() == ui->getOperandNo())
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

// Sometimes we can get phi with GEP (or maybe some other inst) as an argument.
// While GEP's arguments are constants, its OK as GEP is a constant to.
// But when we replace constants with lokals, GEP becomes a normal instruction,
// a normal instruction, that is placed before phi - wrong IR, we need to fix
// it. Here it is fixed.
static void fixPhiUseIssue(Instruction *Inst) {
  auto PhiUse = cast<PHINode>(Inst->use_begin()->getUser());
  auto InstOpNoInPhi = Inst->use_begin()->getOperandNo();
  assert(Inst->getParent() == PhiUse->getParent());
  Inst->removeFromParent();
  Inst->insertBefore(PhiUse->getIncomingBlock(InstOpNoInPhi)->getTerminator());
}

// Replace uses of global variables with the corresponding allocas with a
// specified function.
//
// Returns vector of instructions with phi use, that should be later fixed.
static std::vector<Instruction *>
replaceUsesWithinFunction(SmallDenseMap<Value *, Value *> &GlobalsToReplace,
                          Function *F) {
  std::vector<Instruction *> PhiUseIssueInsts;
  for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    Instruction *Inst = &*I;
    for (unsigned i = 0, e = Inst->getNumOperands(); i < e; ++i) {
      auto Iter = GlobalsToReplace.find(Inst->getOperand(i));
      if (Iter != GlobalsToReplace.end())
        Inst->setOperand(i, Iter->second);
    }
    if (Inst->getNumUses() == 1) {
      auto PhiUse = dyn_cast<PHINode>(Inst->use_begin()->getUser());
      if (PhiUse && Inst->getParent() == PhiUse->getParent()) {
        PhiUseIssueInsts.push_back(Inst);
      }
    }
  }
  return PhiUseIssueInsts;
}

// \brief Create allocas for globals directly used in this kernel and
// replace all uses.
void CMABI::LocalizeGlobals(LocalizationInfo &LI) {
  const LocalizationInfo::GlobalSetTy &Globals = LI.getGlobals();
  typedef LocalizationInfo::GlobalSetTy::const_iterator IteratorTy;

  SmallDenseMap<Value *, Value *> GlobalsToReplace;
  Function *Fn = LI.getFunction();
  for (IteratorTy I = Globals.begin(), E = Globals.end(); I != E; ++I) {
    GlobalVariable *GV = (*I);
    LLVM_DEBUG(dbgs() << "Localizing global: " << *GV);

    Instruction &FirstI = *Fn->getEntryBlock().begin();
    Type *ElemTy = GV->getType()->getElementType();
    AllocaInst *Alloca = new AllocaInst(ElemTy, 0 /*AddressSpace*/,
                                        GV->getName() + ".local", &FirstI);
    Alloca->setAlignment(MaybeAlign(GV->getAlignment()));
    if (!isa<UndefValue>(GV->getInitializer()))
      new StoreInst(GV->getInitializer(), Alloca, &FirstI);

    GlobalsToReplace.insert(std::make_pair(GV, Alloca));
  }

  // Replaces all globals uses within this function.
  auto PhiUseIssueInsts = replaceUsesWithinFunction(GlobalsToReplace, Fn);

  for (auto InstWithPhiUse : PhiUseIssueInsts) {
    fixPhiUseIssue(InstWithPhiUse);
  }
}

CallGraphNode *CMABI::ProcessNode(CallGraphNode *CGN) {
  Function *F = CGN->getFunction();

  // nothing to do for declarations or already visited functions.
  if (!F || F->isDeclaration() || AlreadyVisited.count(F))
    return 0;

  // Variables to be localized.
  LocalizationInfo &LI = getLocalizationInfo(F);

  // This is a kernel.
  if (Kernels.count(F)) {
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

  // Non-kernels, only transforms module locals.
  if (!F->hasLocalLinkage())
    return 0;

  SmallVector<Argument*, 16> PointerArgs;
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
    if (I->getType()->isPointerTy())
      PointerArgs.push_back(I);

  // Check if there is any pointer arguments or globals to localize.
  if (PointerArgs.empty() && LI.empty())
    return 0;

  // Check transformable arguments.
  SmallPtrSet<Argument*, 8> ArgsToTransform;
  for (unsigned i = 0, e = PointerArgs.size(); i != e; ++i) {
    Argument *PtrArg = PointerArgs[i];
    Type *ArgTy = cast<PointerType>(PtrArg->getType())->getElementType();
    // Only transform to simple types.
    if ((ArgTy->isVectorTy() || OnlyUsedBySimpleValueLoadStore(PtrArg)) &&
        (ArgTy->isIntOrIntVectorTy() || ArgTy->isFPOrFPVectorTy()))
      ArgsToTransform.insert(PtrArg);
  }

  if (ArgsToTransform.empty() && LI.empty())
    return 0;

  return TransformNode(F, ArgsToTransform, LI);
}

// check for typical inst sequences passing arg as a base
// of store-like intrinsics
static bool checkSinkToMemIntrinsic(Instruction *Inst) {
  auto *CI = dyn_cast<CallInst>(Inst);
  if (CI && (GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
                 GenXIntrinsic::genx_svm_scatter ||
             GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
                 GenXIntrinsic::genx_scatter_scaled))
    return true;
  for (auto *U : Inst->users()) {
    if (isa<InsertElementInst>(U) || isa<ShuffleVectorInst>(U) ||
        isa<BinaryOperator>(U) || isa<CallInst>(U))
      return checkSinkToMemIntrinsic(cast<Instruction>(U));
  }
  return false;
}

bool CMABI::IsPtrArgModified(Value *Arg) {
  // Arg is a ptr to a vector type. If data is written using a
  // store, then return true. This means copy-in/copy-out are
  // needed as caller may use the updated value. If no data is
  // ever stored in Arg then return false. It is safe to
  // convert the parameter to pass-by-value in GRF.
  // This is a recursive function.
  for (const auto &U : Arg->users()) {
    if (auto *I = dyn_cast<Instruction>(U)) {
      if (isa<StoreInst>(U))
        return true;
      else if (isa<AddrSpaceCastInst>(U) || isa<GetElementPtrInst>(U))
        return IsPtrArgModified(U);
      else if (isa<PtrToIntInst>(U))
        return checkSinkToMemIntrinsic(I);
    }
  }
  return false;
}

bool CMABI::OnlyUsedBySimpleValueLoadStore(Value *Arg) {
  for (const auto &U : Arg->users()) {
    if (auto I = dyn_cast<Instruction>(U)) {
      if (auto LI = dyn_cast<LoadInst>(U)) {
        if (Arg != LI->getPointerOperand())
          return false;
      }
      if (auto SI = dyn_cast<LoadInst>(U)) {
        if (Arg != SI->getPointerOperand())
          return false;
      }
      else if (auto GEP = dyn_cast<GetElementPtrInst>(U)) {
        if (Arg != GEP->getPointerOperand())
          return false;
        else if (!GEP->hasAllZeroIndices())
          return false;
        if (!OnlyUsedBySimpleValueLoadStore(U))
          return false;
      }
      else if (isa<AddrSpaceCastInst>(U) || isa<PtrToIntInst>(U)) {
        if (!OnlyUsedBySimpleValueLoadStore(U))
          return false;
      }
      else if (auto CI = dyn_cast<CallInst>(U)) {
        auto Callee = CI->getCalledFunction();
        if (Callee && !Callee->isIntrinsic()) {
          //if (GenXIntrinsic::isAnyNonTrivialIntrinsic(Inst))
          return false;
        }
      }
      else
        return false;
    }
    else
      return false;
  }
  return true;
}

// \brief Fix argument passing for kernels: i1 -> i8.
CallGraphNode *CMABI::TransformKernel(Function *F) {
  assert(F->getReturnType()->isVoidTy());
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
        ArgTys.push_back(VectorType::get(Ty, ArgTy->getVectorNumElements()));
      else
        ArgTys.push_back(Ty);
    } else {
      // Unchanged argument
      AttributeSet attrs = PAL.getParamAttributes(ArgIndex);
      if (attrs.hasAttributes()) {
        AttrBuilder B(attrs);
        AttrVec = AttrVec.addParamAttributes(Context, ArgTys.size(), B);
      }
      ArgTys.push_back(I->getType());
    }
  }

  FunctionType *NFTy = FunctionType::get(F->getReturnType(), ArgTys, false);
  assert((NFTy != F->getFunctionType()) &&
         "type out of sync, expect bool arguments");

  // Add any function attributes.
  AttributeSet FnAttrs = PAL.getFnAttributes();
  if (FnAttrs.hasAttributes()) {
    AttrBuilder B(FnAttrs);
    AttrVec = AttrVec.addAttributes(Context, AttributeList::FunctionIndex, B);
  }

  // Create the new function body and insert it into the module.
  Function *NF = Function::Create(NFTy, F->getLinkage(), F->getName());
  NF->setAttributes(AttrVec);
  LLVM_DEBUG(dbgs() << "CMABI:  Transforming to:" << *NF << "\n" << "From: " << *F);
  F->getParent()->getFunctionList().insert(F->getIterator(), NF);
  NF->takeName(F);
  NF->setSubprogram(F->getSubprogram()); // tranfer debug-info
  NF->setCallingConv(F->getCallingConv());

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

  auto getValue = [](Metadata *M) -> Value * {
    if (auto VM = dyn_cast<ValueAsMetadata>(M))
      return VM->getValue();
    return nullptr;
  };

  // Scan the CM kernel metadata and replace with NF.
  if (NamedMDNode *Named =
          CG.getModule().getNamedMetadata(genx::FunctionMD::GenXKernels)) {
    for (unsigned I = 0, E = Named->getNumOperands(); I != E; ++I) {
      MDNode *Node = Named->getOperand(I);
      if (F == dyn_cast_or_null<Function>(getValue(Node->getOperand(0))))
        Node->replaceOperandWith(genx::KernelMDOp::FunctionRef, ValueAsMetadata::get(NF));
    }
  }

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
CallGraphNode *CMABI::TransformNode(Function *F,
                                    SmallPtrSet<Argument *, 8> &ArgsToTransform,
                                    LocalizationInfo &LI) {
  // Computing a new prototype for the function. E.g.
  //
  // i32 @foo(i32, <8 x i32>*) becomes {i32, <8 x i32>} @bar(i32, <8 x i32>)
  //
  FunctionType *FTy = F->getFunctionType();
  SmallVector<Type *, 8> RetTys;
  if (!FTy->getReturnType()->isVoidTy())
    RetTys.push_back(FTy->getReturnType());

  // Keep track of parameter attributes for the arguments that we are *not*
  // transforming. For the ones we do transform, parameter attributes are lost.
  AttributeList AttrVec;
  const AttributeList &PAL = F->getAttributes();
  LLVMContext &Context = F->getContext();

  // First, determine the new argument list
  SmallVector<Type *, 8> Params;
  SmallPtrSet<Argument*, 8> CopyInOutNeeded;
  unsigned ArgIndex = 0;
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
       ++I, ++ArgIndex) {
    if (!ArgsToTransform.count(I)) {
      // Unchanged argument
      AttributeSet attrs = PAL.getParamAttributes(ArgIndex);
      if (attrs.hasAttributes()) {
        AttrBuilder B(attrs);
        AttrVec = AttrVec.addParamAttributes(Context, Params.size(), B);
      }
      Params.push_back(I->getType());
    } else if (I->use_empty()) {
      // Delete unused arguments
      ++NumArgumentsDead;
    } else {
      // Use the element type as the new argument type.
      Params.push_back(I->getType()->getPointerElementType());

      if (IsPtrArgModified(I)) {
        CopyInOutNeeded.insert(I);
        RetTys.push_back(I->getType()->getPointerElementType());
      }

      ++NumArgumentsTransformed;
    }
  }

  typedef LocalizationInfo::GlobalSetTy::iterator IteratorTy;
  for (IteratorTy I = LI.getGlobals().begin(), E = LI.getGlobals().end();
       I != E; ++I) {
    GlobalVariable *GV = *I;
    // Store the index information of this global variable.
    LI.setArgIndex(GV, Params.size());

    Type *PointeeTy = GV->getType()->getPointerElementType();
    Params.push_back(PointeeTy);
    RetTys.push_back(PointeeTy);
  }

  // Add any function attributes.
  AttributeSet FnAttrs = PAL.getFnAttributes();
  if (FnAttrs.hasAttributes()) {
    AttrBuilder B(FnAttrs);
    AttrVec = AttrVec.addAttributes(Context, AttributeList::FunctionIndex, B);
  }

  // Construct the new function type using the new arguments.
  llvm::Type *RetTy = StructType::get(Context, RetTys);
  FunctionType *NFTy = FunctionType::get(RetTy, Params, FTy->isVarArg());

  // Create the new function body and insert it into the module.
  Function *NF = Function::Create(NFTy, F->getLinkage(), F->getName());
  NF->setAttributes(AttrVec);
  LLVM_DEBUG(dbgs() << "CMABI:  Transforming to:" << *NF << "\n" << "From: " << *F);
  F->getParent()->getFunctionList().insert(F->getIterator(), NF);
  NF->takeName(F);
  NF->setCallingConv(F->getCallingConv());

  // Get a new callgraph node for NF.
  CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  CallGraphNode *NF_CGN = CG.getOrInsertFunction(NF);

  std::vector<llvm::User*> DirectUsers;

  for (auto U: F->users()) {
    if (isa<CallInst>(U))
      DirectUsers.push_back(U);
  }

  // Loop over all of the callers of the function, transforming the call sites
  // to pass in the loaded pointers.
  for (auto U: DirectUsers) {
    CallSite CS(U);
    assert(CS.getCalledFunction() == F);
    Instruction *Call = CS.getInstruction();
    const AttributeList &CallPAL = CS.getAttributes();

    SmallVector<Value*, 16> Args;
    AttributeList NewAttrVec;

    // Loop over the operands, inserting loads in the caller.
    CallSite::arg_iterator AI = CS.arg_begin();
    ArgIndex = 0;
    for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
         ++I, ++AI, ++ArgIndex) {
      if (!ArgsToTransform.count(I)) {
        // Unchanged argument
        AttributeSet attrs = CallPAL.getParamAttributes(ArgIndex);
        if (attrs.hasAttributes()) {
          AttrBuilder B(attrs);
          NewAttrVec = NewAttrVec.addParamAttributes(Context, Args.size(), B);
        }
        Args.push_back(*AI);
      } else if (!I->use_empty()) {
        LoadInst *Load = new LoadInst(*AI, (*AI)->getName() + ".val", Call);
        Args.push_back(Load);
      }
    }

    // Push any varargs arguments on the list.
    for (; AI != CS.arg_end(); ++AI, ++ArgIndex) {
      AttributeSet attrs = CallPAL.getParamAttributes(ArgIndex);
      if (attrs.hasAttributes()) {
        AttrBuilder B(attrs);
        NewAttrVec = NewAttrVec.addParamAttributes(Context, Args.size(), B);
      }
      Args.push_back(*AI);
    }

    // Push any localized globals.
    for (IteratorTy I = LI.getGlobals().begin(), E = LI.getGlobals().end();
         I != E; ++I) {
      GlobalVariable *GV = *I;
      LoadInst *Load = new LoadInst(GV, GV->getName() + ".val", Call);
      Args.push_back(Load);
    }

    // Add any function attributes.
    if (CallPAL.hasAttributes(AttributeList::FunctionIndex)) {
      AttrBuilder B(CallPAL.getFnAttributes());
      NewAttrVec = NewAttrVec.addAttributes(Context, AttributeList::FunctionIndex, B);
    }

    if (isa<InvokeInst>(Call))
      llvm_unreachable("InvokeInst not supported");

    CallInst *New = CallInst::Create(NF, Args, "", Call);
    New->setCallingConv(CS.getCallingConv());
    New->setAttributes(NewAttrVec);
    if (cast<CallInst>(Call)->isTailCall())
      New->setTailCall();
    New->setDebugLoc(Call->getDebugLoc());

    // Update the callgraph to know that the callsite has been transformed.
    auto CalleeNode = static_cast<IGCLLVM::CallGraphNode *>(
        CG[Call->getParent()->getParent()]);
    CalleeNode->replaceCallEdge(CallSite(Call), New, NF_CGN);

    unsigned Index = 0;
    IRBuilder<> Builder(Call);

    New->takeName(Call);
    if (!F->getReturnType()->isVoidTy())
      Call->replaceAllUsesWith(Builder.CreateExtractValue(New, Index++, "ret"));

    // Loop over the operands, and copy out all pass by reference values.
    AI = CS.arg_begin();
    for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(); I != E;
         ++I, ++AI) {
      // Unused arguments are already eliminated from the call sites.
      if (ArgsToTransform.count(I) && !I->use_empty() &&
          CopyInOutNeeded.count(I)) {
        Value *OutVal = Builder.CreateExtractValue(New, Index++);
        Builder.CreateStore(OutVal, *AI);
      }
    }
    // Loop over localized globals, and copy out all globals.
    for (IteratorTy I = LI.getGlobals().begin(), E = LI.getGlobals().end();
      I != E; ++I) {
      GlobalVariable *GV = *I;
      Value *OutVal = Builder.CreateExtractValue(New, Index++);
      Builder.CreateStore(OutVal, GV);
    }
    assert(Index == New->getType()->getStructNumElements() && "type out of sync");

    // Remove the old call from the function, reducing the use-count of F.
    Call->eraseFromParent();
  }

  // Since we have now created the new function, splice the body of the old
  // function right into the new function.
  NF->getBasicBlockList().splice(NF->begin(), F->getBasicBlockList());

  // Allocas used for transformed arguments.
  SmallVector<AllocaInst *, 8> Allocas;

  // Loop over the argument list, transferring uses of the old arguments over to
  // the new arguments, also transferring over the names as well.
  for (Function::arg_iterator I = F->arg_begin(), E = F->arg_end(),
                              I2 = NF->arg_begin();
       I != E; ++I) {
    // For an unmodified argument, move the name and users over.
    if (!ArgsToTransform.count(I)) {
      I->replaceAllUsesWith(I2);
      I2->takeName(I);
      ++I2;
      continue;
    }

    if (I->use_empty())
      continue;

    // Otherwise, we transformed this argument.
    //
    // In the callee, we create an alloca, and store each of the new incoming
    // arguments into the alloca.
    Instruction *InsertPt = &*(NF->begin()->begin());
    Type *AgTy = I->getType()->getPointerElementType();
    AllocaInst *TheAlloca = new AllocaInst(AgTy, 0, "", InsertPt);
    Instruction * NewInst = TheAlloca;
    if (I->getType()->getPointerAddressSpace() != 0) {
      // Insert addrspace cast
      auto AddrSpaceCast = new AddrSpaceCastInst(
          TheAlloca, AgTy->getPointerTo(I->getType()->getPointerAddressSpace()),
          "");
      AddrSpaceCast->insertAfter(TheAlloca);
      NewInst = AddrSpaceCast;
    }
    if (CopyInOutNeeded.count(I))
      Allocas.push_back(TheAlloca);

    I2->setName(I->getName());
    new StoreInst(I2++, NewInst, InsertPt);

    // Anything that used the arg should now use the alloca.
    I->replaceAllUsesWith(NewInst);
    NewInst->takeName(I);
  }

  // Collect all globals and their corresponding allocas.
  SmallDenseMap<Value *, Value *> GlobalsToReplace;

  // Loop over globals and transfer uses of globals over to new arguments.
  for (IteratorTy I = LI.getGlobals().begin(), E = LI.getGlobals().end();
       I != E; ++I) {
    GlobalVariable *GV = *I;

    Instruction *InsertPt = &*(NF->begin()->begin());
    Type *AgTy = GV->getType()->getPointerElementType();
    AllocaInst *TheAlloca = new AllocaInst(AgTy, 0, "", InsertPt);
    Allocas.push_back(TheAlloca);

    auto ArgIter = NF->arg_begin();
    std::advance(ArgIter, LI.getArgIndex(GV));
    ArgIter->setName(GV->getName() + ".in");
    new StoreInst(ArgIter, TheAlloca, InsertPt);

    TheAlloca->setName(GV->getName() + ".local");
    GlobalsToReplace.insert(std::make_pair(GV, TheAlloca));
  }
  // Replaces all globals uses within this new function.
  replaceUsesWithinFunction(GlobalsToReplace, NF);

  // Fix all return instructions since we have changed the return type.
  Type *NFRetTy = NF->getReturnType();
  for (inst_iterator I = inst_begin(NF), E = inst_end(NF); I != E; /* empty */) {
    Instruction *Inst = &*I++;
    if (ReturnInst *RI = dyn_cast<ReturnInst>(Inst)) {
      IRBuilder<> Builder(RI);

      // Create new return value, which is a struct type.
      Value *RetVal = UndefValue::get(NFRetTy);
      unsigned Index = 0;

      if (!F->getReturnType()->isVoidTy()) {
        Value *RV = RI->getReturnValue();
        assert(RV && RV->getType()->isSingleValueType() && "type unexpected");
        RetVal = Builder.CreateInsertValue(RetVal, RV, Index++);
      }
      for (unsigned i = 0, e = Allocas.size(); i < e; ++i) {
        Value *V = Builder.CreateLoad(Allocas[i]);
        RetVal = Builder.CreateInsertValue(RetVal, V, Index++);
      }

      StructType *ST = cast<StructType>(NFRetTy);
      assert(ST->getNumElements() == Index && "type out of sync");
      (void)ST;

      // Return the final struct by value.
      Builder.CreateRet(RetVal);
      RI->eraseFromParent();
    }
  }

  // It turns out sometimes llvm will recycle function pointers which confuses
  // this pass. We delete its localization info and mark this function as
  // already visited.
  GlobalInfo.erase(F);
  AlreadyVisited.insert(F);

  NF_CGN->stealCalledFunctionsFrom(CG[F]);

  // Now that the old function is dead, delete it. If there is a dangling
  // reference to the CallgraphNode, just leave the dead function around.
  CallGraphNode *CGN = CG[F];
  if (CGN->getNumReferences() == 0)
    delete CG.removeFunctionFromModule(CGN);
  else
    F->setLinkage(Function::ExternalLinkage);

  return NF_CGN;
}

static void breakConstantVector(unsigned i, Instruction *CurInst,
                                Instruction *InsertPt) {
  ConstantVector *CV = cast<ConstantVector>(CurInst->getOperand(i));

  // Splat case.
  if (auto S = dyn_cast_or_null<ConstantExpr>(CV->getSplatValue())) {
    // Turn element into an instruction
    auto Inst = S->getAsInstruction();
    Inst->setDebugLoc(CurInst->getDebugLoc());
    Inst->insertBefore(InsertPt);
    Type *NewTy = VectorType::get(Inst->getType(), 1);
    Inst = CastInst::Create(Instruction::BitCast, Inst, NewTy, "", CurInst);
    Inst->setDebugLoc(CurInst->getDebugLoc());

    // Splat this value.
    IRBuilder<> Builder(InsertPt);
    Value *NewVal = Builder.CreateVectorSplat(CV->getNumOperands(), Inst);

    // Update i-th operand with newly created splat.
    CurInst->setOperand(i, NewVal);
  }

  SmallVector<Value *, 8> Vals;
  bool HasConstExpr = false;
  for (unsigned j = 0, N = CV->getNumOperands(); j < N; ++j) {
    Value *Elt = CV->getOperand(j);
    if (auto CE = dyn_cast<ConstantExpr>(Elt)) {
      auto Inst = CE->getAsInstruction();
      Inst->setDebugLoc(CurInst->getDebugLoc());
      Inst->insertBefore(InsertPt);
      Vals.push_back(Inst);
      HasConstExpr = true;
    } else
      Vals.push_back(Elt);
  }

  if (HasConstExpr) {
    Value *Val = UndefValue::get(CV->getType());
    IRBuilder<> Builder(InsertPt);
    for (unsigned j = 0, N = CV->getNumOperands(); j < N; ++j)
      Val = Builder.CreateInsertElement(Val, Vals[j], j);
    CurInst->setOperand(i, Val);
  }
}

static void breakConstantExprs(Function *F) {
  for (po_iterator<BasicBlock *> i = po_begin(&F->getEntryBlock()),
                                 e = po_end(&F->getEntryBlock());
       i != e; ++i) {
    BasicBlock *BB = *i;
    // The effect of this loop is that we process the instructions in reverse
    // order, and we re-process anything inserted before the instruction
    // being processed.
    for (Instruction *CurInst = BB->getTerminator(); CurInst;) {
      PHINode *PN = dyn_cast<PHINode>(CurInst);
      for (unsigned i = 0, e = CurInst->getNumOperands(); i < e; ++i) {
        auto InsertPt = PN ? PN->getIncomingBlock(i)->getTerminator() : CurInst;
        Value *Op = CurInst->getOperand(i);
        if (ConstantExpr *CE = dyn_cast<ConstantExpr>(Op)) {
          Instruction *NewInst = CE->getAsInstruction();
          NewInst->setDebugLoc(CurInst->getDebugLoc());
          NewInst->insertBefore(CurInst);
          CurInst->setOperand(i, NewInst);
        } else if (isa<ConstantVector>(Op))
          breakConstantVector(i, CurInst, InsertPt);
      }
      CurInst = CurInst == &BB->front() ? nullptr : CurInst->getPrevNode();
    }
  }
}

// For each function, compute the list of globals that need to be passed as
// copy-in and copy-out arguments.
void CMABI::AnalyzeGlobals(CallGraph &CG) {
  Module &M = CG.getModule();
  // assuming the device module is self-contained,
  // set internal-linkage for global variables
  // and functions so globla-DCE can remove them
  // if there is no use in the module.
  for (auto& Global : M.getGlobalList()) {
    if (!Global.isDeclaration())
      Global.setLinkage(GlobalValue::InternalLinkage);
  }
  for (auto& F : M.getFunctionList()) {
    // __cm_intrinsic_impl_* could be used for emulation mul/div etc
    if (GenXIntrinsic::getAnyIntrinsicID(&F) ==
      GenXIntrinsic::not_any_intrinsic &&
      !F.getName().contains("__cm_intrinsic_impl_") &&
      !F.isDeclaration() && !F.hasDLLExportStorageClass())
      F.setLinkage(GlobalValue::InternalLinkage);
  }
  // No global variables.
  if (M.global_empty())
    return;

  // Store functions in a SetVector to keep order and make searching efficient.
  SetVector<Function *> Funcs;
  for (auto I = scc_begin(&CG), IE = scc_end(&CG); I != IE; ++I) {
    const std::vector<CallGraphNode *> &SCCNodes = *I;
    for (const CallGraphNode *Node : SCCNodes) {
      Function *F = Node->getFunction();
      if (F != nullptr && !F->isDeclaration()) {
        Funcs.insert(F);
        breakConstantExprs(F);
      }
    }
  }
  auto PrintIndexChecker = [](Use &IUI) {
    CallInst *CI = dyn_cast<CallInst>(IUI.getUser());
    if (!CI)
      return false;
    Function *Callee = CI->getCalledFunction();
    if (!Callee)
      return false;
    unsigned IntrinID = GenXIntrinsic::getAnyIntrinsicID(Callee);
    return (IntrinID == GenXIntrinsic::genx_print_format_index);
  };
  auto UsesPrintChecker =  [PrintIndexChecker](const Use &UI) {
    auto *User = UI.getUser();
    return std::any_of(User->use_begin(), User->use_end(), PrintIndexChecker);
  };
  const auto &DL = M.getDataLayout();
  auto ToLocalize = selectGlobalsToLocalize(
      M.globals(), LocalizationLimit.getValue(),
      [UsesPrintChecker](const GlobalVariable &GV) {
        // don't localize global constant format string if it's used by print_index intrinsic
        bool UsesPrintIndex = std::any_of(GV.use_begin(), GV.use_end(), UsesPrintChecker);
        return (GV.hasAttribute(genx::FunctionMD::GenXVolatile) ||
                UsesPrintIndex);
      },
      [&DL](const GlobalVariable &GV) { return calcGVWeight(GV, DL); });
  for (auto I = Funcs.begin(), E = Funcs.end(); I != E; ++I) {
    Function *Fn = *I;
    LLVM_DEBUG(dbgs() << "Visiting " << Fn->getName() << "\n");

    // Collect globals used directly.
    for (GlobalVariable *GV : ToLocalize) {
      for (Value::use_iterator UI = GV->use_begin(), UE = GV->use_end();
           UI != UE; ++UI) {
        Instruction *Inst = dyn_cast<Instruction>(UI->getUser());
        // not used in this function.
        if (!Inst || Inst->getParent()->getParent() != Fn)
          continue;

        // Find the global being used and populate this info.
        for (unsigned i = 0, e = Inst->getNumOperands(); i < e; ++i) {
          Value *Op = Inst->getOperand(i);
          if (GlobalVariable *GV = dyn_cast<GlobalVariable>(Op))
            addDirectGlobal(Fn, GV);
        }
      }
    }

    // Collect globals used indirectly.
    for (inst_iterator II = inst_begin(Fn), IE = inst_end(Fn); II != IE; ++II) {
      Instruction *Inst = &*II;
      // Ignore InvokeInst.
      if (CallInst *CI = dyn_cast<CallInst>(Inst)) {
        // Ignore indirect calls
        if (Function *Callee = CI->getCalledFunction()) {
          // Collect all globals from its callee.
          if (!Callee->isDeclaration())
            addIndirectGlobal(Fn, Callee);
        }
      }
    }
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
  for (unsigned ArgIndex = 1, NumArgs = CI->getNumArgOperands();
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
      if (auto VT = dyn_cast<VectorType>(LI->getType()))
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
        assert(!(OpndEntry->size() & ((1U << LogRatio) - 1)));
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
              TempVector.resize(CI->getType()->getVectorNumElements(), 0);
              int VStride = cast<ConstantInt>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrVStrideOperandNum))->getSExtValue();
              unsigned Width = cast<ConstantInt>(CI->getOperand(
                    GenXIntrinsic::GenXRegion::WrWidthOperandNum))->getZExtValue();
              assert((Width > 0)&& "Width of a region must be non-zero");
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
      assert(VectorToMerge->size() == Entry->size());
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
            DiagnosticInfoOverlappingArgs Err(CI, "by reference arguments "
                + Twine(ArgIdx1) + " and " + Twine(ArgIdx2) + " overlap",
                DS_Error);
            Inst->getContext().diagnose(Err);
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

/***********************************************************************
 * DiagnosticInfoOverlappingArgs initializer from Instruction
 *
 * If the Instruction has a DebugLoc, then that is used for the error
 * location.
 * Otherwise, the location is unknown.
 */
DiagnosticInfoOverlappingArgs::DiagnosticInfoOverlappingArgs(Instruction *Inst,
    const Twine &Desc, DiagnosticSeverity Severity)
    : DiagnosticInfo(getKindID(), Severity), Line(0), Col(0)
{
  auto DL = Inst->getDebugLoc();
  if (!DL) {
    Filename = DL.get()->getFilename();
    Line = DL.getLine();
    Col = DL.getCol();
  }
  Description = Desc.str();
}

/***********************************************************************
 * DiagnosticInfoOverlappingArgs::print : print the error/warning message
 */
void DiagnosticInfoOverlappingArgs::print(DiagnosticPrinter &DP) const
{
  std::string Loc(
        (Twine(!Filename.empty() ? Filename : "<unknown>")
        + ":" + Twine(Line)
        + (!Col ? Twine() : Twine(":") + Twine(Col))
        + ": ")
      .str());
  DP << Loc << Description;
}


char CMABI::ID = 0;
INITIALIZE_PASS_BEGIN(CMABI, "cmabi", "Fix ABI issues for the genx backend", false, false)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
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
  void process();
};

struct CMLowerVLoadVStore : public FunctionPass {
  static char ID;
  CMLowerVLoadVStore() : FunctionPass(ID) {
    initializeCMLowerVLoadVStorePass(*PassRegistry::getPassRegistry());
  }
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<PostDominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }

  virtual bool runOnFunction(Function &F) override;

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
          auto LI = Builder.CreateLoad(Inst.getOperand(0), Inst.getName());
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
  assert(V1 && V2 && "null value");
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
    assert(&*I == L1 || &*I == L2);
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
  assert(Alloca);
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
  // Probably we should assert on region descriptors.
  if (CopyOutRegion &&
      !isBitwiseIdentical(CopyInRegion->getOperand(0),
                          CopyOutRegion->getOperand(0)))
    return false;

  // It should be OK to rewrite all loads and stores into the argref.
  VLoads.swap(Loads);
  VStores.swap(Stores);
  return true;
}

void ArgRefPattern::process() {
  // 'Spill' the base region into memory during rewriting.
  IRBuilder<> Builder(Alloca);
  Function *RdFn = CopyInRegion->getCalledFunction();
  assert(RdFn);
  Type *BaseAllocaTy = RdFn->getFunctionType()->getParamType(0);
  AllocaInst *BaseAlloca = Builder.CreateAlloca(BaseAllocaTy, nullptr,
                                                Alloca->getName() + ".refprom");

  Builder.SetInsertPoint(CopyInRegion);
  Builder.CreateStore(CopyInRegion->getArgOperand(0), BaseAlloca);

  if (CopyOutRegion) {
    Builder.SetInsertPoint(CopyOutRegion);
    CopyOutRegion->setArgOperand(0, Builder.CreateLoad(BaseAlloca));
  }

  // Rewrite all stores.
  for (auto ST : VStores) {
    Builder.SetInsertPoint(ST);
    Value *OldVal = Builder.CreateLoad(BaseAlloca);
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
    Value *SrcVal = Builder.CreateLoad(BaseAlloca);
    SmallVector<Value *, 8> Args(CopyInRegion->arg_operands());
    Args[0] = SrcVal;
    Value *Val = Builder.CreateCall(RdFn, Args);
    LI->replaceAllUsesWith(Val);
    LI->eraseFromParent();
  }
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
    else
      break;
  }

  for (auto AI : Allocas) {
    ArgRefPattern ArgRef(AI);
    if (ArgRef.match(DT, PDT)) {
      ArgRef.process();
      Modified = true;
    }
  }

  return Modified;
}

Pass *llvm::createCMLowerVLoadVStorePass() { return new CMLowerVLoadVStore; }
