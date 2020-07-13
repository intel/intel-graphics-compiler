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
//
/// GenXCoalescing
/// --------------
///
/// The LLVM target independent code generator, used by most backends, has a
/// coalescing pass that runs after de-SSA of the machine IR and two-address
/// handling, and attempts to remove the added copies by coalescing values. It
/// also attempts to coalesce a value with a hardreg that it is copied to/from.
///
/// This GenX coalescing and copy insertion pass is a bit different, in that
/// it runs on LLVM IR, which must remain in SSA, and it attempts to coalesce
/// values to try and avoid adding the copy in the first place. In any phi node
/// or two address op where it fails to coalesce, it inserts a copy (and
/// coalesces the result of the copy into the result of the phi node or
/// two address op).
///
/// There are three different kinds of coalescing. Copy coalescing is done first,
/// then the other two are done together.
///
/// 1. Copy coalescing.
///
///    Generally there are no copy instructions in SSA, but we
///    can treat a bitcast as a copy (the operand and result can live in the
///    same register aliased in different registers), and an extractvalue is
///    treated as a copy to be coalesced, and the "inserted value" operand
///    and the corresponding element(s) of the result in an insertvalue are
///    treated as a copy to be coalesced.
///
///    Copy coalescing represents two values that are known to be identical
///    occupying the same register at the same time, thus it is possible even
///    if the two values interfere (are live at the same point). Because we
///    handle copy coalescing before any other kind of coalescing, it usually
///    succeeds.
///
///    This only works because we do copy coalescing first, so we know that
///    neither value that we want to copy coalesce has already undergone normal
///    or phi coalescing.
///
///    However there is a case when copy coalescing between two live ranges
///    LR1 and LR2 (each of which is possibly already copy coalesced) cannot be
///    allowed: when LR2 loops round and has a phi use in the same basic block
///    as a phi definition in LR1, where the phi use of LR2 is after the phi
///    definition of LR1. This can happen because LLVM IR does not attach any
///    meaning to the order of phi nodes, but the GenX backend does with its
///    instruction numbering.
///
///    This constraint on copy coalescing is embodied in the concept of
///    "copy-interference". The two live ranges LR1 and LR2 copy-interfere,
///    meaning they cannot be copy coalesced, if LR1 has a phi definition,
///    one of whose numbers is within LR2's live range.
///
/// 2. Normal coalescing
///
///    This arises where we have a two-address operation, that is, it has an
///    operand that needs to be in the same register as the result, because the
///    instruction represents a partial write operation. The main example of
///    this is wrregion, but there are also some shared function intrinsics
///    that need this.
///
///    Here, we gather all the possible coalesces (including the phi ones),
///    together with an estimate of the cost of failing to coalesce (due to
///    needing to insert a copy), and then sort them in cost order and process
///    them.
///
///    This kind of coalescing is possible only if the two live ranges do not
///    interfere. If coalescing fails, we need to insert a copy just before
///    the instruction, creating a new value with a very short live range
///    that can trivially be coalesced with the result of the original
///    instruction.
///
///    Some subkinds of normal coalescing are:
///
///    2a. call arg pre-copy
///
///        A call arg needs to be coalesced with or copied to the corresponding
///        function arg.
///
///        Unlike most other kinds of coalescing, if coalescing fails, the copy
///        insertion is delayed until later, so we can ensure that the copies
///        are in the same order as the args, as the live ranges were computed
///        on that basis.
///
///        Normally, call arg pre-copy coalescing occurs, like other normal
///        coalescing, if the two live ranges do not interfere. If this fails,
///        we can still do *call arg special coalescing* (CASC) of call arg A
///        and function arg B as long as both of the following are true:
///
///         i. B has not been normal coalesced into anything (which would be
///            in the subroutine or some other subroutine it calls), except
///            that B is allowed to be call arg pre-copy coalesced;
///
///        ii. For any other call site where the corresponding call arg is not
///            A, A does not interfere with it.
///
///        Call arg special coalescing allows call arg A and function arg B to
///        be in the same register, even if A is used after the call, as long
///        as that register is not already being used for a different value
///        in the subroutine, and as long as a different value for the call
///        arg is not used at a different call site where A is live.
///
///        **Note**: Call arg special coalescing is disabled, because it broke
///        a test and I never got round to investigating why. I don't even know
///        if it would be beneficial any more, given more recent changes to
///        liveness and coalescing.
///
///    2b. ret value pre-copy
///
///        At a ReturnInst, the return value operand needs to be coalesced with
///        or copied to the unified return value for the function. This is
///        handled mostly the same as a normal coalesce.
///
///    2c. ret value post-copy
///
///        After a CallInst for a subroutine call, the unified return value
///        needs to be coalesced with or copied to the result of the call. On
///        failure, the copy insertion is delayed until later.
///
/// 3. Phi coalescing
///
///    This is how we "de-SSA" the code. A phi incoming wants to coalesce with
///    the result of the phi node.
///
///    Again, this kind of coalescing is possible only if the two live ranges
///    do not interfere. (A phi incoming can never interfere with its phi
///    result, but earlier coalescing could make them now interfere.) If
///    coalescing fails, we need to insert a copy at the end of the incoming
///    predecessor basic block. In fact we defer the copy insertion from failed
///    phi coalescing to the end, because we need to make sure the inserted
///    copies are in the same order as the phi nodes, as that is the basis on
///    which the live ranges were constructed.
///
///    After phi coalescing, the LLVM IR is still in SSA form, but the phi
///    coalescing, and the copies inserted where phi coalescing failed, mean
///    that it is trivial to transform into non-SSA vISA code: generate code for
///    the phi copies, and ignore the phi nodes themselves because they are
///    completely coalesced.
///
/// Kernel argument copying
/// ^^^^^^^^^^^^^^^^^^^^^^^
///
/// The kernel argument offsets (i.e. where kernel arguments appear in the GRF
/// on entry to the kernel) are set in a very early pass just after Clang
/// codegen. This sets offsets and packs holes in a way that is specific to the
/// language being compiled and its contract with its runtime.
///
/// However, when we get here, we may find that a live range that contains a
/// kernel argument has an alignment requirement that the offset from
/// earlier does not comply with.
///
/// So an extra function of this pass, after doing the coalescing, is to spot
/// this case, where a kernel argument has an offset that is not aligned enough,
/// and insert an extra copy at the start of the function.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_COALESCING"

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXNumbering.h"
#include "GenXRegion.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include <algorithm>
#include <vector>

using namespace llvm;
using namespace genx;

static cl::opt<unsigned> GenXShowCoalesceFailThreshold("genx-show-coalesce-fail-threshold", cl::init(UINT_MAX), cl::Hidden,
                                      cl::desc("GenX size threshold (bytes) for showing coalesce fails."));

// Diagnostic information for error/warning relating fast-composition.
class DiagnosticInfoFastComposition : public DiagnosticInfo {
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
  DiagnosticInfoFastComposition(Instruction *Inst,
    const Twine &Desc, DiagnosticSeverity Severity = DS_Error);
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};
int DiagnosticInfoFastComposition::KindID = 0;

namespace {

  // Candidate : description of a coalescing candidate
  struct Candidate {
    genx::SimpleValue Dest;
    Use *UseInDest;
    unsigned SourceIndex;
    unsigned Priority;
    unsigned Serial;
    Candidate(SimpleValue Dest, Use *UseInDest, unsigned SourceIndex,
          unsigned Priority, unsigned Serial)
      : Dest(Dest), UseInDest(UseInDest), SourceIndex(SourceIndex),
        Priority(Priority), Serial(Serial) {}
    bool operator<(const Candidate &C2) const {
      if (Priority != C2.Priority)
        return Priority > C2.Priority;
      // Make the sort order preserving for equal priority, to get consistent
      // results across different runs.
      return Serial < C2.Serial;
    }
  };

  struct PhiCopy {
    PHINode *Phi;
    unsigned IncomingIdx;
    PhiCopy(PHINode *Phi, unsigned IncomingIdx)
        : Phi(Phi), IncomingIdx(IncomingIdx) {}
  };

  // GenX coalescing pass
  class GenXCoalescing : public FunctionGroupPass {
  private:
    const GenXSubtarget *ST;
    GenXBaling *Baling;
    GenXLiveness *Liveness;
    GenXNumbering *Numbering;
    DominatorTreeGroupWrapperPass *DTWrapper;
    std::vector<Candidate> CopyCandidates;
    std::vector<Candidate> NormalCandidates;
    std::vector<CallInst*> Callables;
  public:
    static char ID;
    explicit GenXCoalescing() : FunctionGroupPass(ID) {}
    virtual StringRef getPassName() const { return "GenX coalescing and copy insertion"; }
    void getAnalysisUsage(AnalysisUsage &AU) const {
      FunctionGroupPass::getAnalysisUsage(AU);
      AU.addRequired<GenXLiveness>();
      AU.addRequired<GenXGroupBaling>();
      AU.addRequired<GenXNumbering>();
      AU.addRequired<DominatorTreeGroupWrapperPass>();
      AU.addPreserved<DominatorTreeGroupWrapperPass>();
      AU.addPreserved<GenXGroupBaling>();
      AU.addPreserved<GenXLiveness>();
      AU.addPreserved<GenXModule>();
      AU.addPreserved<GenXNumbering>();
      AU.addPreserved<FunctionGroupAnalysis>();
      AU.setPreservesCFG();
    }
    bool runOnFunctionGroup(FunctionGroup &FG);
    // createPrinterPass : get a pass to print the IR, together with the GenX
    // specific analyses
    virtual Pass *createPrinterPass(raw_ostream &O, const std::string &Banner) const
    { return createGenXGroupPrinterPass(O, Banner); }
  private:
    void recordCandidates(FunctionGroup *FG);
    unsigned getPriority(Type *Ty, BasicBlock *BB);
    // Various permutations of the function to record a coalescing candidate.
    void recordCopyCandidate(Value *Dest, Use *UseInDest, unsigned Priority) {
      recordCandidate(SimpleValue(Dest), UseInDest, 0, Priority,
                      &CopyCandidates);
    }
    void recordCopyCandidate(SimpleValue Dest, Use *UseInDest,
                             unsigned SourceIndex, unsigned Priority) {
      recordCandidate(Dest, UseInDest, SourceIndex, Priority, &CopyCandidates);
    }
    void recordNormalCandidate(Instruction *Dest, Use *UseInDest,
                               unsigned Priority) {
      recordCandidate(SimpleValue(Dest), UseInDest, 0, Priority,
                      &NormalCandidates);
    }
    void recordNormalCandidate(SimpleValue Dest, Use *UseInDest,
                               unsigned SourceIndex, unsigned Priority) {
      recordCandidate(Dest, UseInDest, SourceIndex, Priority,
                      &NormalCandidates);
    }
    void recordPhiCandidate(Value *Dest, Use *UseInDest, unsigned Priority) {
      recordCandidate(SimpleValue(Dest), UseInDest, 0, Priority,
                      &NormalCandidates);
    }
    void recordCandidate(SimpleValue Dest, Use *UseInDest, unsigned SourceIndex,
                         unsigned Priority, std::vector<Candidate> *Candidates);
    void recordCallCandidates(FunctionGroup *FG);
    void recordCallArgCandidates(Value *Dest, unsigned ArgNum,
                                 ArrayRef<Instruction *> Insts);
    // Functions for processing coalecing candidates.
    void processCopyCandidate(Candidate *Cand) {
      processCandidate(Cand, true /*IsCopy*/);
    }
    void processCandidate(Candidate *Cand, bool IsCopy = false);
    void processPhiNodes(FunctionGroup *FG);
    void analysePhiCopies(PHINode *Phi, std::vector<PhiCopy> &ToProcess);
    void processPhiCopy(PHINode *Phi, unsigned Inc,
                        std::vector<PHINode *> &Phis);
    void processPhiBranchingJoinLabelCopy(PHINode *Phi, unsigned Inc,
                                          std::vector<PHINode *> &Phis);
    PHINode *copyNonCoalescedPhi(PHINode *PhiPred, PHINode *PhiSucc);
    void processCalls(FunctionGroup *FG);
    void processKernelArgs(FunctionGroup *FG);
    void coalesceOutputArgs(FunctionGroup *FG);
    void coalesceCallables();
    void coalesceGlobalLoads(FunctionGroup *FG);
    Instruction *insertCopy(SimpleValue Input, LiveRange *LR,
                            Instruction *InsertBefore, StringRef Name,
                            unsigned Number);
    Instruction *insertIntoStruct(Type *Ty, unsigned FlattenedIndex,
                                  Value *OldStruct, Instruction *NewVal,
                                  Instruction *InsertBefore);
    void showCoalesceFail(SimpleValue V, const DebugLoc &DL, const char *Intro,
                          LiveRange *DestLR, LiveRange *SourceLR);
    // Helpers
    DominatorTree *getDomTree(Function *F) { return DTWrapper->getDomTree(F); }
  };

} // end anonymous namespace

char GenXCoalescing::ID = 0;
namespace llvm {
void initializeGenXCoalescingPass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXCoalescing, "GenXCoalescing", "GenXCoalescing", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBaling)
INITIALIZE_PASS_DEPENDENCY(GenXLiveness)
INITIALIZE_PASS_DEPENDENCY(GenXNumbering)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPass);
INITIALIZE_PASS_END(GenXCoalescing, "GenXCoalescing", "GenXCoalescing", false, false)

FunctionGroupPass *llvm::createGenXCoalescingPass() {
  initializeGenXCoalescingPass(*PassRegistry::getPassRegistry());
  return new GenXCoalescing();
}

/***********************************************************************
 * runOnFunctionGroup : run the coalescing pass for this FunctionGroup
 */
bool GenXCoalescing::runOnFunctionGroup(FunctionGroup &FG)
{
  // Get analyses that we use and/or modify.
  auto P = getAnalysisIfAvailable<GenXSubtargetPass>();
  ST = P ? P->getSubtarget() : nullptr;
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Numbering = &getAnalysis<GenXNumbering>();
  DTWrapper = &getAnalysis<DominatorTreeGroupWrapperPass>();

  // Coalesce all global loads prior to normal coalescing.
  coalesceGlobalLoads(&FG);

  // Record all the coalescing candidates except the call arg and return
  // value pre-copy ones.
  recordCandidates(&FG);

  // Process the copy coalescing candidates.
  for (unsigned i = 0; i != CopyCandidates.size(); ++i)
    processCopyCandidate(&CopyCandidates[i]);

  // Record the call arg and return value pre-copy candidates.
  recordCallCandidates(&FG);

  // Sort the array of normal coalescing candidates (including phi ones) then
  // process them.
  std::sort(NormalCandidates.begin(), NormalCandidates.end());
  for (unsigned i = 0; i != NormalCandidates.size(); ++i)
    processCandidate(&NormalCandidates[i]);

  // Now scan all phi nodes again, inserting copies where necessary. Doing
  // them in one go here ensures that the copies appear in the predecessor
  // blocks in the same order as the phi nodes, which is the basis on which
  // we computed live ranges.
  processPhiNodes(&FG);

  // Scan all the calls, inserting copies where necessary for call arg
  // pre-copies and return value pre- and post-copies. Doing them in one go
  // here ensures that the copies appear in the order that live range
  // computation assumed they would appear. Also, for call arg and return
  // value pre-copies, a single coalesce candidate is shared across multiple
  // calls/returns using the same LR, so we need this separate scan to find
  // the calls/returns.
  processCalls(&FG);

  // Add a copy for each kernel arg that is not aligned enough.
  processKernelArgs(&FG);
  coalesceCallables();
  coalesceOutputArgs(&FG);

  CopyCandidates.clear();
  NormalCandidates.clear();
  Callables.clear();
  return true;
}

/***********************************************************************
 * recordCandidates : record all the coalescing candidates from code
 *
 * This does not record call arg pre-copy or ret value pre-copy candidates.
 * That is done in recordCallCandidates.
 */
void GenXCoalescing::recordCandidates(FunctionGroup *FG)
{
  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (Function::iterator fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        Instruction *Inst = &*bi;
        if (PHINode *Phi = dyn_cast<PHINode>(Inst)) {
          // Phi node. For each incoming, record a phi candidate, unless it is a
          // registerless value (EM/RM).
          // If the incoming block is a branching join label block, then we
          // cannot insert any phi copies there, so give the coalescing
          // candidate a high priority to ensure it gets coalesced first.
          if (Liveness->getLiveRange(Phi)->getCategory()
              < RegCategory::NUMREALCATEGORIES) {
            for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i) {
              auto IncomingBlock = Phi->getIncomingBlock(i);
              unsigned Priority = getPriority(Phi->getType(), IncomingBlock);
              if (GotoJoin::isBranchingJoinLabelBlock(IncomingBlock))
                Priority = UINT_MAX;
              recordPhiCandidate(Phi, &Phi->getOperandUse(i), Priority);
            }
          }
        } else if (IGCLLVM::CallInst *CI = dyn_cast<IGCLLVM::CallInst>(Inst)) {
          if (!GenXIntrinsic::isAnyNonTrivialIntrinsic(CI)) {
            if (CI->isInlineAsm()) {
              InlineAsm *IA = cast<InlineAsm>(CI->getCalledValue());
              // Do not process if no constraints provided or it's baled
              // (the coalescing actually needs to be done at the wrregion).
              if (IA->getConstraintString().empty() || Baling->isBaled(CI))
                continue;
              unsigned NumOutputs = genx::getInlineAsmNumOutputs(CI);
              auto ConstraintsInfo = genx::getGenXInlineAsmInfo(CI);
              // we need to coalesce if there is a '+' modifier
              // because those operands are tied and have to be in the same
              // registers
              for (unsigned ArgNo = 0; ArgNo < ConstraintsInfo.size();
                   ArgNo++) {
                auto &Info = ConstraintsInfo[ArgNo];
                if (!Info.isOutput() || !Info.hasMatchingInput())
                  continue;
                unsigned ActualIdx = Info.getMatchingInput() - NumOutputs;
                auto OpInst = dyn_cast<Instruction>(CI->getOperand(ActualIdx));
                if (!OpInst || Baling->isBaled(OpInst))
                  continue;
                Use *OpUse = &CI->getOperandUse(ActualIdx);
                if (isa<StructType>(CI->getType())) {
                  unsigned Priority = getPriority(
                      IndexFlattener::getElementType(CI->getType(), ArgNo),
                      CI->getParent());
                  recordNormalCandidate(SimpleValue(CI, ArgNo), OpUse, 0,
                                        Priority);
                } else {
                  recordNormalCandidate(
                      CI, OpUse, 0,
                      getPriority(CI->getType(), CI->getParent()));
                }
              }
            } else if (CI->isIndirectCall())
              continue;
            // This is a non-intrinsic call. If it returns a value, mark
            // (elements of) the return value for coalescing with the
            // unified return value.
            else if (!CI->getType()->isVoidTy()) {
              for (unsigned i = 0,
                  e = IndexFlattener::getNumElements(CI->getType());
                  i != e; ++i)
                recordNormalCandidate(SimpleValue(CI, i), 0, i,
                    getPriority(IndexFlattener::getElementType(
                        CI->getType(), i), CI->getParent()));
            } else {
              // handle callable kernel
              Function *Callee = CI->getCalledFunction();
              if (Callee->hasFnAttribute("CMCallable")) {
                if (F->hasFnAttribute("CMCallable")) {
                  DiagnosticInfoFastComposition Err(CI,
                    "Callable function must not call another callable"
                    " function", DS_Error);
                  F->getContext().diagnose(Err);
                }
                Callables.push_back(CI);
              }
            }
          } else {
            int OperandNum = getTwoAddressOperandNum(CI);
            if (OperandNum >= 0) {
              // This is an intrinsic with a two address operand (including
              // the case of operand 0 in wrregion). That operand has to be in
              // the same register as the result.
              if (Baling->isBaled(CI)) {
                // The intrinsic is baled into a wrregion. The two address
                // operand must also have a rdregion baled in whose input is
                // the "old value" input of the wrregion, and the coalescing
                // actually needs to be done at the wrregion.  That is handled
                // when this pass reaches the wrregion, so we do not want to do
                // anything here.
                //
                // it may also be baled into a g_store.
                // assert(Baling->getBaleInfo(CI).isOperandBaled(OperandNum) &&
                // "expecting rdregion to be baled in to the two addr operand");
                continue;
              }
              // Normal unbaled twoaddr operand.
              recordNormalCandidate(CI, &CI->getOperandUse(OperandNum),
                  getPriority(CI->getType(), CI->getParent()));
            }
          }
        } else if (isa<BitCastInst>(Inst) || isa<AddrSpaceCastInst>(Inst)) {
          assert(!isa<StructType>(Inst->getType()) && "not expecting bitcast to struct");
          assert(!isa<StructType>(Inst->getOperand(0)->getType()) && "not expecting bitcast from struct");
          // The source and destination of a bitcast can copy coalesce,
          // but only if it is not the case that the source is a phi and
          // the destination has a use in a phi node in the same block and
          // after the source's phi. If the above is the case, then we try
          // and normal coalesce instead, which fails, leading to a copy
          // being generated.
          if (GenXLiveness::wrapsAround(Inst->getOperand(0), Inst)) {
            recordNormalCandidate(Inst, &Inst->getOperandUse(0),
                getPriority(Inst->getType(), Inst->getParent()));
          } else if (Liveness->getLiveRangeOrNull(Inst)) {
            recordCopyCandidate(Inst, &Inst->getOperandUse(0),
                getPriority(Inst->getType(), Inst->getParent()));
          }
        } else if (auto EVI = dyn_cast<ExtractValueInst>(Inst)) {
          // extractvalue: copy coalesce the element being extracted, as long as
          // both source and destination have live ranges. The two cases where
          // they don't are:
          //  1. the source live range got removed in the code below that
          //     handles undef elements in an insertvalue chain;
          //  2. this is the extract of the !any(EM) result of a goto/join,
          //     which does not have a live range because it is baled in to the
          //     branch.
          if (Liveness->getLiveRangeOrNull(Inst)) {
          unsigned Index = IndexFlattener::flatten(
              cast<StructType>(EVI->getAggregateOperand()->getType()),
              EVI->getIndices());
            if (Liveness->getLiveRangeOrNull(
                  SimpleValue(Inst->getOperand(0), Index))) {
              recordCopyCandidate(SimpleValue(EVI), &Inst->getOperandUse(0), Index,
                  getPriority(EVI->getType(), EVI->getParent()));
            }
          }
        } else if (auto IVI = dyn_cast<InsertValueInst>(Inst)) {
          // insertvalue:
          // First, if the struct value input is undef, scan the possible chain
          // of insertvalues and remove the live range for any SimpleValue that
          // is undef. We need to do this to stop a register being allocated
          // later for a coalesced SimpleValue from a chain of insertvalues
          // for a return where that element is never set.
          auto ST = cast<StructType>(IVI->getType());
          unsigned NumElements = IndexFlattener::getNumElements(ST);
          if (isa<UndefValue>(IVI->getOperand(0))) {
            SmallVector<bool, 8> IsDefined;
            IsDefined.resize(NumElements, false);
            // For each insertvalue in the chain:
            for (auto ThisIVI = IVI; ThisIVI;) {
              // For the element set by this one, set it as defined (unless the
              // input is undef).
              IsDefined[IndexFlattener::flatten(ST, ThisIVI->getIndices())]
                = !isa<UndefValue>(IVI->getOperand(1));
              // For any element that is still undef, remove its live range.
              for (unsigned i = 0; i != NumElements; ++i)
                if (!IsDefined[i])
                  Liveness->removeValue(SimpleValue(ThisIVI, i));
              if (!ThisIVI->hasOneUse())
                break;
              ThisIVI = dyn_cast<InsertValueInst>(ThisIVI->use_begin()->getUser());
            }
          }
          // Copy coalesce the element being inserted and the other elements,
          // as long as the appropriate live ranges did not get removed above.
          unsigned Index = IndexFlattener::flatten(ST, IVI->getIndices());
          for (unsigned i = 0; i != NumElements; ++i) {
            if (!Liveness->getLiveRangeOrNull(SimpleValue(IVI, i)))
              continue;
            if (i == Index) {
              if (Liveness->getLiveRangeOrNull(Inst->getOperand(1)))
                recordCopyCandidate(SimpleValue(IVI, i), &Inst->getOperandUse(1), 0,
                    getPriority(IVI->getOperand(1)->getType(), IVI->getParent()));
            } else {
              if (Liveness->getLiveRangeOrNull(SimpleValue(Inst->getOperand(0), i)))
                recordCopyCandidate(SimpleValue(IVI, i), &Inst->getOperandUse(0), i,
                    getPriority(IVI->getOperand(1)->getType(), IVI->getParent()));
            }
          }
        }
      }
    }
  }
}

/***********************************************************************
 * recordCallCandidates : record the call arg pre-copy and ret value
 *                        pre-copy candidates
 *
 * This is done here, after copy coalescing has been done, so we can
 * more accurately estimate the cost of not coalescing a candidate by
 * summing the cost from each call site / return instruction that uses
 * the same (copy coalesced) value.
 */
void GenXCoalescing::recordCallCandidates(FunctionGroup *FG)
{
  // For each subroutine...
  for (auto fgi = FG->begin() + 1, fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    // Gather the call sites.
    SmallVector<Instruction *, 8> CallSites;
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui)
      if (auto CI = dyn_cast<CallInst>(ui->getUser()))
        CallSites.push_back(CI);
    // For each arg...
    unsigned ArgIdx = 0;
    for (auto ai = F->arg_begin(), ae = F->arg_end();
        ai != ae; ++ai, ++ArgIdx) {
      Argument *Arg = &*ai;
      if (Arg->use_empty())
        continue; // Ignore unused arg.
      // Record a coalesce candidate for each unique input LR for each
      // struct element in the arg.
      recordCallArgCandidates(Arg, ArgIdx, CallSites);
    }
    // Now scan for return value pre-copies.
    if (F->getReturnType()->isVoidTy())
      continue;
    // Gather the return insts by looking at the terminator of each BB.
    SmallVector<Instruction *, 8> RetInsts;
    for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      auto RetInst = dyn_cast<ReturnInst>(fi->getTerminator());
      if (RetInst)
        RetInsts.push_back(RetInst);
    }
    // Record a coalesce candidate for each unique input LR for each
    // struct element in the return value.
    recordCallArgCandidates(Liveness->getUnifiedRet(F), 0, RetInsts);
  }
}

/***********************************************************************
 * recordCallArgCandidates : common code for adding a candidate for each
 *    struct element of a call arg or a return value pre-copy
 *
 * Enter:   Dest = destination Value; the Argument for a call arg, or the
 *                 Function's unified return value for a ret pre-copy
 *          ArgNum = argument number for call arg, 0 for ret pre-copy
 *          Insts = array of call sites or return instructions
 *
 * For each struct element, this adds a coalesce candidate for each unique LR
 * used as a call arg or return value.
 */
namespace { struct CallArg {
  Use *U;
  LiveRange *LR;
  CallArg(Use *U, LiveRange *LR) : U(U), LR(LR) {}
}; }
void GenXCoalescing::recordCallArgCandidates(Value *Dest, unsigned ArgNum,
    ArrayRef<Instruction *> Insts)
{
  for (unsigned StructIdx = 0,
      StructEnd = IndexFlattener::getNumElements(Dest->getType());
      StructIdx != StructEnd; ++StructIdx) {
    // For each unique LR used as this arg at any call site, sum the
    // cost and add a candidate.
    SmallVector<CallArg, 8> CallArgs;
    for (unsigned i = 0, ie = Insts.size(); i != ie; ++i) {
      Use *U = &Insts[i]->getOperandUse(ArgNum);
      CallArgs.push_back(CallArg(U,
          Liveness->getLiveRangeOrNull(SimpleValue(*U, StructIdx))));
    }
    for (unsigned i = 0, ie = CallArgs.size(); i != ie; ++i) {
      LiveRange *LR = CallArgs[i].LR;
      if (!LR)
        continue; // Already done this one (or it was an undef).
      unsigned Priority = 0;
      for (unsigned j = i, je = CallArgs.size(); j != je; ++j) {
        if (LR != CallArgs[j].LR)
          continue;
        Priority += getPriority(nullptr, Insts[j]->getParent());
        CallArgs[j].LR = 0; // Blank out so we can see we have done this one.
      }
      Use *U = CallArgs[i].U;
      Priority *= getPriority(IndexFlattener::getElementType(
            (*U)->getType(), StructIdx), 0);
      recordNormalCandidate(SimpleValue(Dest, StructIdx),
          U, StructIdx, Priority);
    }
  }
}

/***********************************************************************
 * getPriority : get priority of coalescing candidate
 *
 * Enter:   Ty = type that would need to be copied if coalescing failed,
 *               so we can estimate the copy cost. Can be nullptr, in which
 *               case the copy cost is assumed to be 1
 *          BB = basic block where copy would be inserted, so we can use
 *               loop depth to adjust the cost. Can be nullptr, in which
 *               the loop depth is assumed to be 0
 *
 * Return:  priority (estimate of cost of inserting a copy)
 *
 * getPriority(Ty, BB) is equivalent to getPriority(Ty, 0) * getPriority(0, BB).
 */
unsigned GenXCoalescing::getPriority(Type *Ty, BasicBlock *BB)
{
  // Set priority to the number of GRFs.
  // FIXME this should also take into account a non power of two
  // vector size, which would result in multiple copy instructions.
  // See GenXCoalescing::insertCopy.
  // FIXME scale by loop depth.
  unsigned Priority = 1;
  if (Ty) {
    if (VectorType *VT = dyn_cast<VectorType>(Ty)) {
      Priority = VT->getNumElements() * VT->getElementType()->getPrimitiveSizeInBits();
      Priority = (Priority + 255) / 256;
    }
  }
  return Priority;
}

/***********************************************************************
 * recordCandidate : record a candidate for coalescing
 *
 * Enter:   Dest = destination of copy
 *          UseInDest = pointer to the use of the source in Dest
 *          SourceIndex = flattened index of element in source struct
 *          Priority = priority of coalescing this candidate
 *          Candidates = vector of candidates to push to
 *
 * For call arg coalescing, Dest is the subroutine's Argument, and
 * UseInDest/SourceIndex are the use in one of the possibly many call sites
 * using the same source value.
 *
 * For ret value pre-copy coalescing (before the return inst), Dest is the the
 * unified return value, and UseInDest/SourceIndex are the use in one of the
 * possibly many return instructions using the same source value.
 *
 * For ret value post-copy coalescing (after the call inst), Dest is the
 * CallInst, and UseInDest and SourceIndex are 0.
 */
void GenXCoalescing::recordCandidate(SimpleValue Dest, Use *UseInDest,
    unsigned SourceIndex, unsigned Priority, std::vector<Candidate> *Candidates)
{
  if (UseInDest && isa<UndefValue>(*UseInDest))
    return;
  assert(!UseInDest || !isa<Constant>(*UseInDest));
  Candidates->push_back(Candidate(Dest, UseInDest, SourceIndex, Priority,
        Candidates->size()));
}

/***********************************************************************
 * processCandidate : process a coalescing candidate
 *
 * This attempts to coalesce the candidate. On failure, it inserts a copy
 * if necessary:
 *
 *  - a copy candidate never fails to coalesce;
 *  - a two address candidate needs a copy and it is inserted here;
 *  - a phi candidate needs a copy, but it is not inserted here. Instead it
 *    is inserted later so we can ensure that multiple copies inserted at
 *    the end of an incoming block are in phi node order, which was the
 *    assumption made by the live range calculation.
 *
 * See the comment at the top of recordCandidate for the special values of
 * fields in Candidate for a call arg coalesce and a ret value coalesce.
 */
void GenXCoalescing::processCandidate(Candidate *Cand, bool IsCopy)
{
  SimpleValue Dest = Cand->Dest;
  SimpleValue Source;
  if (!Cand->UseInDest) {
    // This is a return value post-copy coalesce candidate. The actual source
    // is the unified return value.
    Source = SimpleValue(Liveness->getUnifiedRet(cast<CallInst>(
            Dest.getValue())->getCalledFunction()), Cand->SourceIndex);
  } else
    Source = SimpleValue(*Cand->UseInDest, Cand->SourceIndex);
  LLVM_DEBUG(dbgs() << "Trying coalesce from ";
      Source.printName(dbgs());
      dbgs() << " to ";
      Dest.printName(dbgs());
      dbgs() << " priority " << Cand->Priority;
      if (isa<Argument>(Dest.getValue()))
        dbgs() << " (call arg)";
      else if (Liveness->isUnifiedRet(Dest.getValue()))
        dbgs() << " (ret pre-copy)";
      else if (!Cand->UseInDest)
        dbgs() << " (ret post-copy)";
      dbgs() << "\n");
  LiveRange *DestLR = Liveness->getLiveRange(Dest);
  LiveRange *SourceLR = 0;
  // Source should not be a constant (but could be undef) because
  // GenXLowering ensured that all our two address operands and phi incomings
  // are not constant.
  assert(!Cand->UseInDest || !isa<Constant>(Source.getValue()) || isa<UndefValue>(Source.getValue()));
  SourceLR = Liveness->getLiveRange(Source);
  assert(DestLR);
  if (SourceLR == DestLR)
    return; // already coalesced
  if (SourceLR && SourceLR->Category == DestLR->Category) {
    if (IsCopy) {
      // For a copy candidate, we can coalesce if the source and destination do
      // not copy-interfere, i.e. we do not have a situation where DestLR
      // wraps round a loop into a phi use in the same basic block as the phi
      // def of SourceLR but after it.
      if (!Liveness->copyInterfere(SourceLR, DestLR)) {
        Liveness->coalesce(DestLR, SourceLR, /*DisallowCASC=*/ false);
        return;
      }
    } else {
      // For a normal candidate, we can coalesce if the source and destination
      // do not interfere, i.e. there is no point in the program where both
      // LRs are live.
      if (!Liveness->twoAddrInterfere(DestLR, SourceLR)) {
        // In the coalesce, disallow future call arg special coalescing if this
        // is not a call arg coalesce.
        Liveness->coalesce(DestLR, SourceLR,
            /*DisallowCASC=*/ !isa<Argument>(Dest.getValue()));
        return;
      }
    }
  }
#if 0
  // Disable call arg special coalescing for now, as it seems to break the FRC_MC example.

  if (isa<Argument>(Dest.getValue())
      && SourceLR->Category == DestLR->Category) {
    // This is an attempt at call arg coalescing. The two LRs interfere, but
    // we can still try for "call arg special coalescing" (CASC). See the
    // comment at the top of the file.
    if (!DestLR->DisallowCASC) {
      // CASC not disallowed. (It would have been disallowed if DestLR had
      // already participated in normal coalescing other than CASC.)
      // For any call site where SourceLR is not the corresponding call arg,
      // check that A is not live.
      auto ThisCallSite = cast<CallInst>(Cand->UseInDest->getUser());
      auto Callee = ThisCallSite->getCalledFunction();
      bool FailedCASC = false;
      for (auto ui = Callee->use_begin(), ue = Callee->use_end();
          ui != ue; ++ui) {
        auto CallSite = cast<CallInst>(ui->getUser());
        if (CallSite == ThisCallSite)
          continue;
        auto OtherArg = SimpleValue(CallSite->getArgOperand(cast<Argument>(
                Dest.getValue())->getArgNo()), Dest.getIndex());
        auto OtherLR = Liveness->getLiveRange(OtherArg);
        // Check whether OtherArg is the same as SourceLR. This check covers
        // several cases:
        // 1. OtherArg == SourceLR: the other arg is already coalesced with
        //    our arg, so it would be OK to do CASC.
        // 2. OtherArg is DestLR, meaning that the other call arg has already
        //    been coalesced with the func arg. We cannot do CASC if SourceLR
        //    and OtherArg interfere, which they do because we already know
        //    that DestLR interferes with SourceLR.
        // 3. OtherArg is something else, meaning that some other value will
        //    be copied to the func arg here. We cannot do CASC if SourceLR
        //    and OtherArg interfere.
        if (OtherLR == SourceLR)
          continue;
        if (Liveness->interfere(OtherLR, SourceLR)) {
          FailedCASC = true;
          break;
        }
      }
      if (!FailedCASC) {
        // Can coalesce. Do not disallow future CASC.
        Liveness->coalesce(DestLR, SourceLR, /*DisallowCASC=*/ false);
        return;
      }
    }
  }
#endif

  // Coalescing failed.
  LLVM_DEBUG(
    if (SourceLR) {
      dbgs() << "Live ranges \"";
      DestLR->print(dbgs());
      dbgs() << "\" and \"";
      SourceLR->print(dbgs());
      dbgs() << "\"" << (IsCopy ? " copy" : "") << " interfere, not coalescing\n";
    } else {
      dbgs() << "Need copy of constant \"";
      Source.print(dbgs());
      dbgs() << "\" to \"";
      Dest.printName(dbgs());
      dbgs() << "\"\n";
    }
  );
  if (isa<PHINode>(Dest.getValue()))
    return; // Candidate is phi; copy insertion done later.
  if (isa<Argument>(Dest.getValue()))
    return; // Call arg pre-copy, defer copy insertion
  if (Liveness->isUnifiedRet(Dest.getValue()))
    return; // Return value pre-copy, defer copy insertion
  if (!Cand->UseInDest)
    return; // Return value post-copy, defer copy insertion
  if (isa<BitCastInst>(Dest.getValue()) || isa<AddrSpaceCastInst>(Dest.getValue())) {
    // A bitcast is normally copy coalesced, which means it cannot fail to
    // coalesce. However, if the source is a phi node and the destination
    // wraps round the loop and is used in another phi node in the same
    // block that is later than the first phi node, then we instead
    // try to normal coalesce, which fails because they interfere.
    // This happens with a bitcast inserted in GenXLiveRanges to resolve
    // an overlapping circular phi, but can happen in other cases too.
    if ((int)genx::exactLog2(
          Dest.getValue()->getType()->getPrimitiveSizeInBits()) <= 8) {
      // This is a bitcast with a legal size for a single copy. We do not
      // insert a copy, because GenXVisaFuncWriter will generate one.
      // (GenXLegalization does not legalize a bitcast, so it can be
      // illegal size here. We do that on the basis that a bitcast is
      // normally copy coalesced.)
      return;
    }
    // Otherwise, it is a bitcast of size more than 1 GRF or non-power-of-two,
    // so we insert a copy.
  }
  // Insert the copy now for a two address op. Give it the number of the
  // pre-copy slot, which is one less than the number of the two address
  // instruction.
  Instruction *DestInst = cast<Instruction>(Dest.getValue());
  showCoalesceFail(Dest, DestInst->getDebugLoc(), "two address",
      DestLR, SourceLR);
  Instruction *NewCopy = insertCopy(Source, DestLR, DestInst, "twoaddr",
      Numbering->getNumber(DestInst) - 1);
  NewCopy = insertIntoStruct(Dest.getValue()->getType(),
      Dest.getIndex(), *Cand->UseInDest, NewCopy, DestInst);
  // Replace the use of the old source.
  *Cand->UseInDest = NewCopy;
  // No need to extend the live range, as the result of the two address op was
  // already marked as defined at the pre-copy slot.
}

/***********************************************************************
 * processPhiNodes : add copies for uncoalesced phi node incomings
 */
void GenXCoalescing::processPhiNodes(FunctionGroup *FG)
{
  std::vector<PhiCopy> PhiCopies;

  for (auto fgi = FG->begin(), fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    for (Function::iterator fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      BasicBlock *BB = &*fi;
      for (BasicBlock::iterator bi = BB->begin(), be = BB->end(); bi != be; ++bi) {
        // Scan the phi nodes at the start of this BB, if any.
        PHINode *Phi = dyn_cast<PHINode>(&*bi);
        if (!Phi)
          break;

        // Collect copies to process
        analysePhiCopies(Phi, PhiCopies);
      }
    }
  }

  // Perform copy of uncoalesced phi node incomings.
  // New phis can be created during this, store them.
  std::vector<PHINode *> NewPhis;
  for (auto Elem : PhiCopies) {
    processPhiCopy(Elem.Phi, Elem.IncomingIdx, NewPhis);
  }
  // Phi copies are resolved. Clean the list.
  PhiCopies.clear();

  // Process newly created phis. This loop is executed
  // when coalescing failed to resolve issues with phis
  // in branching join label blocks. Such situation is
  // very rare because coalescing tries to solve it
  // with the highest priority.
  while (!NewPhis.empty()) {
    // Collect phi copy candidates
    for (auto *Phi : NewPhis) {
      analysePhiCopies(Phi, PhiCopies);
    }
    // Phi copies are collected, clean current Phis worklist
    NewPhis.clear();

    // Perform copy of uncoalesced phi node incomings.
    for (auto Elem : PhiCopies) {
      processPhiCopy(Elem.Phi, Elem.IncomingIdx, NewPhis);
    }
    // Phi copies are resolved. Clean the list.
    PhiCopies.clear();
  }
}

/***********************************************************************
 * analysePhiCopies : for one phi node, collect copies for uncoalesced incomings
 */
void GenXCoalescing::analysePhiCopies(PHINode *Phi,
                                      std::vector<PhiCopy> &ToProcess) {
  // Scan each incoming to see if it was successfully coalesced.
  LiveRange *DestLR = Liveness->getLiveRange(Phi);
  if (DestLR->getCategory() >= RegCategory::NUMREALCATEGORIES)
    return; // Ignore phi node of EM/RM value.
  for (unsigned i = 0, e = Phi->getNumIncomingValues(); i != e; ++i) {
    Value *Incoming = Phi->getIncomingValue(i);
    // Incoming should not be a constant (but could be undef) because
    // GenXPostLegalization and GenXCategory called loadNonSimpleConstants
    // to load the non-simple constant incomings, then GenXCategory also
    // called GenXConstants::loadConstant for each remaining (simple)
    // constant.
    if (isa<UndefValue>(Incoming))
      continue; // undef, no copy needed
    assert(!isa<Constant>(Incoming));
    if (Liveness->getLiveRange(Incoming) == DestLR)
      continue; // coalesced, no copy needed
    // A phi copy is needed
    auto IncomingBlock = Phi->getIncomingBlock(i);
    LLVM_DEBUG(dbgs() << "Need phi copy " << Incoming->getName() << " -> "
                      << Phi->getName() << " in " << IncomingBlock->getName()
                      << "\n");
    ToProcess.push_back(PhiCopy(Phi, i));
  }
}

/***********************************************************************
 * processPhiCopy : for one phi node incoming, add copy
 */
void GenXCoalescing::processPhiCopy(PHINode *Phi, unsigned Inc,
                                    std::vector<PHINode *> &Phis) {
  LiveRange *DestLR = Liveness->getLiveRange(Phi);
  Value *Incoming = Phi->getIncomingValue(Inc);
  auto *IncomingBlock = Phi->getIncomingBlock(Inc);
  // Should be checked in analysePhiCopies
  assert(DestLR->getCategory() < RegCategory::NUMREALCATEGORIES &&
         "Should be checked earlier!");
  assert(!isa<UndefValue>(Incoming) && "Should be checked earlier!");
  assert(!isa<Constant>(Incoming) && "Should be checked earlier!");
  // Check it again: something could change
  if (Liveness->getLiveRange(Incoming) == DestLR) {
    LLVM_DEBUG(dbgs() << "Already coalesced " << Incoming->getName() << " -> "
                      << Phi->getName() << " in " << IncomingBlock->getName()
                      << "\n");
    return;
  }

  LLVM_DEBUG(dbgs() << "Copying " << Incoming->getName() << " -> "
                    << Phi->getName() << " in " << IncomingBlock->getName()
                    << "\n");

  // Handle branching join label block separately
  if (GotoJoin::isBranchingJoinLabelBlock(IncomingBlock)) {
    processPhiBranchingJoinLabelCopy(Phi, Inc, Phis);
    return;
  }

  DominatorTree *DomTree = getDomTree(IncomingBlock->getParent());
  Instruction *InsertPoint = IncomingBlock->getTerminator();
  InsertPoint = GotoJoin::getLegalInsertionPoint(InsertPoint, DomTree);

  // Give the copy the number allocated to the phi incoming
  unsigned Num = Numbering->getPhiNumber(Phi, IncomingBlock);

  if (auto *I = dyn_cast<Instruction>(Incoming)) {
    // This should not happen for good BBs (not join blocks)
    // if DFG is correct.
    assert(DomTree->dominates(I->getParent(), InsertPoint->getParent()) &&
           "Dominance corrupted!");
  }

  showCoalesceFail(SimpleValue(Incoming), InsertPoint->getDebugLoc(), "phi",
                   DestLR, Liveness->getLiveRange(Incoming));
  Instruction *NewCopy =
      insertCopy(SimpleValue(Incoming), DestLR, InsertPoint, "phicopy", Num);
  Phi->setIncomingValue(Inc, NewCopy);
  // No need to extend the live range like we do in the two address op case
  // in processCandidate(). The live range of a phi node already starts at
  // each point where a copy might need to be inserted.
}

/***********************************************************************
 * processPhiBranchingJoinLabelCopy : for one phi node incoming, add copy
 * for branching join label incoming BB case
 */
void GenXCoalescing::processPhiBranchingJoinLabelCopy(
    PHINode *Phi, unsigned Inc, std::vector<PHINode *> &Phis) {
  LiveRange *DestLR = Liveness->getLiveRange(Phi);
  Value *Incoming = Phi->getIncomingValue(Inc);
  auto *IncomingBlock = Phi->getIncomingBlock(Inc);
  // Should be checked in analysePhiCopies
  assert(DestLR->getCategory() < RegCategory::NUMREALCATEGORIES &&
         "Should be checked earlier!");
  assert(!isa<UndefValue>(Incoming) && "Should be checked earlier!");
  assert(!isa<Constant>(Incoming) && "Should be checked earlier!");
  // Should be checked in processPhiCopy
  assert(Liveness->getLiveRange(Incoming) != DestLR &&
         "Should be checked earlier!");
  assert(GotoJoin::isBranchingJoinLabelBlock(IncomingBlock) &&
         "Should be checked earlier!");

  LLVM_DEBUG(dbgs() << "Handling branching join label block case\n");

  DominatorTree *DomTree = getDomTree(IncomingBlock->getParent());
  Instruction *InsertPoint = IncomingBlock->getTerminator();
  InsertPoint = GotoJoin::getLegalInsertionPoint(InsertPoint, DomTree);

  // Give the copy the number of term to make proper liverange
  unsigned Num = Numbering->getNumber(InsertPoint);

  if (auto *PhiPred = dyn_cast<PHINode>(Incoming)) {
    // In case when pred is Phi, it is possible to meet Phi in
    // branching join blocks since such Phi does not brake
    // SIMD CF Conformance. If such situation happens, we cannot
    // perform copy of a phi value copy, we need to perform copy
    // on all its incoming values. To do that, copy Phi and add
    // it to Phis worklist.
    //
    // This situation is detected via corrupted dominance.
    if (!DomTree->dominates(PhiPred->getParent(), InsertPoint->getParent())) {
      auto *PhiCopy = copyNonCoalescedPhi(PhiPred, Phi);
      assert(PhiCopy && "Invalid phi copy!");
      Phis.push_back(PhiCopy);
      return;
    }
  }

  if (auto *I = dyn_cast<Instruction>(Incoming)) {
    // This should not happen for good BBs (not join blocks)
    // if DFG is correct.
    //
    // For join block, def must be somewhere before it
    // because of SIMD CF Conformance. Case for Phi is
    // described and handled above.
    assert(DomTree->dominates(I->getParent(), InsertPoint->getParent()) &&
           "Dominance corrupted!");
  }

  showCoalesceFail(SimpleValue(Incoming), InsertPoint->getDebugLoc(), "phi",
                   DestLR, Liveness->getLiveRange(Incoming));
  Instruction *NewCopy =
      insertCopy(SimpleValue(Incoming), DestLR, InsertPoint, "phicopy", Num);
  Phi->setIncomingValue(Inc, NewCopy);

  // Extend liverange: we skipped some basic blocks
  Liveness->rebuildLiveRange(DestLR);
}

/***********************************************************************
 * copyNonCoalescedPhi : copy PhiPred and coalesce copy's LR with
 * PhiSucc's LR
 */
PHINode *GenXCoalescing::copyNonCoalescedPhi(PHINode *PhiPred,
                                             PHINode *PhiSucc) {
  // Perform copy
  auto *PhiCopy = cast<PHINode>(PhiPred->clone());
  PhiCopy->insertBefore(PhiPred->getNextNode());
  PhiCopy->setName(PhiPred->getName() + ".copy");
  Numbering->setNumber(PhiCopy, Numbering->getNumber(PhiPred));

  // Handle LRs
  Liveness->buildLiveRange(PhiCopy);
  LiveRange *DestLR = Liveness->getLiveRange(PhiSucc);
  LiveRange *NewLR = Liveness->getLiveRange(PhiCopy);
  Liveness->coalesce(DestLR, NewLR, false);

  // Update incoming values
  for (unsigned i = 0, e = PhiSucc->getNumIncomingValues(); i != e; ++i) {
    Value *IncValue = PhiSucc->getIncomingValue(i);
    if (IncValue == PhiPred)
      PhiSucc->setIncomingValue(i, PhiCopy);
  }

  return PhiCopy;
}

/***********************************************************************
 * processCalls : insert copies where necessary for call args and ret values
 *
 * This scans all the calls, inserting copies where necessary for call arg
 * pre-copies and return value pre- and post-copies.
 *
 * We need to do them in one go here because
 * 1. a call arg or return value pre-copy coalescing candidate covers
 *    possibly multiple sites where the same LR input is used, without giving
 *    any way of getting back to them all;
 * 2. we want the inserted copies to be in the order that live range
 *    computation assumed they would appear.
 */
void GenXCoalescing::processCalls(FunctionGroup *FG)
{
  // For each subroutine...
  for (auto fgi = FG->begin() + 1, fge = FG->end(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    // For each call site...
    for (auto ui = F->use_begin(), ue = F->use_end(); ui != ue; ++ui) {
      if (auto *CI = dyn_cast<CallInst>(ui->getUser())) {
        // For each func arg...
        unsigned ArgIdx = 0;
        for (auto ai = F->arg_begin(), ae = F->arg_end(); ai != ae;
             ++ai, ++ArgIdx) {
          Argument *Arg = &*ai;
          if (Arg->use_empty()) {
            // Arg is unused inside the subroutine. Do not try and process
            // further, as its live range probably does not have a category.
            continue;
          }
          Value *CallArg = CI->getOperand(ArgIdx);
          if (isa<UndefValue>(CallArg)) {
            // Call arg undefined. No coalescing needed.
            continue;
          }
          // For each SimpleValue in the func arg...
          for (unsigned StructIdx = 0,
                        se = IndexFlattener::getNumElements(Arg->getType());
               StructIdx != se; ++StructIdx) {
            assert(!StructIdx &&
                   "coalesce failure on struct call arg not tested");
            auto FuncArgSV = SimpleValue(Arg, StructIdx);
            auto CallArgSV = SimpleValue(CallArg, StructIdx);
            // See if they are coalesced.
            auto DestLR = Liveness->getLiveRange(FuncArgSV);
            auto SourceLR = Liveness->getLiveRange(CallArgSV);
            if (!DestLR || DestLR == SourceLR || F == CI->getFunction())
              continue;
            if (DestLR->getCategory() >= RegCategory::NUMREALCATEGORIES)
              continue; // Called function arg is EM.
            // Need to insert a copy. Give it the number of the arg's pre-copy
            // slot.
            showCoalesceFail(CallArgSV, CI->getDebugLoc(), "call arg", DestLR,
                             SourceLR);
            unsigned Num =
                Numbering->getArgPreCopyNumber(CI, ArgIdx, StructIdx);
            Instruction *NewCopy =
                insertCopy(CallArgSV, DestLR, CI, "callarg.precopy", Num);
            NewCopy = insertIntoStruct(Arg->getType(), StructIdx,
                                       CI->getOperand(ArgIdx), NewCopy, CI);
            // Replace operand in call.
            CI->setOperand(ArgIdx, NewCopy);
            // No need to extend the live range like we do in the two address op
            // case in processCandidate(). The live range of a func arg already
            // starts at each point where a copy might need to be inserted.
          }
        }
        // Now check the return value post-copy.
        //
        // The code to handle a coalesce failure in a return value post-copy
        // is different to all other cases of coalesce failure, which are
        // pre-copy. We need to ensure that the post-copied value is in the
        // original live range for the original value (the return value),
        // and all the original value's users are changed to use the post-copied
        // value instead. The original value (the return value) gets moved out
        // of its live range and put into that of the unified return value.
        //
        // If the return value is a struct, all the above happens for each
        // struct element, with the extra complication of more new values to
        // handle because of the extractvalue and insertvalue instructions we
        // need to insert.
        //
        // First remember all uses of the return value, because we want to
        // replace them after adding new ones below. Remember if they are
        // all extractvalue with a non-struct result (which should usually be
        // the case because GenXLowering removes most structs).
        SmallVector<Use *, 8> CIUses;
        bool AllUsesAreExtract = isa<StructType>(CI->getType());
        for (auto ui = CI->use_begin(), ue = CI->use_end(); ui != ue; ++ui) {
          auto EV = dyn_cast<ExtractValueInst>(ui->getUser());
          if (!EV || isa<StructType>(EV->getType()))
            AllUsesAreExtract = false;
          CIUses.push_back(&*ui);
        }
        Instruction *InsertBefore = CI->getNextNode();
        Value *StructValue = CI;
        SmallVector<LiveRange *, 8> PreviousElements;
        // For each SimpleValue in the return value...
        for (unsigned StructIdx = 0,
                      se = IndexFlattener::getNumElements(CI->getType());
             StructIdx != se; ++StructIdx) {
          auto UnifiedSV = SimpleValue(Liveness->getUnifiedRet(F), StructIdx);
          auto SV = SimpleValue(CI, StructIdx);
          // See if (the element in) the returned value is dead, or successfully
          // coalesced with (the element in) the unified return value.
          auto DestLR = Liveness->getLiveRangeOrNull(SV);
          PreviousElements.push_back(DestLR);
          if (!DestLR)
            continue; // dead
          auto SourceLR = Liveness->getLiveRange(UnifiedSV);
          if (DestLR == SourceLR)
            continue; // coalesced
          assert(SourceLR);
          if (SourceLR->getCategory() >= RegCategory::NUMREALCATEGORIES)
            continue; // Unified return value is EM, ignore.
          // Remove (the element of) CI, the actual return value, from its
          // own live range, and add it instead to the unified return value.
          // insertCopy() will add the new value to DestLR (what
          // was the LR for the element of CI).
          Liveness->removeValueNoDelete(SV);
          Liveness->setLiveRange(SV, SourceLR);
          // Need to insert a copy. Give it the number of the post-copy slot.
          showCoalesceFail(SimpleValue(CI, StructIdx), CI->getDebugLoc(),
                           "ret postcopy", DestLR, SourceLR);
          unsigned Num = Numbering->getRetPostCopyNumber(CI, StructIdx);
          Instruction *NewCopy =
              insertCopy(SimpleValue(CI, StructIdx), DestLR, InsertBefore,
                         "retval.postcopy", Num);
          assert(NewCopy);
          if (AllUsesAreExtract) {
            // For a struct ret value where all the uses are non-struct
            // extractvalue, replace uses of the extractvalues with NewCopy.
            // Doing this, rather than calling insertIntoStruct() and letting
            // the existing extractvalue extract it again, does not improve the
            // code generated by the compiler (insertvalue/extractvalue do not
            // generate any code), but it does make the IR simpler and easier
            // to understand in a dump.
            for (unsigned i = 0, e = CIUses.size(); i != e; ++i) {
              if (!CIUses[i])
                continue;
              auto EV = cast<ExtractValueInst>(CIUses[i]->getUser());
              if (StructIdx ==
                  IndexFlattener::flatten(cast<StructType>(CI->getType()),
                                          EV->getIndices())) {
                NewCopy->takeName(EV);
                EV->replaceAllUsesWith(NewCopy);
                if (EV == InsertBefore)
                  InsertBefore = InsertBefore->getNextNode();
                Liveness->removeValue(SimpleValue(EV));
                EV->eraseFromParent();
                CIUses[i] = 0;
              }
            }
          } else {
            // If this is a struct return value, we also need to insertvalue,
            // creating a new struct value.
            StructValue = insertIntoStruct(CI->getType(), StructIdx,
                                           StructValue, NewCopy, InsertBefore);
            // Also, for this and previously seen elements that are not dead,
            // add that element of StructValue (the new insertvalue) to the live
            // range.
            if (StructValue != NewCopy) {
              for (unsigned k = 0, ke = PreviousElements.size(); k != ke; ++k) {
                if (PreviousElements[k])
                  Liveness->setLiveRange(SimpleValue(StructValue, k),
                                         PreviousElements[k]);
              }
            }
          }
        }
        if (!AllUsesAreExtract) {
          // Replace uses of the whole return value that existed before we added
          // more uses above.
          for (unsigned i = 0, e = CIUses.size(); i != e; ++i)
            *CIUses[i] = StructValue;
        }
      }
    }
    if (F->getReturnType()->isVoidTy())
      continue; // no return value from this func
    // For each return inst in the func...
    for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
      auto RI = dyn_cast<ReturnInst>(fi->getTerminator());
      if (!RI)
        continue;
      Value *Input = RI->getOperand(0);
      if (isa<UndefValue>(Input))
        continue;
      Value *UnifiedRet = Liveness->getUnifiedRet(F);
      // For each struct element in the return value...
      for (unsigned StructIdx = 0,
          StructEnd = IndexFlattener::getNumElements(UnifiedRet->getType());
          StructIdx != StructEnd; ++StructIdx) {
        auto DestLR = Liveness->getLiveRange(SimpleValue(UnifiedRet, StructIdx));
        auto SourceLR = Liveness->getLiveRange(SimpleValue(Input, StructIdx));
        if (DestLR == SourceLR)
          continue; // coalesced
        // Need to insert a copy. Give it the number of the ret pre-copy slot.
        showCoalesceFail(SimpleValue(Input, StructIdx), RI->getDebugLoc(),
              "ret precopy", DestLR, SourceLR);
        unsigned Num = Numbering->getNumber(RI) - StructEnd + StructIdx;
        Instruction *NewCopy = insertCopy(SimpleValue(Input, StructIdx),
            DestLR, RI, "retval.precopy", Num);
        NewCopy = insertIntoStruct(UnifiedRet->getType(), StructIdx,
            RI->getOperand(0), NewCopy, RI);
        // Replace operand in call.
        RI->setOperand(0, NewCopy);
        // No need to extend the live range like we do in the two address op
        // case in processCandidate(). The live range of the unified return
        // value already starts at each point where a copy might need to be
        // inserted.
      }
    }
  }
}

/***********************************************************************
 * processKernelArgs : add a copy for each kernel arg that is not aligned enough
 */
void GenXCoalescing::processKernelArgs(FunctionGroup *FG)
{
  auto F = FG->getHead();
  if (!isKernel(F))
    return;
  Instruction *InsertBefore = F->front().getFirstNonPHIOrDbg();
  KernelMetadata KM(F);
  unsigned Idx = 0;
  for (auto ai = F->arg_begin(), ae = F->arg_end(); ai != ae; ++ai) {
    if (KM.shouldSkipArg(Idx++))
      continue;
    auto Arg = &*ai;
    auto LR = Liveness->getLiveRange(Arg);
    if (!(LR->Offset & ((1U << LR->LogAlignment) - 1)))
      continue; // aligned enough
    // Insert a copy and give the original arg its own new live range. This
    // leaves the original live range still live from the start of the
    // function, and thus interfering with the new live range for the arg,
    // but that doesn't matter.
    SmallVector<Use *, 4> Uses;
    for (auto ui = Arg->use_begin(), ue = Arg->use_end(); ui != ue; ++ui)
      Uses.push_back(&*ui);
    unsigned Num = Numbering->getKernelArgCopyNumber(Arg);
    auto Copy = insertCopy(Arg, LR, InsertBefore, "argcopy", Num);
    Liveness->removeValueNoDelete(Arg);
    for (auto ui = Uses.begin(), ue = Uses.end(); ui != ue; ++ui)
      **ui = Copy;
    auto NewLR = Liveness->getOrCreateLiveRange(Arg);
    NewLR->setCategory(LR->getCategory());
    NewLR->push_back(Segment(Numbering->getNumber(F), Num));
    NewLR->Offset = LR->Offset;
    LR->Offset = 0;
  }
}

void GenXCoalescing::coalesceOutputArgs(FunctionGroup *FG) {
  auto F = FG->getHead();
  if (!isKernel(F))
    return;

  std::string Name = GenXIntrinsic::getGenXName(GenXIntrinsic::genx_output);
  Function *OutputFn = F->getParent()->getFunction(Name);
  if (!OutputFn)
    return;

  KernelMetadata KM(F);
  for (auto U : OutputFn->users()) {
    auto CI = dyn_cast<CallInst>(U);
    if (!CI || CI->getParent()->getParent() != F)
      continue;

    unsigned Idx = 0; // kernel argument index
    unsigned i = 0;   // call argument index
    for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      if (!KM.isOutputArg(Idx++))
        continue;

      // This is the final value stored into the output argument.
      // If this is coalesced into kernel argument, nothing to do.
      // Otherwise, insert a copy.
      Value *V = CI->getArgOperand(i);
      Value *Arg = &*I;
      LiveRange *LR1 = Liveness->getLiveRangeOrNull(V);
      LiveRange *LR2 = Liveness->getLiveRange(Arg);

      auto coalesceInput = [=]() {
        // When LR1 is null, the input value should be Undef. Otherwise, it
        // should be loaded as a constant.
        if (LR1 == nullptr || LR1 == LR2)
          return false;

        if (!Liveness->interfere(LR1, LR2)) {
          Liveness->coalesce(LR1, LR2, false);
          return false;
        }

        // A copy is needed.
        return true;
      };

      if (coalesceInput()) {
        // Insert copy and add a short live range for copy-out.
        unsigned Num = Numbering->getNumber(CI);
        auto Copy = insertCopy(V, LR2, CI, "copyout", Num);
        CI->setArgOperand(i, Copy);
        LR2->push_back(Num, Num + 1);
        LR2->sortAndMerge();
      }
      ++i;
    }
  }
}

void GenXCoalescing::coalesceCallables() {
  for (auto CI : Callables) {
    auto NI = CI->getNextNode();
    // if the next instruction is a CM-output intrinsic,
    // we don't really need that cm-output because CMCallable can serve as
    // the anchor for preventing DCE
    if (NI && isa<CallInst>(NI)) {
      CallInst *OC = cast<CallInst>(NI);
      if (GenXIntrinsic::getGenXIntrinsicID(OC) == GenXIntrinsic::genx_output) {
        OC->eraseFromParent();
      }
    }

    auto Nxt = CI->getNextNode();
    auto Ret = Nxt;

    // 1. Possible next node is branch to return
    auto Br = dyn_cast<BranchInst>(Nxt);
    if (Br && Br->isUnconditional())
      Ret = &Br->getSuccessor(0)->front();

    // 2. Possible next node is GenXIntrinsic::genx_output
    if (GenXIntrinsic::getGenXIntrinsicID(Ret) == GenXIntrinsic::genx_output)
      Ret = Ret->getNextNode();

    // Check if next node is correct return insn
    if (!Ret || !isa<ReturnInst>(Ret)) {
      // getRetVal could not determine what happens to this return value.
      DiagnosticInfoFastComposition Err(CI,
        "Callable Call must be right before function return",
        (ST->warnCallable() ? DS_Warning : DS_Error));
      CI->getContext().diagnose(Err);
    }
    Function *F = CI->getParent()->getParent();
    assert(isKernel(F));
    KernelMetadata KM(F);
    unsigned Idx = 0; // kernel argument index
    unsigned i = 0;   // call argument index
    for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I) {
      if (KM.getArgInputOutputKind(Idx++) == KernelMetadata::IO_Normal)
        continue;

      // This is the final value stored into the output argument.
      // If this is coalesced into kernel argument, nothing to do.
      // Otherwise, insert a copy.
      Value *V = CI->getArgOperand(i);
      Value *Arg = &*I;
      LiveRange *LR1 = Liveness->getLiveRangeOrNull(V);
      LiveRange *LR2 = Liveness->getLiveRange(Arg);

      auto coalesceInput = [=]() {
        // When LR1 is null, the input value should be Undef. Otherwise, it
        // should be loaded as a constant.
        if (LR1 == nullptr || LR1 == LR2)
          return false;

        if (!Liveness->interfere(LR1, LR2)) {
          Liveness->coalesce(LR1, LR2, false);
          return false;
        }

        // A copy is needed.
        return true;
      };

      if (coalesceInput()) {
        // Insert copy and add a short live range for copy-out.
        unsigned Num = Numbering->getNumber(CI);
        auto Copy = insertCopy(V, LR2, CI, "copyout", Num);
        CI->setArgOperand(i, Copy);
        LR2->push_back(Num, Num + 1);
        LR2->sortAndMerge();
      }
      ++i;
    }
  }
}

void GenXCoalescing::coalesceGlobalLoads(FunctionGroup *FG) {
  for (auto &GV : FG->getModule()->globals()) {
    if (!GV.hasAttribute(genx::FunctionMD::GenXVolatile))
      continue;
    LiveRange *LR1 = Liveness->getLiveRangeOrNull(&GV);
    if (!LR1)
      continue;

    // Collect all loads.
    std::set<Instruction *> LoadsInGroup;
    for (auto UI : GV.users()) {
      if (auto LI = dyn_cast<LoadInst>(UI)) {
        assert(LI->getPointerOperand() == &GV);
        auto Fn = LI->getParent()->getParent();
        // Check this load is inside the group.
        if (std::find(FG->begin(), FG->end(), Fn) != FG->end())
          LoadsInGroup.insert(LI);
      }
      // Global variable is used in a constexpr.
      if (&GV != getUnderlyingGlobalVariable(UI))
        continue;
      for (auto U : UI->users())
        if (auto LI = dyn_cast<LoadInst>(U)) {
          auto Fn = LI->getParent()->getParent();
          // Check this load is inside the group.
          if (std::find(FG->begin(), FG->end(), Fn) != FG->end())
            LoadsInGroup.insert(LI);
        }
    }

    // Do coalescing.
    for (auto LI : LoadsInGroup) {
      LiveRange *LR2 = Liveness->getLiveRange(LI);
      LR1 = Liveness->coalesce(LR1, LR2, false);
    }
  }
}

/***********************************************************************
 * insertCopy : insert a copy of a non-struct value
 *
 * Enter:   Input = value to copy
 *          LR = live range to add the new value to
 *          InsertBefore = insert copy before this inst
 *          Name = name to give the new value
 *          Number = number to give the new instruction(s)
 *
 * Return:  The new copy instruction
 *
 * This inserts multiple copies if the input value is a vector that is
 * bigger than two GRFs or a non power of two size.
 */
Instruction *GenXCoalescing::insertCopy(SimpleValue Input, LiveRange *LR,
    Instruction *InsertBefore, StringRef Name, unsigned Number)
{
  assert(!isa<Constant>(Input.getValue()));
  if (auto ST = dyn_cast<StructType>(Input.getValue()->getType())) {
    // Input is a struct element. First extract it. This
    // extract is created coalesced by adding it to the live
    // range of the struct element. An extractvalue is always
    // coalesced and never generates code.
    SmallVector<unsigned, 4> Indices;
    IndexFlattener::unflatten(ST, Input.getIndex(), &Indices);
    Instruction *Extract = ExtractValueInst::Create(Input.getValue(), Indices,
        "twoaddr.extract", InsertBefore);
    auto SourceLR = Liveness->getLiveRange(Input);
    assert(SourceLR);
    Liveness->setLiveRange(SimpleValue(Extract), SourceLR);
    Input = SimpleValue(Extract);
  }
  return Liveness->insertCopy(Input.getValue(), LR, InsertBefore, Name, Number,
                              ST);
}

/***********************************************************************
 * insertIntoStruct : create an insertvalue to insert a new value into a
 *                    struct
 *
 * Enter:   Ty = type of putative struct
 *          FlattenedIndex = flattened index within the struct
 *          OldStruct = old value of struct
 *          NewVal = new value to insert into it
 *          InsertBefore = where to insert new instruction before
 *
 * Return:  the new InsertValueInst
 *
 * If Ty is not a struct type, this just returns NewVal.
 */
Instruction *GenXCoalescing::insertIntoStruct(Type *Ty,
    unsigned FlattenedIndex, Value *OldStruct, Instruction *NewVal,
    Instruction *InsertBefore)
{
  auto ST = dyn_cast<StructType>(Ty);
  if (!ST)
    return NewVal;
  // We're copying into struct element. We need to add an insertvalue.
  SmallVector<unsigned, 4> Indices;
  IndexFlattener::unflatten(ST, FlattenedIndex, &Indices);
  return InsertValueInst::Create(OldStruct, NewVal,
      Indices, "coalescefail.insert", InsertBefore);
}

/***********************************************************************
 * showCoalesceFail : output a message to say that coalescing has failed
 */
void GenXCoalescing::showCoalesceFail(SimpleValue V, const DebugLoc &DL,
                                      const char *Intro, LiveRange *DestLR,
                                      LiveRange *SourceLR) {
  if (isa<Constant>(V.getValue()))
    return;
  if (V.getType()->getPrimitiveSizeInBits() >=
      GenXShowCoalesceFailThreshold * 8U) {
    dbgs() << "GenX " << Intro << " coalesce failed on ";
    V.printName(dbgs());
    dbgs() << " size " << V.getType()->getPrimitiveSizeInBits() / 8U
           << " bytes at ";
    DL.print(dbgs());
    dbgs() << "\nDestLR: " << *DestLR << "\nSourceLR: " << *SourceLR << "\n";
  }
}

/***********************************************************************
* DiagnosticInfoFastComposition initializer from Instruction
*
* If the Instruction has a DebugLoc, then that is used for the error
* location.
* Otherwise, the location is unknown.
*/
DiagnosticInfoFastComposition::DiagnosticInfoFastComposition(Instruction *Inst,
  const Twine &Desc, DiagnosticSeverity Severity)
  : DiagnosticInfo(getKindID(), Severity), Line(0), Col(0)
{
  auto DL = Inst->getDebugLoc();
  if (!DL) {
    Filename = DL->getFilename();
    Line = DL.getLine();
    Col = DL.getCol();
  }
  Description = (Twine("Fast Composition restriction violation")
    + ": " + Desc).str();
}

/***********************************************************************
* DiagnosticInfoFastComposition::print : print the error/warning message
*/
void DiagnosticInfoFastComposition::print(DiagnosticPrinter &DP) const
{
  std::string Loc(
    (Twine(!Filename.empty() ? Filename : "<unknown>")
    + ":" + Twine(Line)
    + (!Col ? Twine() : Twine(":") + Twine(Col))
    + ": ")
    .str());
  DP << Loc << Description;
}


