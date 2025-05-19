/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXArgIndirection
/// ------------------
///
/// The GenXArgIndirection pass runs very late, after coalescing and address
/// commoning, to change arguments and return values that were originally by ref
/// to use address registers. This saves copies and register pressure.
///
/// Recall that, very early on in CMABI, a by ref argument is transformed into
/// copy-in copy-out semantics.
///
/// This pass is run very late on for two reasons:
///
/// 1. There is no convenient way to represent passing an argument using an
///    address register in LLVM IR. We don't want to pretend that the address
///    register is a pointer, and the GRF is an area of memory, as that would
///    stop us using Values to represent registers normally, and so would stop
///    us using lots of LLVM optimizations.
///
///    Running the pass this late means that the IR afterwards does not have to
///    strictly represent the semantics, as nothing else happens to it before
///    generating the output code. So uses and defs of the indirected argument
///    (and other Values coalesced with it) still use the same Values, but that
///    live range has no register allocated (it is category NONE), and all
///    accesses are indirected.  We rely on the LLVM IR together with the
///    liveness information representing the code well enough for register
///    allocation and code generation to work.
///
/// 2. We cannot tell whether we want to perform this transformation until we can
///    see how Values have coalesced.
///
/// Action of GenXArgIndirection
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// An argument for a subroutine call can generate a bunch of mov instructions in
/// two circumstances:
///
/// 1. Coalescing failed to coalesce this call argument, so the argument in the
///    caller and the argument in the subroutine are in different registers
///    (different coalesced live ranges). In this case, GenXCoalescing has to
///    generate a sequence of baled together rdregion-wrregion intrinsic pairs,
///    each generating a mov instruction, to copy the value.
///
/// 2. The argument was originally a by ref CM select(), so is an rdregion,
///    legalized into a sequence of baled together rdregion-wrregion pairs.
///
/// The argument indirection pass attempts to spot these cases. The regions at
/// each call site must be similar (same region parameters except start index)
/// and contiguous.
///
/// The pass modifies each call to pass an address register into the subroutine
/// as an extra argument, using it to indirecting all accesses to the subroutine
/// argument and other Values coalesced with it. It then removes the rd-wr
/// sequence so it does not generate any code.
///
/// Indirecting all accesses to the subroutine argument is only possible if each
/// one would be legal as an indirect region. The pass uses the
/// hasIndirectGRFCrossing feature from GenXSubtarget to tell whether it would
/// be legal. The optimization can fail for this reason, and that is more common
/// on pre-SKL where there is no indirect region GRF crossing.
///
/// The pass deals with one subroutine argument in one subroutine at a time. It
/// looks at all call sites to see if there is anything that stops this
/// transformation happening at all, and whether there is any call site that
/// would benefit from the transformation.
///
/// Coalesced return value
/// """"""""""""""""""""""
///
/// If the subroutine argument is coalesced with a return value from the call,
/// then argument indirection can succeed only if the return value at each call
/// site is written (similarly using a rd-wr sequence) to exactly the same
/// region in a vector that is coalesced (so same register) with the input
/// vector to the rd-wr sequence for the argument.
///
/// No coalesced return value
/// """""""""""""""""""""""""
///
/// If the subroutine argument is _not_ coalesced with a return value from the
/// call, so only the arg could be indirected, indirection can only occur if one
/// of these conditions is met:
///
/// 1. the live range being indirected is not live over the call (so it does not
///    matter if the subroutine writes to the same register), or
///
/// 2. the subroutine does not write to the same register (i.e. there are no defs
///    in the subroutine arg's live range other than args and coalesced
///    bitcasts).
///
/// Constant argument and rd-wr sequence return value
/// """""""""""""""""""""""""""""""""""""""""""""""""
///
/// Where the original source initializes a big vector or matrix to constant and
/// then calls a subroutine passing the vector by ref, the IR that this pass sees
/// is that the argument passed to the call is constant, and the rd-wr sequence
/// for the return value has an "old value" input that is another constant
/// (including undef).
///
/// GenXArgIndirection spots this case, and transforms the code to load the
/// combination of the two constants before the call and pass an address register
/// set to the appropriate point.
///
/// Indirection of subroutine
/// """""""""""""""""""""""""
///
/// If an argument is being indirected, all references to that register
/// (coalesced live range) inside the subroutine and everything it calls must be
/// indirected.
///
/// GenXArgIndirection does not include the facility to split up a bale if it
/// would become illegal when indirected. This is only a problem in BDW and
/// earlier, where an indirect region is not allowed to cross even one GRF
/// boundary. If it sees an access with a region that would become illegal if
/// indirected, it abandons indirection of that argument.
///
/// Warning messages
/// ^^^^^^^^^^^^^^^^
///
/// Where GenXArgIndirection sees a suitably large uncoalesced call arg that
/// would benefit from arg indirection, but it fails to satisfy the criteria,
/// the pass outputs a warning message. The idea is that the CM programmer
/// might consider some changes to his/her kernel to optimize it.
///
//===----------------------------------------------------------------------===//
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXAlignmentInfo.h"
#include "GenXBaling.h"
#include "GenXConstants.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/KernelInfo.h"
#include "vc/Utils/GenX/RegCategory.h"
#include "vc/Utils/General/FunctionAttrs.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Function.h"

#define DEBUG_TYPE "GENX_ARGINDIRECTION"

using namespace llvm;
using namespace genx;

static cl::opt<unsigned> LimitGenXArgIndirection("limit-genx-arg-indirection", cl::init(UINT_MAX), cl::Hidden,
                                      cl::desc("Limit GenX argument indirection."));

namespace {

class GenXArgIndirection;
class SubroutineArg;

// processArgLR relies on these being in this order.
// checkIndirectability relies on these being powers of 2 (except
// CALLER_INDIRECTING being 0)
enum Indirectability {
  CALLER_INDIRECTING = 0,
  NO_OPTIMIZATION = 1,
  WANT_INDIRECTION = 2,
  WANT_SOME_INDIRECTION = 4,
  CANNOT_INDIRECT = 8
};

// A call site and the action that we want to take when indirecting the arg.
// This is then subclassed by the *ArgCallSite classes below.
class ArgIndCallSite {
public:
  CallInst *CI;
protected:
  Indirectability State;
  Value *Index;
public:
  ArgIndCallSite(CallInst *CI, Indirectability State, Value *Index)
      : CI(CI), State(State), Index(Index) {}
  virtual ~ArgIndCallSite() {}
  Indirectability getState() const { return State; }
  Value *getIndex() const { return Index; }
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg) = 0;
  virtual void printImpl(raw_ostream &OS) const = 0;
  void print(raw_ostream &OS) const { printImpl(OS); }
};

raw_ostream &operator<<(raw_ostream &OS, const ArgIndCallSite &CS) {
  CS.print(OS); return OS;
}

// A call site in a subroutine that is itself indirecting the arg.
class CallerIndirectingCallSite : public ArgIndCallSite {
  SubroutineArg *CallerSubrArg;
public:
  CallerIndirectingCallSite(CallInst *CI, SubroutineArg *CallerSubrArg)
      : ArgIndCallSite(CI, Indirectability::CALLER_INDIRECTING, nullptr),
        CallerSubrArg(CallerSubrArg) {}
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg);
  virtual void printImpl(raw_ostream &OS) const {
    OS << "CallerIndirectingCallSite " << *CI;
  }
};

// A call site where indirecting the arg does not give any optimization because
// we did not find copies or rd/wr regions that we can get rid of. We can still
// indirect it though if other call sites do get an optimization.
class NoOptCallSite : public ArgIndCallSite {
public:
  NoOptCallSite(CallInst *CI)
      : ArgIndCallSite(CI, Indirectability::NO_OPTIMIZATION, nullptr) {}
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg);
  virtual void printImpl(raw_ostream &OS) const {
    OS << "NoOptCallSite " << *CI;
  }
};

// A call site where the arg is constant (including undef) and the arg is
// coalesced with a retval that is used only in a legalized wrregion
// whose "old value" input is constant.
class ConstArgRetCallSite : public ArgIndCallSite {
  Constant *LdConst; // the constant that needs to be loaded
  AssertingVH<Instruction> RetEndWr; // the last wrregion in the sequence for the retval
public:
  ConstArgRetCallSite(CallInst *CI, Constant *LdConst, Instruction *RetEndWr,
                      Value *Index)
      : ArgIndCallSite(CI, Indirectability::WANT_INDIRECTION, Index),
        LdConst(LdConst), RetEndWr(RetEndWr) {}
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg);
  virtual void printImpl(raw_ostream &OS) const {
    OS << "ConstArgRetCallSite " << *CI << "\n    LdConst " << *LdConst
       << " \n    RetEndWr " << *RetEndWr << "\n    Index " << *Index;
  }
};

// A call site where the arg is a legalized rdregion or copy, and there is no
// retval coalesced with it.
class IndirectArgCallSite : public ArgIndCallSite {
protected:
  // Some use of input (arg or inst) in legalized rdregion or copy. This is
  // kept as a Use * rather than the value it actually uses to allow for the
  // case that the value is something that will be replaced and erased by
  // another call site processing the same ArgLR.
  Use *InputUse;
public:
  IndirectArgCallSite(CallInst *CI, Use *InputUse, Value *Index)
      : ArgIndCallSite(CI, Indirectability::WANT_INDIRECTION, Index),
        InputUse(InputUse) {}
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg);
  virtual void printImpl(raw_ostream &OS) const {
    OS << "IndirectArgCallSite " << *CI << "\n    Input " << (*InputUse)->getName()
       << " Index " << *Index;
  }
};

// A call site where the arg is a legalized rdregion or copy, and the arg is
// coalesced with a retval that is used only in a legalized wrregion or copy.
class IndirectArgRetCallSite : public IndirectArgCallSite {
  AssertingVH<Instruction> RetEndWr; // the last wrregion in the sequence for the retval
public:
  IndirectArgRetCallSite(CallInst *CI, Use *InputUse, Instruction *RetEndWr,
      Value *Index) : IndirectArgCallSite(CI, InputUse, Index), RetEndWr(RetEndWr)
  {}
  virtual Value *process(GenXArgIndirection *Pass, SubroutineArg *SubrArg);
  virtual void printImpl(raw_ostream &OS) const {
    OS << "IndirectArgRetCallSite " << *CI << "\n    Input " << (*InputUse)->getName()
       << " RetEndWr " << RetEndWr->getName() << " Index " << *Index;
  }
};


class GenXArgIndirection;

// A subroutine arg that we might want to indirect
class SubroutineArg {
  GenXArgIndirection *Pass = nullptr;
public:
  LiveRange *ArgLR = nullptr;
  Argument *Arg = nullptr;
private:
  int CoalescedRetIdx = -1;
  bool CanCoalesceWithoutKill = false;
  SmallVector<ArgIndCallSite *, 4> CallSites;
  Alignment Align;
  Function *F = nullptr;
  Function *NewFunc = nullptr;
public:
  Argument *AddressArgument = nullptr;
  SubroutineArg(GenXArgIndirection *Pass, LiveRange *ArgLR, Argument *Arg)
    : Pass(Pass), ArgLR(ArgLR), Arg(Arg), F(Arg->getParent()), NewFunc(nullptr) {}
  ~SubroutineArg() {
    for (auto i = CallSites.begin(), e = CallSites.end(); i != e; ++i)
      delete *i;
  }
  SubroutineArg() = delete;
  SubroutineArg(const SubroutineArg &) = delete;
  SubroutineArg &operator=(const SubroutineArg &) = delete;

  SubroutineArg(SubroutineArg &&) = default;
  SubroutineArg &operator=(SubroutineArg &&) = delete;

  Indirectability checkIndirectability();
  ArgIndCallSite *createCallSite(CallInst *CI);
  Alignment getIndirectAlignment(unsigned GRFWidth) const;
  void gatherBalesToModify(Alignment Align);
  std::pair<Value *, Value *> addAddressArg();
  void fixCallSites();
  void coalesceAddressArgs();
  void replaceFunction();
private:
  static Value *getRetVal(CallInst *CI, unsigned RetNum);
};

// GenX arg indirection pass
class GenXArgIndirection : public FGPassImplInterface,
                           public IDMixin<GenXArgIndirection> {
  friend ArgIndCallSite;
  friend SubroutineArg;
  friend NoOptCallSite;
  friend ConstArgRetCallSite;
  friend IndirectArgCallSite;
  friend IndirectArgRetCallSite;
private:
  FunctionGroup *FG = nullptr;
  FunctionGroupAnalysis *FGA = nullptr;
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  AlignmentInfo *AI = nullptr;
  const GenXSubtarget *ST = nullptr;
  const DataLayout *DL = nullptr;
  // List of arg live ranges to consider.
  SmallVector<LiveRange *, 4> ArgLRs;
  // For the ArgLR being processed:
  // List of subroutine args in the ArgLR.
  SmallVector<SubroutineArg, 4> SubrArgs;
  // Bales that need modifying for indirection.
  SmallVector<Instruction *, 4> BalesToModify;
  // Map from function back to the SubroutineArg for it.
  std::map<Function *, SubroutineArg *> FuncMap;
  // List of LRs that we need to recalculate.
  SmallVector<LiveRange *, 4> LRsToCalculate;
public:
  explicit GenXArgIndirection() {}
  static StringRef getPassName() { return "GenX arg indirection"; }
  static void getAnalysisUsage(AnalysisUsage &AU) {
    AU.addRequired<FunctionGroupAnalysis>();
    AU.addRequired<GenXNumbering>();
    AU.addRequired<GenXLiveness>();
    AU.addRequired<GenXGroupBaling>();
    AU.addRequired<GenXGroupLiveElementsWrapper>();
    AU.addRequired<GenXModule>();
    AU.addPreserved<GenXGroupBaling>();
    AU.addPreserved<GenXLiveness>();
    AU.addPreserved<GenXNumbering>();
    AU.addPreserved<GenXModule>();
    AU.addPreserved<FunctionGroupAnalysis>();
  }
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void gatherArgLRs();
  bool processArgLR(LiveRange *ArgLR);
  bool gatherBalesToModify(LiveRange *ArgLR, Alignment Align);
  bool checkIndirectBale(Bale *B, LiveRange *ArgLR, Alignment Align);
  void indirectBale(Bale *B, LiveRange *ArgLR, Argument *AddressArg);
  void indirectRegion(Use *U, Value *AddressArg, Instruction *InsertBefore);
  static Argument *getArgForFunction(LiveRange *LR, Function *F);
  void replaceAndEraseSequence(Instruction *RetEndWr, Value *V);

  struct ArgToConvertAddr {
    Value *Arg;
    Value *ConvertAddr;
  };
  std::vector<ArgToConvertAddr> collectAlreadyIndirected(LiveRange *ArgLR);
};

} // end anonymous namespace

namespace llvm {
void initializeGenXArgIndirectionWrapperPass(PassRegistry &);
using GenXArgIndirectionWrapper = FunctionGroupWrapperPass<GenXArgIndirection>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXArgIndirectionWrapper, "GenXArgIndirectionWrapper",
                      "GenXArgIndirectionWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXGroupLiveElementsWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXNumberingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_END(GenXArgIndirectionWrapper, "GenXArgIndirectionWrapper",
                    "GenXArgIndirectionWrapper", false, false)

ModulePass *llvm::createGenXArgIndirectionWrapperPass() {
  initializeGenXArgIndirectionWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXArgIndirectionWrapper();
}

/***********************************************************************
 * runOnFunctionGroup : run the coalescing pass for this FunctionGroup
 */
bool GenXArgIndirection::runOnFunctionGroup(FunctionGroup &ArgFG)
{
  FG = &ArgFG;
  unsigned Modified = 0;
  // Get analyses that we use and/or modify.
  FGA = &getAnalysis<FunctionGroupAnalysis>();
  Baling = &getAnalysis<GenXGroupBaling>();
  Numbering = &getAnalysis<GenXNumbering>();
  Liveness = &getAnalysis<GenXLiveness>();
  Liveness->setLiveElements(&getAnalysis<GenXGroupLiveElements>());
  AI = new AlignmentInfo;
  ST = getAnalysis<GenXModule>().getSubtarget();
  DL = &ArgFG.getModule()->getDataLayout();
  // Gather list of LRs containing an arg that we want to consider. (Two
  // args might be coalesced together, so we consider a whole arg-containing
  // LR at a time.)
  gatherArgLRs();
  // Process them.
  for (auto i = ArgLRs.begin(), e = ArgLRs.end();
      i != e && Modified < LimitGenXArgIndirection; ++i) {
    if (processArgLR(*i)) {
      ++Modified;
      if (LimitGenXArgIndirection != UINT_MAX)
        dbgs() << "genx-arg-indirection " << Modified << "\n";
    }
  }
  ArgLRs.clear();
  SubrArgs.clear();
  BalesToModify.clear();
  FuncMap.clear();
  LRsToCalculate.clear();
  delete AI;
  return Modified != 0;
}

/***********************************************************************
 * gatherArgLRs : gather a list of LRs containing an arg that we want to
 *    consider
 */
void GenXArgIndirection::gatherArgLRs()
{
  std::set<LiveRange *> Seen;
  // For a kernel arg, add it to Seen but not to the list, so it will not get
  // added to the list. We cannot indirect a kernel arg.
  for (auto ai = FG->at(0)->arg_begin(), ae = FG->at(0)->arg_end();
      ai != ae; ++ai)
    Seen.insert(Liveness->getLiveRange(&*ai));
  // For a subroutine arg, add its LR to the list if it is not already in Seen.
  for (auto fgi = FG->begin() + 1, fge = FG->end(); fgi != fge; ++fgi) {
    if (vc::requiresStackCall(*fgi))
      continue;
    for (auto ai = (*fgi)->arg_begin(), ae = (*fgi)->arg_end(); ai != ae; ++ai) {
      Argument *Arg = &*ai;
      // Only process an arg that is bigger than 2 GRFs.
      if (Arg->getType()->getPrimitiveSizeInBits() <= ST->getGRFByteSize() * 16)
        continue;
      LiveRange *LR = Liveness->getLiveRange(Arg);
      if (Seen.insert(LR).second)
        ArgLRs.push_back(LR);
    }
  }
}

// A simple helper function for
// GenXArgIndirection::collectAlreadyIndirected. It tries to find the original
// argument in wrr and rdr sequence.
static Value *searchForArg(Value *V) {
  IGC_ASSERT(V);
  while (!isa<Argument>(V)) {
    auto *I = dyn_cast<Instruction>(V);
    if (I && (GenXIntrinsic::isRdRegion(I) || GenXIntrinsic::isWrRegion(I)))
      V = I->getOperand(0);
    else
      return nullptr;
  }
  return V;
}

/***********************************************************************
 * collectAlreadyIndirected : Some values in the ArgLR may be already
 * indirected. It collects genx.convert.addr (indirection) and an argument that
 * was the base.
 *
 * Return:  A vector with collected values.
 */
std::vector<GenXArgIndirection::ArgToConvertAddr>
GenXArgIndirection::collectAlreadyIndirected(LiveRange *ArgLR) {
  std::vector<GenXArgIndirection::ArgToConvertAddr> IndirectedArgInfo;
  for (auto &VI : ArgLR->getValues()) {
    Value *Base = VI.getValue();
    std::vector<Value *> Addrs = Liveness->getAddressWithBase(Base);
    // Not a base
    if (Addrs.empty())
      continue;
    // A chain of
    // @bar (%arg) {
    //   ...
    //   %1 = genx.convert.addr
    //   %2 = call foo(..., %1)
    //   %dummy_use_for_indirection = bitcast ...
    // }
    // ArgLR: ..., %arg, %dummy_use_for_indirection, ...
    // is expected. The bitcast is a dummy instruction that holds a base for the
    // genx.convert.addr. If the bitcast is in ArgLR which will be set to NONE
    // category, we must find another base for the genx.convert.addr. The idea
    // is that the bitcast operand originates from the %arg (because both of
    // them are in ArgLR) and the base for genx.convert.addr may be taken as
    // indirected %arg. This indirected arg will have ADDRESS category and
    // genx.convert.addr will be just an offset in this address.
    auto *DummyBC = dyn_cast<BitCastInst>(Base);
    IGC_ASSERT_MESSAGE(DummyBC, "Unexpected base: not a dummy bitcast.");
    Value *Arg = searchForArg(DummyBC->getOperand(0));
    IGC_ASSERT_MESSAGE(Arg, "Cannot find original argument");

    std::transform(Addrs.begin(), Addrs.end(),
                   std::back_inserter(IndirectedArgInfo), [Arg](Value *Addr) {
                     return ArgToConvertAddr{Arg, Addr};
                   });
  }
  return IndirectedArgInfo;
}

/***********************************************************************
 * processArgLR : process one live range containing at least one subroutine arg
 *
 * Return:  true = some modifications made
 */
bool GenXArgIndirection::processArgLR(LiveRange *ArgLR)
{
  // Get a list of args in this live range.
  SubrArgs.clear();
  FuncMap.clear();
  LLVM_DEBUG(dbgs() << "processArgLR: " << *ArgLR << "\n");
  for (auto vi = ArgLR->value_begin(), ve = ArgLR->value_end(); vi != ve; ++vi)
    if (auto Arg = dyn_cast<Argument>(vi->getValue())) {
      SubrArgs.push_back(SubroutineArg(this, ArgLR, Arg));
      FuncMap[Arg->getParent()] = &SubrArgs.back();
    }
  // For each arg, see if we can or want to indirect.
  Indirectability Res = Indirectability::NO_OPTIMIZATION;
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg) {
    LLVM_DEBUG(dbgs() << " checkIndirectability on arg " << SubrArg->Arg->getArgNo()
        << " (" << (SubrArg->Arg->getType()->getPrimitiveSizeInBits() / 8U)
        << " bytes) in " << SubrArg->Arg->getParent()->getName() << "\n");
    Res = std::max(Res, SubrArg->checkIndirectability());
  }
  if (Res == Indirectability::NO_OPTIMIZATION) {
    LLVM_DEBUG(dbgs() << "NO_OPTIMIZATION\n");
    return false; // no indirection needed
  }
  if (Res == Indirectability::CANNOT_INDIRECT) {
    LLVM_DEBUG(dbgs() << "CANNOT_INDIRECT\n");
    return false; // cannot indirect this ArgLR
  }
  // Get the worst case alignment of the indices from the call sites if we
  // indirect this arg.
  Alignment Align = Alignment(genx::log2(ST->getGRFByteSize()), 0);
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg) {
    auto ThisAlign = SubrArg->getIndirectAlignment(ST->getGRFByteSize());
    Align = Align.merge(ThisAlign);
  }
  // Gather the bales that need indirecting, and check whether indirection is
  // possible.
  if (!gatherBalesToModify(ArgLR, Align))
    return false;
  LLVM_DEBUG(dbgs() << "GenXArgIndirection is going to indirect " << *ArgLR << "\n");
  LRsToCalculate.clear();
  if (Res == Indirectability::WANT_SOME_INDIRECTION) {
    // The arg that we're indirecting is coalesced at some call site where we
    // are going to indirect it (represented by a NoOptCallSite). To avoid the
    // coalesced LR also being live at other call sites where the arg is in
    // fact in some other register, we need to uncoalesce. We take the values
    // in ArgLR and separate into two piles: one defined outside subroutines
    // where ArgLR has an arg, and one defined inside such subroutines. Then
    // the two piles get a live range each, and the latter one is marked as not
    // needing a register allocating.
    SmallVector<SimpleValue, 4> OutsidePile;
    SmallVector<SimpleValue, 4> InsidePile;
    for (auto vi = ArgLR->value_begin(), ve = ArgLR->value_end();
        vi != ve; ++vi) {
      auto SV = *vi;
      Function *ContainingFunc = Liveness->isUnifiedRet(SV.getValue());
      if (!ContainingFunc) {
        if (auto VArg = dyn_cast<Argument>(SV.getValue()))
          ContainingFunc = VArg->getParent();
        else
          ContainingFunc = cast<Instruction>(SV.getValue())->getFunction();
      }
      if (!FuncMap[ContainingFunc])
        OutsidePile.push_back(SV);
      else
        InsidePile.push_back(SV);
    }
    IGC_ASSERT(!InsidePile.empty());
    if (!OutsidePile.empty()) {
      Liveness->removeValuesNoDelete(ArgLR);
      LiveRange *OutsideLR = Liveness->getOrCreateLiveRange(OutsidePile[0]);
      OutsideLR->setCategory(ArgLR->getCategory());
      for (auto vi = OutsidePile.begin() + 1, ve = OutsidePile.end();
          vi != ve; ++vi)
        Liveness->setLiveRange(*vi, OutsideLR);
      for (auto vi = InsidePile.begin(), ve = InsidePile.end();
          vi != ve; ++vi)
        Liveness->setLiveRange(*vi, ArgLR);
      LLVM_DEBUG(dbgs() << " Uncoalesced ArgLR into " << *OutsideLR
           << "\n                    and " << *ArgLR << "\n");
      LRsToCalculate.push_back(OutsideLR);
    }
  }

  auto IndirectedArgInfo = collectAlreadyIndirected(ArgLR);

  // ArgLR now contains only these values:
  //  - args that we are indirecting
  //  - other values inside the subroutines that we are indirecting
  // We do not want it to get a register allocated, since those values will be
  // indirected. We achieve that by setting ArgLR's category to NONE.
  ArgLR->setCategory(vc::RegCategory::None);
  LLVM_DEBUG(dbgs() << " Not allocating register for arg's LR\n");
  // Arg to indirected arg map.
  std::map<Value *, Value *> ArgToAddressArg;
  // For each subroutine, replace the func with a new one that has an extra
  // address arg.
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg)
    ArgToAddressArg.insert(SubrArg->addAddressArg());
  // Since the ArgLR has been set to NONE category, it cannot be used as a base
  // register and the indirected argument address should be used instead.
  for (auto &Info : IndirectedArgInfo) {
    IGC_ASSERT_MESSAGE(ArgToAddressArg.count(Info.Arg),
                       "Cannot find indirected arg");
    Liveness->setArgAddressBase(Info.ConvertAddr, ArgToAddressArg[Info.Arg]);
  }
  // For each subroutine, fix up its call sites.
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg)
    SubrArg->fixCallSites();
  // Replace old function with new function.
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg)
    SubrArg->replaceFunction();
  // Run gatherBalesToModify again, as the list it made last time is now invalid
  // due to code being changed.
  if (!gatherBalesToModify(ArgLR, Align))
    IGC_ASSERT_EXIT_MESSAGE(0, "not expecting indirection to have become invalid in second run");
  // Indirect the bales.
  for (auto bi = BalesToModify.begin(), be = BalesToModify.end();
      bi != be; ++bi) {
    Instruction *Inst = *bi;
    Bale B;
    Baling->buildBale(Inst, &B);
    auto argIter = Inst->getFunction()->arg_begin();
    std::advance(argIter, Inst->getFunction()->arg_size() - 1);
    Argument *AddressArg = &*argIter;
    indirectBale(&B, ArgLR, AddressArg);
  }
  // Recalculate live ranges as required. Rebuild the call graph first, as it
  // has been made invalid by us replacing some functions.
  {
    Liveness->rebuildCallGraph();
    std::set<LiveRange *> LRsSeen;
    for (auto i = LRsToCalculate.begin(), e = LRsToCalculate.end(); i != e; ++i) {
      LiveRange *LR = *i;
      if (LRsSeen.insert(LR).second) {
        Liveness->rebuildLiveRange(LR);
        LLVM_DEBUG(dbgs() << " recalculated " << *LR << "\n");
      }
    }
  }
  // Coalesce (or insert copy on coalesce failure) new address args.
  for (auto SubrArg = SubrArgs.begin(), e = SubrArgs.end();
      SubrArg != e; ++SubrArg)
    SubrArg->coalesceAddressArgs();
  return true;
}

/***********************************************************************
 * checkIndirectability : check whether we want to and can indirect a
 *    subroutine argument, populating the SubrArg struct so we have the
 *    information needed to indirect it
 *
 * Return:  NO_OPTIMIZATION : can indirect, but no optimization in terms of
 *                saving instructions or register pressure
 *          WANT_INDIRECTION : can indirect and it is an optimization. The live
 *                range does not include anything outside of subroutines where
 *                it is an arg, thus we need to ensure that no register is
 *                allocated to it.
 *          WANT_SOME_INDIRECTION : can indirect and it is an optimization. The
 *                live range does include something outside of subroutines where
 *                it is an arg, so we need to ensure that a register is allocated
 *                to it. We get this if some call sites are WANT_INDIRECTION and
 *                some are NO_OPTIMIZATION.
 *          CANNOT_INDIRECT : cannot indirect this live range at all.
 */
Indirectability SubroutineArg::checkIndirectability()
{
  if (vc::requiresStackCall(F))
    return CANNOT_INDIRECT;
  // See if there is a return value that is coalesced with the arg.
  CoalescedRetIdx = -1;
  for (unsigned ri = 0, re = IndexFlattener::getNumElements(F->getReturnType());
      ri != re; ++ri) {
    if (Pass->Liveness->getLiveRange(
          SimpleValue(Pass->Liveness->getUnifiedRet(F), ri)) == ArgLR) {
      if (CoalescedRetIdx >= 0) {
        for (auto *U: F->users()) {
          if (auto *CI = checkFunctionCall(U, F)) {
            vc::warn(CI->getContext(), "GenXArgIndirection",
                     "Argument coalesced with multiple return values", Arg);
          }
        }
        return Indirectability::CANNOT_INDIRECT;
      }
      CoalescedRetIdx = ri;
    }
  }
  // If there is no return value, check whether it is OK to indirect a call arg
  // even if the call arg is not killed at the call. This is the case if there
  // is no write to the subroutine arg's live range inside the subroutine(s)
  // other than args and coalesced bitcasts.
  CanCoalesceWithoutKill = true;
  if (CoalescedRetIdx < 0) {
    for (auto vi = ArgLR->value_begin(), ve = ArgLR->value_end(); vi != ve; ++vi) {
      auto Inst = dyn_cast<Instruction>(vi->getValue());
      if (!Inst)
        continue; // it's an arg, not an instruction
      Function *Func = Pass->Liveness->isUnifiedRet(Inst);
      if (!Func)
        Func = Inst->getFunction();
      else
        continue;
      if (Pass->FuncMap.find(Func) == Pass->FuncMap.end())
        continue; // value not in one of the subroutines where the arg is indirected
      auto *CI = dyn_cast<CastInst>(Inst);
      if (!CI || !genx::isNoopCast(CI) || !Pass->Liveness->isNoopCastCoalesced(CI)) {
        CanCoalesceWithoutKill = false;
        break;
      }
    }
  }

  // Create an object of some subclass of ArgIndCallSite for each call site.
  for (auto &U: F->uses()) {
    if (auto *CI = checkFunctionCall(U.getUser(), F)) {
      IGC_ASSERT(U.getOperandNo() == IGCLLVM::getNumArgOperands(CI));
      auto CS = createCallSite(CI);
      if (!CS)
        return Indirectability::CANNOT_INDIRECT;
      CallSites.push_back(CS);
      LLVM_DEBUG(dbgs() << "  " << *CS << "\n");
    }
  }
  // Check indirection state for each call site.
  unsigned States = 0;
  for (auto csi = CallSites.begin(), cse = CallSites.end(); csi != cse; ++csi) {
    auto CS = *csi;
    States |= CS->getState();
  }
  switch (States & (Indirectability::NO_OPTIMIZATION | Indirectability::WANT_INDIRECTION)) {
  case Indirectability::NO_OPTIMIZATION | Indirectability::WANT_INDIRECTION:
      return Indirectability::WANT_SOME_INDIRECTION;
  case Indirectability::WANT_INDIRECTION:
      return Indirectability::WANT_INDIRECTION;
  }
  return Indirectability::NO_OPTIMIZATION;
}

/***********************************************************************
 * createCallSite : create a ArgIndCallSite object for this call
 *
 * Enter:   CI = CallInst
 *          this->Arg = the Argument to look at
 *          this->ArgLR = its LiveRange
 *          this->CoalescedRetIdx = -1 else struct index of coalesced return
 * value
 *
 * Return:  0 if this call stops arg indirection happening for this arg
 *          otherwise object of some subclass of CallSite
 */
ArgIndCallSite *SubroutineArg::createCallSite(CallInst *CI) {
  const DataLayout &DL = CI->getModule()->getDataLayout();
  // Check if this call site is in a function that is itself indirecting the
  // arg.
  if (auto SubrArg = Pass->FuncMap[CI->getFunction()])
    return new CallerIndirectingCallSite(CI, SubrArg);
  // Look at the call arg.
  Value *V = CI->getArgOperand(Arg->getArgNo());
  // Skip any coalesced bitcasts.
  while (auto BC = dyn_cast<BitCastInst>(V)) {
    if (Pass->Liveness->getLiveRangeOrNull(BC->getOperand(0)) != ArgLR)
      break;
    V = BC->getOperand(0);
  }
  // If the call arg (before coalesced bitcasts) is a wrregion where the arg
  // is the only use, try and parse it as a rd-wr sequence that reads a
  // contiguous region and writes the whole of Arg.
  RdWrRegionSequence ArgRWS;
  if (!V->hasOneUse() || !GenXIntrinsic::isWrRegion(V) ||
      !ArgRWS.buildFromWr(cast<Instruction>(V), Pass->Baling) ||
      !ArgRWS.RdR.isContiguous() || !ArgRWS.WrR.isWhole(Arg->getType(), &DL)) {
    // Failed to find such a rd-wr sequence. Set ArgRWS to null.
    ArgRWS = RdWrRegionSequence();
  }
  // Look at the retval.
  RdWrRegionSequence RetRWS;
  if (CoalescedRetIdx >= 0) {
    Value *RetVal = getRetVal(CI, CoalescedRetIdx);
    if (!RetVal) {
      // getRetVal could not determine what happens to this return value.
      vc::warn(CI->getContext(), "GenXArgIndirection",
               "Coalesced return value has unknown uses", Arg);
      return nullptr;
    }
    if (!isa<UndefValue>(RetVal)) {
      // See if the return value has a single use in (after skipping coalesced
      // bitcasts) a single wrregion or a rd-wr sequence.
      // First skip single use coalesced bitcasts.
      while (!RetVal->use_empty()) {
        auto User = cast<Instruction>(RetVal->use_begin()->getUser());
        if (RetVal->hasOneUse()) {
          if (auto BC = dyn_cast<BitCastInst>(User)) {
            if (Pass->Liveness->getLiveRange(BC) == ArgLR) {
              // Skip coalesced bitcast.
              RetVal = BC;
              continue;
            }
          }
        }
        // Attempt to parse as a rd-wr sequence that reads the whole of RetVal
        // and writes a contiguous region, so it is either a legalized copy, or
        // a legalized contiguous wrregion, and it is the only use of the input.
        if (!GenXIntrinsic::isRdRegion(User) ||
            RetVal->use_begin()->getOperandNo() !=
                GenXIntrinsic::GenXRegion::OldValueOperandNum ||
            !RetRWS.buildFromRd(User, Pass->Baling) ||
            !RetRWS.WrR.isContiguous() ||
            !RetRWS.RdR.isWhole(RetVal->getType(), &DL) ||
            !RetRWS.isOnlyUseOfInput()) {
          // That failed, so make RetRWS null.
          RetRWS = RdWrRegionSequence();
        }
        break;
      }
    }
  }

  // Now check the various cases. This results in the creation of an object of
  // some subclass of ArgIndCallSite.

  // Check that the regions are contiguous, and report if they are not.
  if (ArgRWS.isNull() && !ArgRWS.RdR.isContiguous()) {
    vc::warn(CI->getContext(), "GenXArgIndirection", "Non-contiguous region",
             Arg);
    return new NoOptCallSite(CI);
  }
  if (RetRWS.isNull() && !RetRWS.WrR.isContiguous()) {
    vc::warn(CI->getContext(), "GenXArgIndirection",
             "Non-contiguous region for coalesced return value", Arg);
    return new NoOptCallSite(CI);
  }

  // Case 1: The call arg is constant (inc undef, or a legalized constant
  // load), and the retval is input to a wrregion sequence where the "old
  // value" input is also a constant (a legalized constant load, also allowing
  // for a bitcast). This typically happens when the arg and ret were a by ref
  // region of a matrix, but the matrix was initialized to constant, or not
  // initialized at all, before the call, so the rdregion got simplified away.
  if (!RetRWS.isNull() && RetRWS.OldVal) {
    Value *RetOldVal = RetRWS.OldVal;
    while (auto BC = dyn_cast<BitCastInst>(RetOldVal))
      RetOldVal = BC->getOperand(0);
    auto *RetOldValC = dyn_cast<Constant>(RetOldVal);
    if (!RetOldValC && GenXIntrinsic::isWrRegion(RetOldVal)) {
      RdWrRegionSequence ConstRWS;
      if (ConstRWS.buildFromWr(cast<Instruction>(RetOldVal), Pass->Baling))
        RetOldValC = dyn_cast<Constant>(ConstRWS.Input);
    }
    if (RetOldValC) {
      Constant *Input;
      if (!ArgRWS.isNull())
        Input = dyn_cast<Constant>(ArgRWS.Input);
      else
        Input = dyn_cast<Constant>(CI->getArgOperand(Arg->getArgNo()));
      if (Input) {
        // Get the Input constant to the same element type as RetOldValC.
        if (RetOldValC->getType()->getScalarType()
            != Input->getType()->getScalarType()) {
          Type *ElTy = RetOldValC->getType()->getScalarType();
          IGC_ASSERT(ElTy->getPrimitiveSizeInBits());
          Input = ConstantExpr::getBitCast(
              Input, IGCLLVM::FixedVectorType::get(
                         ElTy, Input->getType()->getPrimitiveSizeInBits() /
                                   ElTy->getPrimitiveSizeInBits()));
        }
        // Construct the constant that needs to be loaded.
        IGC_ASSERT(RetOldValC->getType()->getScalarType() == Input->getType()->getScalarType());
        auto LdConst = RetRWS.WrR.evaluateConstantWrRegion(RetOldValC, Input);
        // Create the ConstArgRetCallSite object.
        return new ConstArgRetCallSite(CI, LdConst, RetRWS.EndWr,
            RetRWS.getWrIndex());
      }
    }
  }

  // Case 2: The call arg is a legalized contiguous rdregion or copy of
  // non-constant, and there is no retval coalesced with it.
  if (RetRWS.isNull() && !ArgRWS.isNull() && CoalescedRetIdx < 0
      && !isa<Constant>(ArgRWS.Input)
      && !GenXIntrinsic::isReadPredefReg(ArgRWS.getInputUse()->get())) {
    // It is valid to indirect this arg only if one of these is true:
    // 1. the input to ArgRWS is not live over the call, or
    // 2. the coalesced live range for the arg is not written to inside the
    //    subroutine or anything it calls.
    if (CanCoalesceWithoutKill || !Pass->Liveness->getLiveRange(ArgRWS.Input)
            ->contains(Pass->Numbering->getNumber(CI)))
      return new IndirectArgCallSite(CI, ArgRWS.getInputUse(),
          ArgRWS.getRdIndex());
    vc::warn(CI->getContext(), "GenXArgIndirection",
             "Argument is region in value that is live over call", Arg);
    return nullptr;
  }

  // Case 3: The call arg is a legalized rdregion or copy of non-constant, and
  // the coalesced retval is a legalized wrregion or copy with the same region
  // and the same base register.
  if (!RetRWS.isNull() && !ArgRWS.isNull() && CoalescedRetIdx >= 0
      && !isa<Constant>(ArgRWS.Input)) {
    // Check the regions are the same.
    if (ArgRWS.RdR == RetRWS.WrR) {
      // Check the base registers are the same.
      if (Pass->Liveness->getLiveRange(ArgRWS.Input)
          == Pass->Liveness->getLiveRange(RetRWS.EndWr))
        return new IndirectArgRetCallSite(CI, ArgRWS.getInputUse(),
            RetRWS.EndWr, ArgRWS.getRdIndex());
    }
    vc::warn(CI->getContext(), "GenXArgIndirection",
             "Coalesced return value does not match argument", Arg);
    return nullptr;
  }

  // Case 4: No optimization for this call site, and cannot even indirect it
  // unless either there is a coalesced retval, or the subroutine arg's LR is
  // not written inside the subroutines, or the call arg is killed at the call.
  if (!CanCoalesceWithoutKill && !ArgRWS.isNull() && !isa<Constant>(ArgRWS.Input)
        && Pass->Liveness->getLiveRange(ArgRWS.Input)
          ->contains(Pass->Numbering->getNumber(CI))) {
    vc::warn(CI->getContext(), "GenXArgIndirection",
             "Argument is value that is live over call", Arg);
    return nullptr;
  }

  // Case 5: No optimization for this call site (but it can still be indirected
  // if some other call site would get optimized).
  return new NoOptCallSite(CI);
}

/***********************************************************************
 * getIndirectAlignment : get worst-case alignment of indices if we indirect
 *                        this arg and retval
 */
Alignment SubroutineArg::getIndirectAlignment(unsigned GRFWidth) const {
  Alignment Align(genx::log2(GRFWidth), 0); // best case is GRF aligned
  for (auto csi = CallSites.begin(), cse = CallSites.end();
      csi != cse; ++csi) {
    auto CS = *csi;
    Value *Index = CS->getIndex();
    if (!Index)
      continue;
    Align = Align.merge(Pass->AI->get(Index));
  }
  return Align;
}

/***********************************************************************
 * gatherBalesToModify : check whether the arg can be indirected and
 *        gather the bales that need modifying
 *
 * Enter:   Align = the worst case alignment of the indirection
 *          this->BalesToModify = vector to populate
 *
 * Return:  true if can be indirected, with
 *            BalesToModify populated with bales that need indirecting
 */
bool GenXArgIndirection::gatherBalesToModify(LiveRange *ArgLR, Alignment Align)
{
  LLVM_DEBUG(dbgs() << "gatherBalesToModify: alignment " << Align << "\n");
  BalesToModify.clear();
  // We call SubroutineArg::gatherBalesToModify for each subroutine that has
  // an arg in this live range. Just gathering bales for all instructions and
  // args in the live range in one go would not work, because there might be a
  // call site where the call arg is coalesced, and we would end up indirecting
  // it and other things it is coalesced with.
  for (auto si = SubrArgs.begin(), se = SubrArgs.end(); si != se; ++si)
    si->gatherBalesToModify(Align);
  // Check the bales to see if we can legally indirect accesses to any value in
  // ArgLR (i.e. the arg, the retval, and anything coalesced with it) by doing
  // a dry run of modifying them.
  for (auto btmi = BalesToModify.begin(), btme = BalesToModify.end();
      btmi != btme; ++btmi) {
    Bale B;
    Baling->buildBale(*btmi, &B);
    if (!checkIndirectBale(&B, ArgLR, Align)) {
      // Failure. For error reporting, get the arg for the function in which the
      // failure occurred.
      auto *Inst = B.getHead()->Inst;
      IGC_ASSERT(Inst);
      Argument *Arg = getArgForFunction(ArgLR, Inst->getFunction());
      vc::warn(Inst->getContext(), "GenXArgIndirection",
               "Use of argument cannot be indirected", Arg);
      return false;
    }
  }
  return true;
}

/***********************************************************************
 * gatherBalesToModify : gather the bales that need modifying for this one
 *    subroutine arg
 *
 * Enter:   Align = the worst case alignment of the indirection
 *          Pass->BalesToModify = vector to populate
 *
 * Return:  BalesToModify populated with bales that need indirecting
 */
void SubroutineArg::gatherBalesToModify(Alignment Align)
{
  std::set<Instruction *> BalesSeen;
  for (auto vi = ArgLR->value_begin(), ve = ArgLR->value_end(); vi != ve; ++vi) {
    Value *V = vi->getValue();
    if (Pass->Liveness->isUnifiedRet(V))
      continue; // ignore unified ret
    if (auto Inst = dyn_cast<Instruction>(V)) {
      if (Inst->getFunction() != F)
        continue; // ignore instruction in wrong function
      // Add the def to the list of bales that will need modifying, unless
      // it is a phi node or coalesced bitcast or insert/extract in struct
      // or a non-intrinsic call.
      if (!isa<PHINode>(Inst) &&
          (!isa<BitCastInst>(Inst) ||
           Pass->Liveness->getLiveRange(Inst->getOperand(0)) != ArgLR) &&
          !isa<InsertValueInst>(Inst) && !isa<ExtractValueInst>(Inst) &&
          (!isa<CallInst>(Inst) || vc::isAnyNonTrivialIntrinsic(Inst)))
        if (BalesSeen.insert(Inst).second)
          Pass->BalesToModify.push_back(Inst);
    } else if (V != Arg)
      continue; // ignore arg in wrong function
    for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui) {
      auto User = cast<Instruction>(ui->getUser());
      if (auto CI = dyn_cast<CallInst>(User)) {
        Function *CF = CI->getCalledFunction();
        if (!vc::isAnyNonTrivialIntrinsic(CF) && !vc::requiresStackCall(CF)) {
          // Non-intrinsic call. Ignore. (A call site using an arg being
          // indirected gets handled differently.)
          // Cannot indirect if there is a stack call. Do not ignore stack
          // calls here and add them to BalesToModify. In checkIndirectBale
          // report that such bale cannot be indirected. This method is
          // confusing, must be improved.
          continue;
        }
      } else {
        if (isa<StructType>(User->getType()))
          continue; // Ignore call with multiple retvals, or insert used to do
                    // multiple retvals
        if (isa<ExtractValueInst>(User))
          continue; // Ignore extract in struct used to do multiple retvals
        if (isa<PHINode>(User))
          continue; // Ignore phi nodes
        if (isa<ReturnInst>(User))
          continue; // Ignore return instruction
        if (isa<BitCastInst>(User) && Pass->Liveness->getLiveRange(User) == ArgLR)
          continue; // Ignore coalesced bitcast
      }
      // Add the head of the bale to the list of bales that will need modifying.
      auto UserHead = Pass->Baling->getBaleHead(User);
      if (BalesSeen.insert(UserHead).second)
        Pass->BalesToModify.push_back(UserHead);
    }
  }
}

/***********************************************************************
 * checkIndirectBale : check if a bale can be indirected
 *
 * Enter:   B = bale to check
 *          ArgLR = live range of values that need to be indirected
 *          Align = alignment of index being introduced
 *
 * Return:  true if can be indirected
 */
bool GenXArgIndirection::checkIndirectBale(Bale *B, LiveRange *ArgLR,
    Alignment Align)
{
  auto MainInst = B->getMainInst();
  if (MainInst) {
    // Check for things about the main instruction that stop us indexing
    // operand(s) or result in this bale.
    if (MainInst->Inst->getType()->getPrimitiveSizeInBits() / genx::ByteBits >
            ST->getGRFByteSize() &&
        !ST->hasIndirectGRFCrossing()) {
      // An execution size bigger than 1 GRF disqualifies the main
      // instruction on <= BDW.
      LLVM_DEBUG(dbgs() << "execution size bigger than GRF\n");
      return false;
    }
    unsigned IID = vc::getAnyIntrinsicID(MainInst->Inst);
    if (vc::isAnyNonTrivialIntrinsic(IID)) {
      auto IntrInfo = GenXIntrinsicInfo(IID);
      // Cannot indirect a raw or direct only operand.
      bool RawOrDirectOnly =
          std::any_of(MainInst->Inst->op_begin(), MainInst->Inst->op_end(),
                      [&IntrInfo](Use &U) {
                        auto AI = IntrInfo.getArgInfo(U.getOperandNo());
                        return AI.isRaw() || AI.isDirectOnly();
                      });
      if (RawOrDirectOnly) {
        LLVM_DEBUG(dbgs() << *MainInst->Inst
                          << "\n\tintrinsic has raw or direct only operand\n");
        return false;
      }
      if (IntrInfo.getRetInfo().isRaw()) {
        LLVM_DEBUG(dbgs() << *MainInst->Inst
                          << "\n\tintrinsic with raw return value\n");
        return false;
      }
    } else if (auto *CI = dyn_cast<CallInst>(MainInst->Inst)) {
      auto *Callee = CI->getCalledFunction();
      IGC_ASSERT_MESSAGE(vc::requiresStackCall(Callee),
                         "Expect a stack call to stop indirection. See "
                         "SubroutineArg::gatherBalesToModify");
      LLVM_DEBUG(dbgs() << *CI << "\n\tcalls stack call function\n");
      return false;
    }
  }
  // Check the rdregion(s) and wrregion.
  for (auto bi = B->begin(), be = B->end(); bi != be; ++bi) {
    switch (bi->Info.Type) {
      case BaleInfo::WRREGION:
        // Check wrregion if its result is coalesced with arg.
        if (Liveness->getLiveRange(bi->Inst) == ArgLR) {
          vc::Region R = makeRegionFromBaleInfo(bi->Inst, bi->Info);
          if (R.Indirect)
            break; // already indirect
          // Fake up scalar indirect index for the benefit of
          // getLegalRegionSizeForTarget. It doesn't matter what the value is,
          // as long as it is scalar.
          R.Indirect = bi->Inst->getOperand(
              GenXIntrinsic::GenXRegion::WrIndexOperandNum);
          if (R.NumElements != getLegalRegionSizeForTarget(
                                   *ST, R,
                                   /*Idx*/ 0,
                                   /*Allow2D=*/false,
                                   /*UseRealIdx=*/true,
                                   /*InputNumElements=*/UINT_MAX, Align)) {
            LLVM_DEBUG(dbgs() << "wrregion cannot be indirected: " << R << "\n");
            return false;
          }
        }
        break;
      case BaleInfo::RDREGION:
        // Check rdregion if its input is coalesced with arg.
        if (Liveness->getLiveRange(bi->Inst->getOperand(0)) == ArgLR) {
          Region R = makeRegionFromBaleInfo(bi->Inst, bi->Info);
          if (R.Indirect)
            break; // already indirect
          // Fake up scalar indirect index for the benefit of
          // getLegalRegionSizeForTarget. It doesn't matter what the value is,
          // as long as it is scalar.
          R.Indirect = bi->Inst->getOperand(
              GenXIntrinsic::GenXRegion::RdIndexOperandNum);
          if (R.NumElements != getLegalRegionSizeForTarget(
                                   *ST, R,
                                   /*Idx*/ 0,
                                   /*Allow2D=*/true,
                                   /*UseRealIdx=*/true,
                                   /*InputNumElements=*/UINT_MAX, Align)) {
            LLVM_DEBUG(dbgs() << "rdregion cannot be indirected: " << R << "\n";
                       dbgs() << getLegalRegionSizeForTarget(
                                     *ST, R, /*Idx*/ 0,
                                     /*Allow2D=*/true,
                                     /*UseRealIdx=*/true,
                                     /*InputNumElements=*/UINT_MAX, Align)
                              << "\n");
            return false;
          }
        }
        break;
      default:
        break;
    }
  }
  return true;
}

/***********************************************************************
 * addAddressArg : for this subroutine, replace the Function with a new
 *    one with an extra address arg, and modify all call sites
 *
 * This sets this->NewFunc, and modifies this->Arg to the argument in the
 * new function.
 *
 * Return: a pair containing pointer to the old Arg and created AddressArg.
 */
std::pair<Value *, Value *> SubroutineArg::addAddressArg() {
  // Create the new function type.
  auto FTy = F->getFunctionType();
  SmallVector<Type *, 4> ArgTys;
  for (unsigned i = 0, e = FTy->getNumParams(); i != e; ++i)
    ArgTys.push_back(FTy->getParamType(i));
  ArgTys.push_back(Type::getInt16Ty(F->getContext()));
  FTy = FunctionType::get(FTy->getReturnType(), ArgTys, false);
  // Create the new function.
  NewFunc = Function::Create(FTy, F->getLinkage(), F->getName());
  vc::transferNameAndCCWithNewAttr(F->getAttributes(), *F, *NewFunc);
  vc::transferDISubprogram(*F, *NewFunc);
  F->getParent()->getFunctionList().insert(F->getIterator(), NewFunc);
  // Set the new function's number to the same as the old function.
  Pass->Numbering->setNumber(NewFunc, Pass->Numbering->getNumber(F));
  // Move the original function's unified return value across to the new
  // function.
  Pass->Liveness->moveUnifiedRet(F, NewFunc);
  // The Function itself has a live range to represent the ranges of the
  // subroutine itself and everything it calls. Change the Function in that
  // live range.
  Pass->Liveness->replaceValue(F, NewFunc);
  // Populate arrays OldArgs (the original func's args) and NewArgs (the new
  // func's args).
  SmallVector<Argument *, 4> OldArgs, NewArgs;
  for (auto ai = F->arg_begin(), ae = F->arg_end(); ai != ae; ++ai)
    OldArgs.push_back(&*ai);
  for (auto ai = NewFunc->arg_begin(), ae = NewFunc->arg_end(); ai != ae; ++ai)
    NewArgs.push_back(&*ai);
  // For the original args, change uses to use the new args instead. Also
  // change the old arg's live range to have the new arg instead.
  for (unsigned ArgNum = 0; ArgNum != OldArgs.size(); ++ArgNum) {
    NewArgs[ArgNum]->setName(OldArgs[ArgNum]->getName());
    OldArgs[ArgNum]->replaceAllUsesWith(NewArgs[ArgNum]);
    Pass->Liveness->replaceValue(OldArgs[ArgNum], NewArgs[ArgNum]);
  }
  // Change the Arg in the current SubroutineArg, and save the address arg.
  Value *OldArg = Arg;
  Arg = NewArgs[Arg->getArgNo()];
  AddressArgument = NewArgs.back();
  // Give the address arg a live range, and mark that it needs calculating.
  auto LR = Pass->Liveness->getOrCreateLiveRange(AddressArgument);
  LR->setCategory(vc::RegCategory::Address);
  Pass->LRsToCalculate.push_back(LR);
  // Set the name of the new address arg.
  NewArgs[OldArgs.size()]->setName(Arg->getName() + ".addr");
  // Move the function code across.
  IGCLLVM::splice(NewFunc, NewFunc->begin(), F);

  return std::make_pair(OldArg, AddressArgument);
}

/***********************************************************************
 * fixCallSites : fix up a call to the subroutine, so it calls the new
 *    function instead and passes the extra address arg
 *
 * For each call site, this calls the process() method on the object of a
 * subclass of ArgIndCallSite set up by createCallSite(). That returns the extra
 * address arg, which this function then uses to create a replacement call
 * instruction.
 */
void SubroutineArg::fixCallSites()
{
  for (auto csi = CallSites.begin(), cse = CallSites.end(); csi != cse; ++csi) {
    auto CS = *csi;
    LLVM_DEBUG(dbgs() << "  fixCallSites: ["
                      << Pass->Numbering->getNumber(CS->CI) << "] " << *CS
                      << "\n");
    // Process the call site.
    // Create the replacement call instruction, with an added address arg that
    // for now we set to undef. We do this first so that process() called below
    // can modify the arg being indirected such that the eraseUnusedTree erases
    // the rd-wr sequence that sets up the arg in the old call.
    SmallVector<Value *, 4> Args;
    for (unsigned oi = 0, oe = IGCLLVM::getNumArgOperands(CS->CI); oi != oe; ++oi)
      Args.push_back(CS->CI->getArgOperand(oi));
    Args.push_back(UndefValue::get(Type::getInt16Ty(CS->CI->getContext())));
    CallInst *OldCI = CS->CI;
    CS->CI = CallInst::Create(NewFunc, Args, "", OldCI);
    CS->CI->takeName(OldCI);
    CS->CI->setDebugLoc(OldCI->getDebugLoc());
    Pass->Numbering->setNumber(CS->CI, Pass->Numbering->getNumber(OldCI));
    Pass->Numbering->setStartNumber(CS->CI,
                                    Pass->Numbering->getStartNumber(OldCI));
    // Get the subclass of ArgIndCallSite to do its processing, returning the
    // extra address arg for the call.
    Value *AddressArg = CS->process(Pass, this);
    LLVM_DEBUG(dbgs() << "    AddressArg is " << AddressArg->getName() << "\n");
    if (!isa<UndefValue>(AddressArg)) {
      // Create a live range for the address arg, and ensure it is recalculated.
      LiveRange *AddressArgLR = Pass->Liveness->getOrCreateLiveRange(AddressArg);
      AddressArgLR->setCategory(vc::RegCategory::Address);
      Pass->LRsToCalculate.push_back(AddressArgLR);
    }
    // Use the address arg in the new call.
    CS->CI->setOperand(Args.size() - 1, AddressArg);
    // Replace the old call with the new one, and erase the old one. We use
    // eraseUnusedTree so that any rd-wr sequence for the indirected arg is also
    // erased.
    OldCI->replaceAllUsesWith(CS->CI);
    Pass->Liveness->replaceValue(OldCI, CS->CI);
    Pass->Liveness->eraseUnusedTree(OldCI);
  }
}

/***********************************************************************
 * CallerIndirectingCallSite::process : arg indirection processing for a call
 *    site in a subroutine that is itself indirecting the arg
 *
 * Return:  the address arg that needs to be passed to the call
 */
Value *CallerIndirectingCallSite::process(GenXArgIndirection *Pass,
      SubroutineArg *SubrArg)
{
  return CallerSubrArg->AddressArgument;
}

/***********************************************************************
 * NoOptCallSite::process : arg indirection processing for a call site where
 *    no optimization is possible, but we can still indirect
 *
 * Return:  the address arg that needs to be passed to the call
 */
Value *NoOptCallSite::process(GenXArgIndirection *Pass, SubroutineArg *SubrArg)
{
  unsigned InsertNumber = Pass->Numbering->getArgIndirectionNumber(
      CI, IGCLLVM::getNumArgOperands(CI) - 1, 0);
  Instruction *InsertBefore = CI;
  Type *I16Ty = Type::getInt16Ty(CI->getContext());
  // If the arg is undef, we can just use an undef address.
  if (isa<UndefValue>(CI->getArgOperand(SubrArg->Arg->getArgNo())))
    return UndefValue::get(I16Ty);
  // Create a convert.addr of index 0, just before the call with the number of
  // the arg pre-copy site for the new address argument that will be added.
  auto Conv = createConvertAddr(ConstantInt::get(I16Ty, 0), 0,
      SubrArg->Arg->getName() + ".indirect", InsertBefore);
  Conv->setDebugLoc(CI->getDebugLoc());
  Pass->Numbering->setNumber(Conv, InsertNumber);
  // Tell GenXLiveness the base register for this address register. The normal
  // mechanism of tracing through to a user of the address does not work for an
  // indirected arg.
  Pass->Liveness->setArgAddressBase(Conv,
      CI->getArgOperand(SubrArg->Arg->getArgNo()));
  // If the live range of the input does not reach over the call, add a
  // use of it (an unused bitcast) after the call and recalculate the
  // live range.
  unsigned CINumber = Pass->Numbering->getNumber(CI);
  Value *Input = CI->getOperand(SubrArg->Arg->getArgNo());
  LiveRange *InputLR = Pass->Liveness->getLiveRange(Input);
  if (!InputLR->contains(CINumber)) {
    auto BC = CastInst::Create(Instruction::BitCast, Input, Input->getType(),
        Input->getName() + ".dummy_use_for_indirection", CI->getNextNode());
    Pass->Liveness->setLiveRange(BC, InputLR);
    Pass->Numbering->setNumber(BC, CINumber + 1);
    Pass->LRsToCalculate.push_back(InputLR);
  }
  return Conv;
}

/***********************************************************************
 * ConstArgRetCallSite::process : arg indirection processing for a call site
 *      where the arg is constant (including undef) and the arg is coalesced
 *      with a retval that is used only in a legalized wrregion whose "old
 *      value" input is constant.
 *
 * Return:  the address arg that needs to be passed to the call
 */
Value *ConstArgRetCallSite::process(GenXArgIndirection *Pass,
      SubroutineArg *SubrArg)
{
  // checkCallSites detected the situation where the arg is a constant
  // (probably a legalized constant load, detected by RdWrRegionSequence,
  // but also including undef), and the ret is wrregioned (probably
  // legalized) with a constant as the "old value" operand (including
  // undef).
  //
  // To handle this, we create a new constant load of the two constants
  // combined, before the call, to turn it back into the normal situation
  // of a legalized rdregion before the call and a legalized wrregion
  // after the call. (However we don't actually create the legalized
  // rdregion and wrregion.)
  //
  // The combined constant was created in checkCallSites, and in this object
  // it is LdConst.
  //
  // Any new instruction is inserted just before the call, and given the
  // instruction number of the address arg's pre-copy slot.
  Instruction *InsertBefore = CI;
  unsigned InsertNumber = Pass->Numbering->getArgIndirectionNumber(
        CI, IGCLLVM::getNumArgOperands(CI) - 1, 0);
  // Insert a load the constant. Bitcast it to the right type to replace
  // RetEndWr.
  SmallVector<Instruction *, 4> AddedInsts;
  ConstantLoader CL(LdConst, *Pass->ST, *Pass->DL, nullptr, &AddedInsts);
  auto LoadedConst = CL.loadBig(InsertBefore);
  IGC_ASSERT(LoadedConst);
  if (LoadedConst->getType() != RetEndWr->getType()) {
    LoadedConst = CastInst::Create(Instruction::BitCast, LoadedConst,
          RetEndWr->getType(), LoadedConst->getName() + ".bitcast",
          InsertBefore);
    AddedInsts.push_back(LoadedConst);
  }
  // An added instruction (from the constant load) is allocated a live range as
  // follows:
  // 1. An instruction with the right result size is assumed to be coalesceable
  // with the final result, and so put in the same live range as the retval's
  // wrregion.
  // 2. A (smaller) wrregion is assumed to be coalesceable with its "old value"
  // input, if that is an instruction.
  // 3. Otherwise it gets its own new live range.
  // A wrregion also needs to be marked as such in baling.
  auto RetValWrLR = Pass->Liveness->getLiveRange(RetEndWr);
  unsigned LoadedConstSize = LoadedConst->getType()->getPrimitiveSizeInBits();
  for (auto i = AddedInsts.begin(), e = AddedInsts.end(); i != e; ++i) {
    auto Inst = *i;
    Pass->Numbering->setNumber(Inst, InsertNumber);
    LiveRange *LR = nullptr;
    if (Inst->getType()->getPrimitiveSizeInBits() == LoadedConstSize)
      Pass->Liveness->setLiveRange(Inst, LR = RetValWrLR);
    if (GenXIntrinsic::isWrRegion(Inst)) {
      BaleInfo BI(BaleInfo::WRREGION);
      if (isa<Instruction>(Inst->getOperand(
              GenXIntrinsic::GenXRegion::NewValueOperandNum)))
        BI.setOperandBaled(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      Pass->Baling->setBaleInfo(Inst, BI);
      if (auto InInst = dyn_cast<Instruction>(
            Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum)))
        if (!LR)
          Pass->Liveness->setLiveRange(Inst,
              LR = Pass->Liveness->getLiveRange(InInst));
    }
    if (!LR) {
      LR = Pass->Liveness->getOrCreateLiveRange(Inst);
      LR->setCategory(vc::RegCategory::General);
    }
    Pass->LRsToCalculate.push_back(LR);
  }
  // Create the genx.convert.addr for the region of that constant load. We
  // use the offset of the retval's legalized wrregion.
  auto AddressArg = createConvertAddr(Index, 0,
      SubrArg->Arg->getName() + ".indirect", InsertBefore);
  AddressArg->setDebugLoc(CI->getDebugLoc());
  Pass->Numbering->setNumber(AddressArg, InsertNumber);
  // Tell GenXLiveness the base register for this address register.
  // The normal mechanism of tracing through to a user of the address
  // does not work for an indirected arg.
  Pass->Liveness->setArgAddressBase(AddressArg, LoadedConst);
  // Undef out the arg in the call, so the old code to load the constant (if
  // any) gets erased when the call is erased.
  unsigned CallArgNum = SubrArg->Arg->getArgNo();
  CI->setOperand(CallArgNum,
      UndefValue::get(CI->getOperand(CallArgNum)->getType()));
  // Replace uses of the (legalized) wrregion sequence with the newly inserted
  // constant load, then erase the sequence.
  Instruction *ToErase = RetEndWr;
  RetEndWr = nullptr; // need to do this as RetEndWr is an AssertingVH
  Pass->replaceAndEraseSequence(ToErase, LoadedConst);
  return AddressArg;
}

/***********************************************************************
 * IndirectArgCallSite::process : arg indirection processing for a call site
 *      where the arg is a legalized rdregion or copy, and there is no retval
 *      coalesced with it.
 *
 * Return:  the address arg that needs to be passed to the call
 */
Value *IndirectArgCallSite::process(GenXArgIndirection *Pass,
      SubroutineArg *SubrArg)
{
  // Any new instruction is inserted just before the call, and given the
  // instruction number of the address arg's pre-copy slot.
  Instruction *InsertBefore = CI;
  unsigned InsertNumber = Pass->Numbering->getArgIndirectionNumber(CI,
      IGCLLVM::getNumArgOperands(CI) - 1, 0);
  Value *AddressArg = nullptr;
  if (isa<Constant>(Index)) {
    // Constant index for the region. Add a convert.addr to load it into an
    // address register.
    auto Conv = createConvertAddr(Index, 0,
        SubrArg->Arg->getName() + ".indirect", InsertBefore);
    Conv->setDebugLoc(CI->getDebugLoc());
    Pass->Numbering->setNumber(Conv, InsertNumber);
    AddressArg = Conv;
  } else {
    // Variable index for the region. The index is already converted to an
    // address. It might be a genx.add.addr baled in to the rdregion; if so
    // unbale it.
    if (auto IndexInst = dyn_cast<Instruction>(Index))
      Pass->Baling->unbale(IndexInst);
    AddressArg = Index;
  }
  // Tell GenXLiveness the base register for this address register.
  // The normal mechanism of tracing through to a user of the address
  // does not work for an indirected arg.
  LiveRange *InputLR = Pass->Liveness->getLiveRange(*InputUse);
  // Add a use of the input (an unused bitcast) in case:
  // 1. the live range does not reach over the call (in which case we need to
  //    recalculate the live range after adding this use), or
  // 2. later on, another arg indirection removes a use, meaning that the live
  //    range no longer reaches over the call (in which case we don't need to
  //    recalculate the live range yet).
  auto BC = CastInst::Create(Instruction::BitCast, *InputUse,
      (*InputUse)->getType(),
      (*InputUse)->getName() + ".dummy_use_for_indirection",
      CI->getNextNode());
  Pass->Liveness->setLiveRange(BC, InputLR);
  Pass->Liveness->setArgAddressBase(AddressArg, BC);
  unsigned CINumber = Pass->Numbering->getNumber(CI);
  Pass->Numbering->setNumber(BC, CINumber + 1);
  if (!InputLR->contains(CINumber))
    Pass->LRsToCalculate.push_back(InputLR);
  // Undef out the arg in the call, so the old rd-wr sequence for the arg gets
  // erased when the call is erased.
  unsigned CallArgNum = SubrArg->Arg->getArgNo();
  CI->setOperand(CallArgNum,
      UndefValue::get(CI->getOperand(CallArgNum)->getType()));
  return AddressArg;
}

/***********************************************************************
 * IndirectArgRetCallSite::process : arg indirection processing for a call site
 *      where the arg is a legalized rdregion or copy, and the arg is coalesced
 *      with a retval that is used only in a legalized wrregion or copy.
 *
 * Return:  the address arg that needs to be passed to the call
 */
Value *IndirectArgRetCallSite::process(GenXArgIndirection *Pass,
      SubroutineArg *SubrArg)
{
  // Common code with IndirectArgCallSite above:
  auto AddressArg = IndirectArgCallSite::process(Pass, SubrArg);
  // Replace uses of the (legalized) wrregion sequence with the input to the
  // legalized rdregion before the call.
  Instruction *ToReplace = RetEndWr;
  RetEndWr = nullptr; // Needed as RetEndWr is an AssertingVH
  Pass->replaceAndEraseSequence(ToReplace, *InputUse);
  return AddressArg;
}

/***********************************************************************
 * GenXArgIndirection::replaceAndEraseSequence : replace uses of a wrregion
 *    sequence with a different value and erase the sequence, coping with
 *    different types due to bitcast
 *
 * Enter:   RetEndWr = end of wrregion sequence
 *          V = value to replace its uses with (not constant, so it has a
 *              live range)
 */
void GenXArgIndirection::replaceAndEraseSequence(Instruction *RetEndWr, Value *V)
{
  // See if the types are different due to some bitcasting somewhere. First
  // handle the case that V is the result of a bitcast whose input is the type
  // we want. We can just use that input.
  if (V->getType() != RetEndWr->getType())
    if (auto BC = dyn_cast<BitCastInst>(V))
      if (BC->getOperand(0)->getType() == RetEndWr->getType())
        V = BC->getOperand(0);
  // Then handle other different type cases by inserting our own bitcast.
  if (V->getType() != RetEndWr->getType()) {
    auto BC = CastInst::Create(Instruction::BitCast, V, RetEndWr->getType(),
        V->getName() + ".bitcast", RetEndWr);
    Numbering->setNumber(BC, Numbering->getNumber(RetEndWr));
    Liveness->setLiveRange(BC, Liveness->getLiveRange(V));
    V = BC;
  }
  // Replace uses and erase resulting tree of unused instructions.
  RetEndWr->replaceAllUsesWith(V);
  Liveness->eraseUnusedTree(RetEndWr);
}

/***********************************************************************
 * coalesceAddressArgs : for the new address arg, attempt to coalesce at
 *      each call site, inserting a copy on failure to coalesce
 */
void SubroutineArg::coalesceAddressArgs()
{
  LiveRange *AddressLR = Pass->Liveness->getLiveRange(AddressArgument);
  unsigned ArgNum = AddressArgument->getArgNo();
  for (unsigned csi = 0, cse = CallSites.size(); csi != cse; ++csi) {
    auto CS = CallSites[csi];
    Value *CallArg = CS->CI->getArgOperand(ArgNum);
    if (isa<UndefValue>(CallArg))
      continue;
    LiveRange *CallArgLR = Pass->Liveness->getLiveRange(CallArg);
    if (AddressLR == CallArgLR)
      continue;
    if (!Pass->Liveness->interfere(AddressLR, CallArgLR)) {
      // No interference -- we can coalesce.
      AddressLR = Pass->Liveness->coalesce(AddressLR, CallArgLR,
          /*DisallowCASC=*/true);
      continue;
    }
    // There is interference. This should not happen if the caller is another
    // subroutine where we are indirecting the arg -- the new address args
    // for each subroutine should coalesce together.
    LLVM_DEBUG(dbgs() << "Failed to coalesce:\n " << *AddressLR << "\n " << *CallArgLR << "\n");
    IGC_ASSERT_MESSAGE(!Pass->FuncMap[CS->CI->getFunction()],
                       "new address args should coalesce together");
    // We need to insert a copy, in the address arg's pre-copy slot. An address
    // copy is done with a genx.convert, even though it is not actually doing a
    // conversion.
    auto Copy =
        createConvert(CallArg, CallArg->getName() + ".coalescefail", CS->CI);
    Copy->setDebugLoc(CS->CI->getDebugLoc());
    Pass->Numbering->setNumber(
        Copy, Pass->Numbering->getArgPreCopyNumber(CS->CI, ArgNum, 0));
    // Add the new value in to AddressLR.
    Pass->Liveness->setLiveRange(Copy, AddressLR);
    CS->CI->setOperand(ArgNum, Copy);
  }
}

/***********************************************************************
 * replaceFunction : replace the old function with the new function
 *
 * This replaces the function in the FunctionGroup, and then erases the old
 * function.
 */
void SubroutineArg::replaceFunction()
{
  Pass->FGA->replaceFunction(F, NewFunc);
  F->eraseFromParent();
  F = NewFunc;
}

/***********************************************************************
 * indirectBale : modify a bale to be indirect
 *
 * Enter:   B = bale to modify
 *          ArgLR = live range of values that need to be indirected
 *          AddressArg = new argument for address
 *
 * On return, the bale struct is no longer valid.
 */
void GenXArgIndirection::indirectBale(Bale *B, LiveRange *ArgLR,
    Argument *AddressArg)
{
  // Indirect the head of the bale, if its result is in ArgLR.
  auto Inst = B->getHead()->Inst;
  if (Liveness->getLiveRange(Inst) == ArgLR) {
    if (B->getHead()->Info.Type == BaleInfo::WRREGION) {
      // wrregion: just modify the index to indirect it.
      indirectRegion(&Inst->getOperandUse(
            GenXIntrinsic::GenXRegion::WrIndexOperandNum), AddressArg, Inst);
    } else {
      // No wrregion: we need to add one, and ensure that the original
      // instruction is baled into it.
      Region R(Inst);
      R.Indirect = AddressArg;
      SmallVector<Use *, 4> Uses;
      for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui)
        Uses.push_back(&*ui);
      auto NewWr = R.createWrRegion(UndefValue::get(Inst->getType()), Inst,
                                    Inst->getName() + ".indirected",
                                    Inst->getNextNode(), Inst->getDebugLoc());
      Liveness->setLiveRange(NewWr, ArgLR);
      Liveness->removeValue(Inst);
      for (auto ui = Uses.begin(), ue = Uses.end(); ui != ue; ++ui)
        **ui = NewWr;
      BaleInfo BI(BaleInfo::WRREGION);
      BI.setOperandBaled(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      Baling->setBaleInfo(NewWr, BI);
    }
  }
  // Process operands in each instruction of the bale.
  for (auto bi = B->begin(), be = B->end(); bi != be; ++bi) {
    Inst = bi->Inst;
    for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi) {
      if (bi->Info.isOperandBaled(oi))
        continue; // Ignore within-bale operands
      if (!oi && bi->Info.Type == BaleInfo::WRREGION)
        continue; // Ignore "old value" input to wrregion
      Value *Opnd = Inst->getOperand(oi);
      if (Liveness->getLiveRangeOrNull(Opnd) != ArgLR)
        continue; // Not in ArgLR, does not need indirecting
      if (bi->Info.Type == BaleInfo::RDREGION
          && oi == GenXIntrinsic::GenXRegion::OldValueOperandNum) {
        // input to rdregion: just modify the index to indirect it.
        indirectRegion(&bi->Inst->getOperandUse(
              GenXIntrinsic::GenXRegion::RdIndexOperandNum), AddressArg, Inst);
      } else {
        // No rdregion: we need to add one, and ensure that it is baled in
        // to the original instruction.
        Region R(Opnd);
        R.Indirect = AddressArg;
        auto NewRd = R.createRdRegion(Opnd, Opnd->getName() + ".indirected",
            Inst, Inst->getDebugLoc());
        Inst->setOperand(oi, NewRd);
        BaleInfo BI = bi->Info;
        BI.setOperandBaled(oi);
        Baling->setBaleInfo(Inst, BI);
        BaleInfo NewRdBI(BaleInfo::RDREGION);
        Baling->setBaleInfo(NewRd, NewRdBI);
      }
    }
  }
}

/***********************************************************************
 * indirectRegion : convert a rdregion/wrregion index operand to indirect
 *
 * Enter:   U = the rdregion/wrregion index operand use
 *          AddressArg = the index to use
 *          InsertBefore = where to insert new instructions
 *
 * If the rdregion/wrregion already has a variable index, then we create an
 * instruction to remove its genx.convert.addr and add it to AddressArg with
 * genx.add.addr.
 */
void GenXArgIndirection::indirectRegion(Use *U, Value *AddressArg,
    Instruction *InsertBefore)
{
  Value *Addr = *U;
  if (auto CI = dyn_cast<Constant>(Addr)) {
    // Currently the index is constant.
    if (CI->isNullValue()) {
      *U = AddressArg;
      return;
    }
    // Create a genx.add.addr and give it an instruction number one less
    // than InsertBefore.
    auto NewAdd = createAddAddr(AddressArg, CI, "indirect.offset", InsertBefore);
    Numbering->setNumber(NewAdd, Numbering->getNumber(InsertBefore) - 1);
    *U = NewAdd;
    // If the constant is within offset range, bale the new genx.add.addr into
    // its user.
    if (GenXBaling::isBalableIndexAdd(NewAdd)) {
      auto User = cast<Instruction>(U->getUser());
      BaleInfo BI = Baling->getBaleInfo(User);
      BI.setOperandBaled(U->getOperandNo());
      Baling->setBaleInfo(User, BI);
    } else {
      // Otherwise, give it a live range, and mark it as needing calculating.
      auto LR = Liveness->getOrCreateLiveRange(NewAdd);
      LR->setCategory(vc::RegCategory::Address);
      LRsToCalculate.push_back(LR);
    }
    return;
  }
  // The index is already variable.
  // Trace back through add_addr instructions until we find one of:
  // 1. The convert_addr instruction set up by GenXCategory, and possibly
  //    commoned up by GenXAddressCommoning. We replace that with an
  //    add_addr instruction that adds the convert_addr's input to AddressArg.
  // or
  // 2. An Argument, so another user of the same address must have already
  //    found and replaced (1).
  for (;;) {
    if (isa<Argument>(Addr))
      return;
    auto IntrinsicID = GenXIntrinsic::getGenXIntrinsicID(Addr);
    switch (IntrinsicID) {
    case GenXIntrinsic::genx_add_addr:
      Addr = cast<Instruction>(Addr)->getOperand(0);
      continue;
    case GenXIntrinsic::genx_rdregioni:
      Addr = cast<Instruction>(Addr)->getOperand(
          GenXIntrinsic::GenXRegion::OldValueOperandNum);
      continue;
    case GenXIntrinsic::genx_convert_addr:
      // we've found what we wanted
      break;
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "unsupported instruction");
    }
    break;
  }
  IGC_ASSERT(GenXIntrinsic::getGenXIntrinsicID(Addr) ==
         GenXIntrinsic::genx_convert_addr);
  auto AddrInst = cast<Instruction>(Addr);
  auto AddrSrc = AddrInst->getOperand(0);
   // Create an add_addr to replace the convert_addr. It needs a live range with
  // ADDRESS category.
  auto NewAddAddr = createAddAddr(AddressArg, AddrSrc,
      AddrInst->getName() + ".indirectedaddr", AddrInst);
  NewAddAddr->setDebugLoc(AddrInst->getDebugLoc());
  Numbering->setNumber(NewAddAddr, Numbering->getNumber(AddrInst) - 1);
  AddrInst->replaceAllUsesWith(NewAddAddr);
  LiveRange *LR = Liveness->getOrCreateLiveRange(NewAddAddr);
  LR->setCategory(vc::RegCategory::Address);
  LRsToCalculate.push_back(LR);
  // AddrSrc (source of convert_addr) should get a live range as well
  LiveRange *SrcLR = Liveness->getOrCreateLiveRange(AddrSrc);
  SrcLR->setCategory(vc::RegCategory::General);
  LRsToCalculate.push_back(SrcLR);
  // remove the old convert_addr
  Liveness->eraseLiveRange(AddrInst);
  AddrInst->eraseFromParent();
}

/***********************************************************************
 * getArgForFunction : find the arg in a live range that belongs to a func
 */
Argument *GenXArgIndirection::getArgForFunction(LiveRange *LR, Function *F)
{
  for (auto vi = LR->value_begin(), ve = LR->value_end(); vi != ve; ++vi) {
    Value *V = vi->getValue();
    if (auto Arg = dyn_cast<Argument>(V))
      if (Arg->getParent() == F)
        return Arg;
  }
  return nullptr;
}

/***********************************************************************
 * getRetVal : get return value for possibly multi return value call
 *
 * Enter:   CI = call instruction
 *          RetNum = return value number
 *
 * Return:  the return value (which is either a CallInst or an
 *          ExtractValueInst), or 0 if unknown use, or undef if it is shown
 *          that the requested return value is never extracted from the struct
 */
Value *SubroutineArg::getRetVal(CallInst *CI, unsigned RetNum)
{
  auto ST = dyn_cast<StructType>(CI->getType());
  if (!ST) {
    IGC_ASSERT(!RetNum);
    return CI;
  }
  Value *RetVal = UndefValue::get(ST->getElementType(RetNum));
  for (auto ui = CI->use_begin(), ue = CI->use_end(); ui != ue; ++ui) {
    auto EVI = dyn_cast<ExtractValueInst>(ui->getUser());
    if (!EVI || EVI->getNumIndices() != 1)
      return nullptr; // unknown use
    if (EVI->getIndices()[0] == RetNum) {
      if (isa<UndefValue>(RetVal))
        RetVal = EVI;
      else
        return nullptr; // multiple extractelements of the same retval
    }
  }
  return RetVal;
}
