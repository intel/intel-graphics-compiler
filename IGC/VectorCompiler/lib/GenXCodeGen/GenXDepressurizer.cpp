/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXDepressurizer
/// -----------------
///
/// GenXDepressurizer is a pass that identifies where register pressure is
/// excessive, and attempts to sink and/or clone definitions past that area to
/// reduce register pressure.
///
/// Currently the pass is enabled to handle only flag (predicate) values. It is
/// supposed to work for general values, but that is not yet enabled and it may
/// require some bug fixing and fine tuning before it is.
///
/// In fact this pass is now viewed as a dead end. The plan to replace it is a
/// pass that does register allocation as if into Gen's real registers, doing
/// live range splitting and rematerialization where required, to help undo the
/// register-pressure-increasing effects of CSE and LICM where it would cause a
/// spill.
///
/// The basic idea of the existing GenXDepressurizer pass:
///
/// 1. Scan the code backwards, keeping track of what values are live and what
///    the register pressure is (total size of all live values, also the total
///    size for flag (predicate) values).
///
/// 2. Where register pressure becomes excessive, look at currently live values
///    to see if any is a definition that could profitably be sunk to below the
///    current point.
///
/// 3. Sink any such instructions until register pressure is no longer
///    excessive.
///
/// 4. For a flag value, "profitably be sunk" includes the case that it
///    decreases flag register pressure but increases overall register pressure
///    (by, for instance, lengthening the live ranges of the inputs to a cmp),
///    but general register pressure is not high at the current point.
///
/// 5. A flag value that does not require cloning (all uses are dominated by the
///    current point) is sunk anyway, as long as it does not push an already
///    high general pressure up higher.
///
/// Point 5 means that this pass replaces GenXCodeSinking, which sank any single
/// use flag value.
///
/// There are some complications to the scheme:
///
/// * How do we scan code backwards in a way that keeps track of pressure when
///   there is control flow, particularly loops?
///
/// * When considering a definition to sink, we need to know whether a
///   particular use is reachable from the current point, and whether it is
///   dominated by it.
///
/// Backwards scanning order and pseudo CFG
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// In order to keep track of liveness and pressure as we scan backwards, we
/// want to scan the basic blocks in an order that ensures that we do not scan
/// a particular basic block until we have scanned all its successors.  In that
/// way we can easily gather the live out set of the basic block from the live
/// in of each successor, modified by the incoming for our block in the phi
/// nodes in the successor. (If there are phi nodes, there is only one
/// successor, because critical edges have been split.)
///
/// A loop needs special consideration. We want to scan all of the blocks of a
/// loop (including inner loops) in one go, after scanning all possible
/// successors of the loop, and before scanning the predecessor(s) of the loop
/// header. Within the loop, we want to start at the backedge predecessor(s),
/// but we need to set up the liveness at the end of a backedge predecessor to
/// take account of
///
/// a. any value that is live in to the loop and live out of the loop at some
///    loop exit, and
///
/// b. any value that is defined in the loop and is live round the backedge.
///
/// Superbales
/// ^^^^^^^^^^
///
/// Sinking is performed in units of a superbale.
///
/// For a general value, a superbale is the bale that defines the value, and,
/// if that is a wrregion, the rest of the chain of wrregion bales that write
/// to other parts of that value and have the same inputs as the defining bale.
/// We consider such a superbale as a whole because considering and sinking
/// just the bale would not show any benefit, because it has an input to the
/// wrregion the same size as the result. Such a chain of wrregions typically
/// arises from legalization where vector decomposing has not subsequently been
/// able to split the big vector up.
///
/// For a flag value, a superbale is a tree where each non-leaf node is an
/// and/or/xor/not instruction acting on predicates. Again this is done because
/// sinking just an and/or/xor/not instruction would not show any benefit to
/// flag pressure.
///
//===----------------------------------------------------------------------===//

#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXModule.h"
#include "GenXUtil.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "GENX_DEPRESSURIZER"

static cl::opt<unsigned>
    LimitGenXDepressurizer("limit-genx-depressurizer", cl::init(UINT_MAX),
                           cl::Hidden, cl::desc("Limit GenX depressurizer."));

STATISTIC(NumSunk, "Number of instructions sunk");
STATISTIC(NumCloned, "Number of instructions cloned");

namespace {

// PseudoCFG : the pseudo control flow graph for a function
//
// The pseudo CFG is a graph of the basic blocks in a function, similar to the
// real CFG, but with the following differences:
//  * It is acyclic
//  * Therefore there are no loop backedges
//  * What was a loop backedge in the real CFG is replaced by special "loop
//    exit" edges from what was the loop backedge predecessor of the loop
//    header to each loop exit block, also pointing to the loop header.
//  * This only works if the real CFG is reducible. Any unnatural loops in the
//    real CFG are probably not properly represented in the pseudoCFG.
//
// The pseudo CFG also provides an ordering of blocks such that a block is not
// visited until all its predecessors have been. Because of the above changes
// in the pseudo CFG, this also has the property that, once we get to a
// (natural) loop header, all blocks in the loop are processed before anything
// else.
//
// The pseudo CFG provides a way to propagate liveness backwards through the
// function:
//  * Visit blocks in the reverse of the pseudo CFG ordering, such that no
//    block is visited until all its successors have been, and no part of a
//    loop is visited until all of the loop exits have been.
//  * For a block:
//    1. initialize the live out with (real CFG) successors' corresponding phi
//       incomings;
//    2. for a normal edge, propagate the successor's live in to this block's
//       live out;
//    3. for a "loop exit" edge, propagate the successor's live in to this
//       block's live out, but only for values that are defined before the loop
//       header, i.e. in a block that would be visited by the loop header in
//       this visit order.
//    This provides the correct liveness for any particular point within a loop
//    for these cases:
//      a. a value that is used after this point in the loop (from 2);
//      b. a value that is live round any backedge reachable from this point
//         (from 1);
//      c. a value that is defined in the loop and used after the loop via a
//         loop exit reachable from this point (from 2);
//      d. a value that is defined before the loop and used after the loop, and
//         is thus live through the whole loop (from 3).
//
// If the real CFG is irreducible, then this liveness information will be
// inaccurate.
//
class PseudoCFG {
public:
  struct Node {
    friend PseudoCFG;
    SmallVector<BasicBlock *, 4> Preds;
    SmallVector<BasicBlock *, 4> Succs;
    BasicBlock *LoopHeader;
    Node() : LoopHeader(nullptr) {}

  public:
    void removeSucc(BasicBlock *Succ);
    void removePred(BasicBlock *Pred);
    // getLoopHeader : normally returns 0. If this is a backedge node,
    // returns the corresponding loop header block
    BasicBlock *getLoopHeader() { return LoopHeader; }
    // pred and succ iterators
    typedef SmallVectorImpl<BasicBlock *>::iterator pred_iterator;
    pred_iterator pred_begin() { return Preds.begin(); }
    pred_iterator pred_end() { return Preds.end(); }
    typedef SmallVectorImpl<BasicBlock *>::iterator succ_iterator;
    succ_iterator succ_begin() { return Succs.begin(); }
    succ_iterator succ_end() { return Succs.end(); }
  };

private:
  std::vector<BasicBlock *> Ordering;
  std::map<BasicBlock *, Node> Nodes;

public:
  void clear() {
    Ordering.clear();
    Nodes.clear();
  }
  // compute : compute the pseudo CFG for the function.
  // It is assumed that critical edges have been split.
  void compute(Function *F, DominatorTree *DT,
               LoopInfoBase<BasicBlock, Loop> *LI);
  // getNode : get pseudo CFG node for basic block
  Node *getNode(BasicBlock *BB) { return &Nodes[BB]; }
  // iterators through the ordering
  typedef std::vector<BasicBlock *>::iterator iterator;
  iterator begin() { return Ordering.begin(); }
  iterator end() { return Ordering.end(); }
  typedef std::vector<BasicBlock *>::reverse_iterator reverse_iterator;
  reverse_iterator rbegin() { return Ordering.rbegin(); }
  reverse_iterator rend() { return Ordering.rend(); }
  // Debug dump/print
  void dump() { print(dbgs()); }
  void print(raw_ostream &OS);
};

// Liveness : the liveness information at some point in the program
// This class is local to this source file and completely unrelated to
// GenXLiveness.
class Liveness {
public:
  enum Category { GENERAL, FLAG, ADDR, NUMCATS };

private:
  std::set<Value *> Values[NUMCATS];
  unsigned Pressure; // overall register pressure
  unsigned Pressures[NUMCATS]; // pressure for each individual category
public:
  Liveness() : Pressure(0) {
    for (unsigned Cat = 0; Cat != NUMCATS; ++Cat)
      Pressures[Cat] = 0;
  }
  static bool isFlag(Value *V) {
    return V->getType()->getScalarType()->isIntegerTy(1);
  }
  static bool isAddr(Value *V) {
    if (!V->getType()->getScalarType()->isIntegerTy(16))
      return false;
    switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
    case GenXIntrinsic::genx_convert_addr:
    case GenXIntrinsic::genx_add_addr:
      return true;
    default:
      break;
    }
    return false;
  }
  static unsigned getValueSize(Value *V);
  void copyFrom(Liveness *Other);
  void addValue(Value *V);
  bool removeValue(Value *V);
  void copyValues(Liveness *Other);
  unsigned getPressure(unsigned Cat) { return Pressures[Cat]; }
  unsigned getPressure() { return Pressure; }
  bool contains(Value *V) {
    auto ValueSet = &Values[GENERAL];
    if (isFlag(V))
      ValueSet = &Values[FLAG];
    else if (isAddr(V))
      ValueSet = &Values[ADDR];
    return ValueSet->find(V) != ValueSet->end();
  }
  // Iterator (over set of values)
  typedef std::set<Value *>::iterator iterator;
  iterator begin(unsigned Cat) { return Values[Cat].begin(); }
  iterator end(unsigned Cat) { return Values[Cat].end(); }
  unsigned cat_begin() { return 0; }
  unsigned cat_end() { return NUMCATS; }
  // Debug print and dump
  void print(raw_ostream &OS);
  void dump() { print(dbgs()); dbgs() << '\n'; }
};

// Superbale : a sequence of bales where each is headed by a wrregion whose
// "old value of vector" input is the previous bale, and the other operands of
// the bales are all the same.
struct Superbale {
  // Instruction number of head instruction of superbale
  unsigned Number;
  // Bale head instructions, stored in reverse of code order
  SmallVector<Instruction *, 8> Bales;
  // Operands (some entries can be nullptr)
  SmallVector<Value *, 8> Operands;
  Instruction *getHead() { return Bales[0]; }
  void print(raw_ostream &OS);
  void dump() { print(dbgs()); dbgs() << '\n'; }
};

// SinkCandidate : a candidate superbale for sinking
struct SinkCandidate {
  Superbale *SB;
  int Benefit;
  bool AllUsesDominatedByHere;
  SinkCandidate(Superbale *SB, int Benefit, bool AUDBH)
      : SB(SB), Benefit(Benefit), AllUsesDominatedByHere(AUDBH) {}
  // Sort by whether all uses are dominated by here, then by best benefit, then
  // by latest definition point.
  bool operator<(const SinkCandidate &Rhs) const {
    if (AllUsesDominatedByHere != Rhs.AllUsesDominatedByHere)
      return AllUsesDominatedByHere > Rhs.AllUsesDominatedByHere;
    if (Benefit != Rhs.Benefit)
      return Benefit > Rhs.Benefit;
    if (SB == nullptr)
      return false;
    if (Rhs.SB == nullptr)
      return true;
    return SB->Number > Rhs.SB->Number;
  }
};

// GenX depressurizer pass
class GenXDepressurizer : public FGPassImplInterface,
                          public IDMixin<GenXDepressurizer> {
  const unsigned FlagThreshold = 6;
  const unsigned AddrThreshold = 32;
  unsigned GRFThreshold = 0;
  unsigned FlagGRFTolerance = 0;
  bool Modified = false;
  GenXGroupBaling *Baling = nullptr;
  const GenXBackendConfig *BC = nullptr;
  DominatorTree *DT = nullptr;
  LoopInfoBase<BasicBlock, Loop> *LI = nullptr;
  PseudoCFG *PCFG = nullptr;
  unsigned MaxPressure = 0;
  std::map<Function *, unsigned> SubroutinePressures;
  std::map<BasicBlock *, Liveness> LiveIn;
  std::map<BasicBlock *, Liveness> LiveOut;
  Liveness *Live = nullptr;
  // A numbering of instructions. Because of the way the basic block ordering
  // is constructed, if instruction I2 is reachable from instruction I1, then
  // InstNumbers[I1] < InstNumbers[I2], unless the reachability is via a
  // loop backedge. The converse is not necessarily true.
  std::map<Instruction *, unsigned> InstNumbers;
  std::map<Value *, CallInst *> TwoAddrValueMap;

  SmallDenseMap<Instruction *, unsigned> InstFenceDomain;

  unsigned SunkCount = 0;

public:
  explicit GenXDepressurizer() {}
  static StringRef getPassName() { return "GenX register pressure reducer"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void processFunction(Function *F);
  void orderAndNumber(Function *F);
  void processBasicBlock(BasicBlock *BB);
  void getLiveOut(BasicBlock *BB, Liveness *Live);
  void processInstruction(Instruction *Inst);
  void attemptSinking(Instruction *InsertBefore, std::set<Value *> *Exclude,
                      Liveness::Category Cat, bool AllowClone);
  bool sink(Instruction *InsertBefore, Superbale *SB, bool AllowClone = false);
  bool sinkOnce(Instruction *InsertBefore, Superbale *SB, ArrayRef<Use *> Uses);
  bool modifyLiveness(Liveness *Live, Superbale *SB);
  int  getSuperbaleKillSize(Superbale *SB);
  int  getSinkBenefit(Superbale *SB, Liveness::Category Cat, unsigned Headroom);
  bool fillSuperbale(Superbale *SB, Instruction *Inst, bool IsFlag);
  void MergeCandidate(SinkCandidate &Lhs, SinkCandidate &Rhs);
  void fillTwoAddrValueMap(BasicBlock *BB);
};

} // end anonymous namespace

namespace llvm {
void initializeGenXDepressurizerWrapperPass(PassRegistry &);
using GenXDepressurizerWrapper = FunctionGroupWrapperPass<GenXDepressurizer>;
}
INITIALIZE_PASS_BEGIN(GenXDepressurizerWrapper, "GenXDepressurizerWrapper",
                      "GenXDepressurizerWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPassWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXDepressurizerWrapper, "GenXDepressurizerWrapper",
                    "GenXDepressurizerWrapper", false, false)

ModulePass *llvm::createGenXDepressurizerWrapperPass() {
  initializeGenXDepressurizerWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXDepressurizerWrapper();
}

void GenXDepressurizer::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<DominatorTreeGroupWrapperPass>();
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXBackendConfig>();
  AU.addPreserved<DominatorTreeGroupWrapperPass>();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<GenXLiveness>();
  AU.addPreserved<GenXGroupBaling>();
  AU.addPreserved<GenXBackendConfig>();
  AU.addPreserved<FunctionGroupAnalysis>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * runOnFunctionGroup : run the register pressure reduction pass for
 *      this FunctionGroup
 */
bool GenXDepressurizer::runOnFunctionGroup(FunctionGroup &FG) {
  if (skipOptWithLargeBlock(FG))
    return false;

  Modified = false;
  SunkCount = 0;
  Baling = &getAnalysis<GenXGroupBaling>();
  BC = &getAnalysis<GenXBackendConfig>();
  GRFThreshold = BC->getDepressurizerGRFThreshold();
  FlagGRFTolerance = BC->getDepressurizerFlagGRFTolerance();
  // Process functions in the function group in reverse order, so we know the
  // max pressure in a subroutine when we see a call to it.
  for (auto fgi = FG.rbegin(), fge = FG.rend(); fgi != fge; ++fgi) {
    Function *F = *fgi;
    processFunction(F);
    SubroutinePressures[F] = MaxPressure;
  }
  SubroutinePressures.clear();
  return Modified;
}

/***********************************************************************
 * processFunction : run depressurizer on one function
 */
void GenXDepressurizer::processFunction(Function *F) {
  LLVM_DEBUG(dbgs() << "GenXDepressurizer on function " << F->getName() << '\n');
  MaxPressure = 0;
  DT = getAnalysis<DominatorTreeGroupWrapperPass>().getDomTree(F);
  LI = new LoopInfoBase<BasicBlock, Loop>();
  LI->analyze(*DT);
  // Calculate the pseudo CFG.
  PCFG = new PseudoCFG();
  PCFG->compute(F, DT, LI);
  // Order and number the instructions.
  orderAndNumber(F);
  // Visit each basic block.
  MaxPressure = 0;
  for (auto ri = PCFG->rbegin(), re = PCFG->rend(); ri != re; ++ri) {
    processBasicBlock(*ri);
  }

  delete PCFG;
  delete LI;
  LLVM_DEBUG(dbgs() << "max pressure " << MaxPressure << " for function "
               << F->getName() << '\n');
  SubroutinePressures[F] = MaxPressure;
}

static bool isSchedulerFence(Instruction *Inst) {
  auto *CI = dyn_cast<CallInst>(Inst);
  if (!CI)
    return false;

  if (CI->isInlineAsm())
    return true;

  auto IID = vc::getAnyIntrinsicID(CI);
  if (IID != GenXIntrinsic::genx_fence)
    return false;

  auto FenceKind = cast<ConstantInt>(CI->getArgOperand(0))->getZExtValue();

  return FenceKind & 0x80;
}

/***********************************************************************
 * orderAndNumber : order and number the instructions
 *
 * This has three purposes:
 *
 * 1. ensure the instructions in a bale are adjacent;
 *
 * 2. for a boolean and/or, ensure that a tree of bales (where each bale has
 *    a single use that is its parent in the tree, in the same basic block) is
 *    adjacent and in depth first order to minimize flag pressure in a tree of
 *    boolean ops;
 *
 * 3. number the instructions, with each instruction in a bale given the same
 *    number.
 *
 * This scans the code backwards, so numbers backwards starting at a high
 * number.
 */
void GenXDepressurizer::orderAndNumber(Function *F) {
  unsigned InstNum = std::numeric_limits<unsigned>::max();
  unsigned FenceNum = InstNum;
  for (auto fi = PCFG->rbegin(), fe = PCFG->rend(); fi != fe; ++fi) {
    BasicBlock *BB = *fi;
    auto *Inst = &BB->back();
    for (;;) {
      InstNum--;

      if (isSchedulerFence(Inst))
        FenceNum--;

      if (isa<PHINode>(Inst)) {
        InstNumbers[Inst] = InstNum;
        InstFenceDomain[Inst] = FenceNum;
      } else {
        Bale B;
        Baling->buildBale(Inst, &B);
        auto InsertBefore = Inst;
        // Move the bale instructions to a contiguous lump, and number them.
        Instruction *GotoJoin = nullptr;
        for (auto ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
          Inst = ii->Inst;
          InstNumbers[Inst] = InstNum;
          InstFenceDomain[Inst] = FenceNum;
          if (Inst == InsertBefore)
            continue;
          switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
          case GenXIntrinsic::genx_simdcf_goto:
          case GenXIntrinsic::genx_simdcf_join:
            GotoJoin = Inst;
            break;
          default:
            break;
          }
          // Surfaces are created after GenXCategory, placed before
          // the dependend instruction and are not baled with anything.
          // It is better to move the surface next to its user since
          // it increases chances to get an immediate descriptor.
          for (auto &U : Inst->operands()) {
            auto *UInst = dyn_cast<Instruction>(U);
            auto IID = GenXIntrinsic::getGenXIntrinsicID(UInst);
            if (IID == GenXIntrinsic::genx_convert &&
                isa<Constant>(UInst->getOperand(0))) {
              // UInst will be numbered in the next iterations.
              IGC_ASSERT(UInst->hasOneUse());
              UInst->removeFromParent();
              UInst->insertBefore(InsertBefore);
            }
          }
          Inst->removeFromParent();
          Inst->insertBefore(InsertBefore);
        }
        if (GotoJoin) {
          // For a goto/join, check that its outside-bale uses are also moved,
          // and number the instructions.
          // This is the only case of an inside-bale instruction having
          // outside-bale uses.
          // This is a bit of a bodge, which we'll tolerate for now on the
          // basis that this pass will go away once we have a better pass for
          // detecting register pressure and alleviating it by moving code and
          // rematerializing.
          SmallVector<Instruction *, 2> Users;
          for (auto ui = GotoJoin->use_begin(), ue = GotoJoin->use_end();
               ui != ue; ++ui)
            Users.push_back(cast<Instruction>(ui->getUser()));
          Instruction *InsertBefore = GotoJoin->getNextNode();
          for (auto ui = Users.begin(), ue = Users.end(); ui != ue; ++ui) {
            Instruction *User = *ui;
            if (!isa<VectorType>(User->getType())) {
              // Skip the use that is in the bale. We are relying on the use
              // in the bale being the only extractvalue that is scalar; the
              // other two (for goto) or one (for join) are vector (the EM and
              // RM values).
              continue;
            }
            if (User->getParent() == GotoJoin->getParent()) {
              // Only move the extractvalue if it is in the same basic block.
              User->removeFromParent();
              User->insertBefore(InsertBefore);
              InstNumbers[User] = InstNum;
              InstFenceDomain[User] = FenceNum;
            }
          }
        }
        Inst = B.getHead()->Inst;
        if (Inst->getType()->getScalarType()->isIntegerTy(1) &&
            (Inst->getOpcode() == Instruction::And ||
             Inst->getOpcode() == Instruction::Or)) {
          // Now look at the operands. Any that is a single use instruction in
          // the same basic block is moved. The rest of its bale, and that
          // bale's own operands, get moved when it is later processed in the
          // loop.
          InsertBefore = B.begin()->Inst;
          for (auto ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
            Inst = ii->Inst;
            for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi) {
              if (ii->Info.isOperandBaled(oi))
                continue; // only consider out-of-bale operands
              auto OpndInst = dyn_cast<Instruction>(Inst->getOperand(oi));
              if (!OpndInst)
                continue;
              if (OpndInst->getParent() != BB)
                continue;
              if (isa<PHINode>(OpndInst))
                continue;
              if (!OpndInst->hasOneUse())
                continue;
              OpndInst->removeFromParent();
              OpndInst->insertBefore(InsertBefore);
            }
          }
        }
        // On to the previous instruction, which is now the one before the
        // first instruction in the current bale.
        Inst = B.begin()->Inst;
      }
      if (Inst == &BB->front())
        break;
      Inst = Inst->getPrevNode();
    }
  }
}

/***********************************************************************
 * processBasicBlock : process one basic block
 */
void GenXDepressurizer::processBasicBlock(BasicBlock *BB) {
  // Create a new empty entry for this BB in the LiveIn map, and use it for
  // keeping track of liveness as we scan backwards through the block.
  Live = &LiveIn[BB];
  // Populate Live with the live out values.
  getLiveOut(BB, Live);

  fillTwoAddrValueMap(BB);
  // Scan backwards through the block, excluding phi nodes.
  auto Inst = &BB->back();
  for (;;) {
    if (isa<PHINode>(Inst))
      break;
    processInstruction(Inst);
    if (Inst == &BB->front())
      break;
    Inst = Inst->getPrevNode();
  }
  // Just before the first (non-phi) instruction, attempt sinking of flag
  // values, as long as non-flag pressure is low, and as long as this is not a
  // join label.
  if (!GotoJoin::isJoinLabel(BB) && FlagGRFTolerance > Live->getPressure())
    attemptSinking(BB->getFirstNonPHI(), nullptr, Liveness::FLAG,
                   /*AllowClone=*/false);
}

/***********************************************************************
 * getLiveOut : populate empty Liveness with the live out of the BB
 */
void GenXDepressurizer::getLiveOut(BasicBlock *BB, Liveness *Live) {
  // Get each successor's live in values into our liveness. If getLoopHeader
  // returns non-0, then we are looking at a loop backedge and we only want
  // to get successors' live in values if they are defined before the loop
  // header.
  unsigned LoopHeaderNum = 0;
  auto BBNode = PCFG->getNode(BB);
  if (auto LoopHeader = BBNode->getLoopHeader())
    LoopHeaderNum = InstNumbers[&LoopHeader->front()];
  for (auto si = BBNode->succ_begin(), se = BBNode->succ_end(); si != se;
       ++si) {
    auto LI = &LiveIn[*si];
    for (auto ci = LI->cat_begin(), ce = LI->cat_end(); ci != ce; ++ci)
      for (auto vi = LI->begin(ci), ve = LI->end(ci); vi != ve; ++vi) {
        Value *V = *vi;
        if (auto Inst = dyn_cast<Instruction>(V))
          if (LoopHeaderNum && LoopHeaderNum <= InstNumbers[Inst])
            continue; // Ignore instruction defined in loop from loop exit succ
        Live->addValue(V);
      }
  }
  // Now adjust the liveness for the phi nodes of each real CFG successor. This
  // includes the case that this is a backedge and the real CFG successor is
  // the loop header; this is how we get defs inside the loop into our
  // liveness.
  auto TI = BB->getTerminator();
  for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
    auto Succ = TI->getSuccessor(i);
    for (auto ii = Succ->begin();; ++ii) {
      auto Phi = dyn_cast<PHINode>(&*ii);
      if (!Phi)
        break;
      Live->removeValue(Phi);
      IGC_ASSERT_EXIT(Phi->getBasicBlockIndex(BB) >= 0);
      Live->addValue(Phi->getIncomingValue(Phi->getBasicBlockIndex(BB)));
    }
  }
  if (MaxPressure < Live->getPressure()) {
    MaxPressure = Live->getPressure();
    LLVM_DEBUG(dbgs() << "max pressure now " << MaxPressure << '\n');
  }
  LLVM_DEBUG(dbgs() << "getLiveOut(" << BB->getName() << "): ";
             Live->print(dbgs()); dbgs() << '\n');
  // Copy the liveness to the LiveOut entry for this BB.
  LiveOut[BB].copyFrom(Live);
}

/***********************************************************************
 * processInstruction : process one instruction in backwards scan of BB
 *
 * Return:  Prev = previous instruction, i.e. next one to scan
 */
void GenXDepressurizer::processInstruction(Instruction *Inst) {
  if (!Inst)
    return;
  if (Baling->isBaled(Inst))
    return; // Not head of bale, ignore
  if (isa<DbgInfoIntrinsic>(Inst))
    return; // debug info intrinsics are ignored
  if (isa<ExtractValueInst>(Inst))
    return; // Too confusing to consider sinking when we get to an extractvalue
            // out of a goto/join, so ignore.
  Bale B;
  Baling->buildBale(Inst, &B);
  LLVM_DEBUG(dbgs() << '[' << InstNumbers[Inst] << ']';
             if (Inst->getDebugLoc()) dbgs()
             << " {line " << Inst->getDebugLoc().getLine() << '}';
             B.print(dbgs()));
  unsigned OldFlagPressure = Live->getPressure(Liveness::FLAG);
  // Remove the result of the bale from liveness.
  Live->removeValue(Inst);
  // If this is a non-intrisic call, add the max pressure from inside the call.
  if (auto CI = dyn_cast<CallInst>(Inst)) {
    if (!vc::isAnyNonTrivialIntrinsic(CI)) {
      LLVM_DEBUG(dbgs() << "pressure inside subroutine: "
                        << SubroutinePressures[CI->getCalledFunction()]
                        << '\n');
      unsigned AddedPressure =
          Live->getPressure() + SubroutinePressures[CI->getCalledFunction()];
      if (MaxPressure < AddedPressure) {
        MaxPressure = AddedPressure;
        LLVM_DEBUG(dbgs() << "max pressure now " << MaxPressure << '\n');
      }
    }
  }
  // Add operands from outside the bale to liveness. Also keep them in a
  // separate set for the use of attemptSinking.
  std::set<Value *> BaleOperands;
  for (auto bi = B.rbegin(), be = B.rend(); bi != be; ++bi) {
    BaleInst *BI = &*bi;
    for (unsigned ii = 0, ie = BI->Inst->getNumOperands(); ii != ie; ++ii) {
      if (!BI->Info.isOperandBaled(ii)) {
        Value *Opnd = BI->Inst->getOperand(ii);
        if (isa<Argument>(Opnd) || isa<Instruction>(Opnd)) {
          Live->addValue(Opnd);
          BaleOperands.insert(Opnd);
        }
      }
    }
  }
  LLVM_DEBUG(Live->print(dbgs()); dbgs() << '\n');
  if (Inst && Inst->isTerminator())
    return; // Do not attempt to sink past last instruction in block.

  // FIXME: This does not deal with a subroutine call instruction, where
  // pressure goes up during the call and then comes back down again on
  // return. I think the last remaining flag spill in HEVCEnc_PB is because
  // of this; a CSEd flag is live over a subroutine call but we do not notice
  // that increased flag pressure inside the call should force the flag def
  // to be cloned and sunk.

  // Attempt sinking of flag values if necessary. Do not do that if non-flag
  // pressure is already high. If flag pressure has just gone high, sink any
  // flag value (with a benefit). Otherwise, only sink single use flag values.
  if (FlagGRFTolerance > Live->getPressure()) {
    bool AllowClone = OldFlagPressure <= FlagThreshold &&
                      Live->getPressure(Liveness::FLAG) > FlagThreshold;
    attemptSinking(Inst->getNextNode(), &BaleOperands, Liveness::FLAG,
                   AllowClone);
  }

  // Attemp sinking of address values if necessary.
  if (Live->getPressure(Liveness::ADDR) > AddrThreshold)
    attemptSinking(Inst->getNextNode(), &BaleOperands, Liveness::ADDR,
                   /*AllowClone=*/false);

  // Attempt sinking of non-flag value(s) if necessary.
  if (Live->getPressure() > GRFThreshold)
    attemptSinking(Inst->getNextNode(), &BaleOperands, Liveness::GENERAL,
                   /*AllowClone=*/false);

  if (MaxPressure < Live->getPressure()) {
    MaxPressure = Live->getPressure();
    LLVM_DEBUG(dbgs() << "max pressure up to " << MaxPressure << '\n');
  }
}

/***********************************************************************
 * attemptSinking : attempt some sinking to reduce pressure
 *
 * Enter:   InsertBefore = instruction to insert sunk instruction before
 *          Exclude = 0 else exclude any sink candidate in this set (used to
 *                    exclude superbales used in the present bale)
 *          FlagSinking = true to sink flags
 *          AllowClone = true to sink anything suitable, false to only sink
 *                       when cloning is not required, used to sink flag defs
 *                       even when flag pressure is low.
 *
 * This is called in three different ways:
 *
 * FlagSinking, !AllowClone: sink any flag def whose uses are all dominated by
 * the current position (quit if normal pressure gets too high)
 *
 * FlagSinking, AllowClone: sink any flag def, preferring ones that do not need
 * a clone, but switch to !AllowClone mode once flag pressure is low enough
 * (and quit if normal pressure gets too high)
 *
 * !FlagSinking, AllowClone: sink normal (non-flag) def if it provides a
 * benefit to pressure, until pressure is low enough.
 */
void GenXDepressurizer::attemptSinking(Instruction *InsertBefore,
                                       std::set<Value *> *Exclude,
                                       Liveness::Category Cat,
                                       bool AllowClone) {
  LLVM_DEBUG(dbgs() << "attemptSinking(Cat="
                    << (Cat == Liveness::FLAG   ? "flag"
                        : Cat == Liveness::ADDR ? "addr"
                                                : "general")
                    << ", AllowClone=" << AllowClone << ")\n");
  if (!InsertBefore)
    return;
  // Gather the currently live superbales with a sink benefit.
  // Exclude any that is used in the present bale.
  SmallVector<SinkCandidate, 8> Candidates;
  SmallVector<SinkCandidate, 8> SecondRound;
  std::map<Instruction *, Superbale> Superbales;
  unsigned CurNumber = InstNumbers[InsertBefore];
  unsigned FenceDomain = InstFenceDomain[InsertBefore];
  int Headroom = 0;
  switch (Cat) {
  case Liveness::FLAG:
    Headroom = FlagGRFTolerance - Live->getPressure();
    break;
  default:
    break;
  }
  for (auto i = Live->begin(Cat), e = Live->end(Cat); i != e; ++i) {
    if (Exclude && Exclude->find(*i) != Exclude->end())
      continue;
    auto Inst = dyn_cast<Instruction>(*i);
    if (!Inst)
      continue; // only instructions can sink, not args
    if (isa<PHINode>(Inst))
      continue; // cannot sink phi node
    if (isa<ExtractValueInst>(Inst))
      continue; // Don't sink extractvalue from a goto/join.
    if (Inst->mayHaveSideEffects() || Inst->mayReadOrWriteMemory())
      continue;
    // For this candidate, determine where its uses are, one of these cases:
    //
    // 1. All uses are dominated by here. This is the preferred case as the
    //    instruction can simply be sunk, with no cloning.
    // 2. Not all uses are reachable from here, but all uses that are reachable
    //    from here are dominated by here. This can be handled by a clone of
    //    the instruction, where the cloned instruction takes on the uses that
    //    are reachable from here.
    // 3. Other cases. We do not handle that, although we could enhance it in
    //    the future to handle this case by finding multiple sites to clone
    //    the instruction to.
    //
    // We are using "has a higher instruction number than" as a proxy for "is
    // reachable from", which in fact could include some uses that are not
    // reachable.
    bool AllUsesDominatedByHere = true;
    bool AllReachableUsesDominatedByHere = true;
    for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
      Instruction *user = cast<Instruction>(ui->getUser());
      if (InstNumbers[user] < CurNumber) {
        // Unreachable use.
        AllUsesDominatedByHere = false;
        continue;
      }
      if (InsertBefore->getParent() != user->getParent() &&
          !DT->dominates(InsertBefore->getParent(), user->getParent())) {
        AllReachableUsesDominatedByHere = false;
        break;
      }
    }
    if (!AllReachableUsesDominatedByHere)
      continue; // exclude case 3
    if (!AllowClone && !AllUsesDominatedByHere)
      continue; // exclude case 2 if !AllowClone
    bool IsFlag = Liveness::isFlag(Inst);
    bool IsAddr = Liveness::isAddr(Inst);
    if (!IsFlag && !IsAddr &&
        Inst->getType()->getPrimitiveSizeInBits() < 32 * 8) {
      // don't bother with anything smaller than a GRF unless it is a flag
      continue;
    }
    Superbale *SB = &Superbales[Inst];
    IGC_ASSERT(SB->Bales.empty());
    if (!fillSuperbale(SB, Inst, IsFlag))
      continue;

    // The compiler shouldn't sink instructions through a software fence. When
    // a user puts a fence into the code, the compiler assumes that the user has
    // a reason to do so. The compiler should respect the user's intention.
    if (FenceDomain != InstFenceDomain[SB->getHead()]) {
      LLVM_DEBUG(
          dbgs() << "could not sink/clone as it will cross the fence!\n");
      continue;
    }

    // Check whether the sink of this SB will cross its operands' two-addr
    // instructions, i.e.
    //
    // ... := use(v0); // SB.Head
    //
    // v1  := twoaddr(v0); // two-addr instruction.
    //
    // x <--- here this SB could be sunk to.
    //
    // In such case, sinking this SB should be avoided as it creates
    // overlapping between v0 and v1; otherwise, additional copy of v0 has to
    // be inserted. That won't alleviate the register pressure.
    bool CrossTwoAddr = false;
    for (auto OI = SB->Operands.begin(), OE = SB->Operands.end(); OI != OE;
         ++OI) {
      Value *Opnd = *OI;
      if (!TwoAddrValueMap.count(Opnd))
        continue;
      unsigned TwoAddrNum = InstNumbers[TwoAddrValueMap[Opnd]];
      unsigned SBNum = InstNumbers[SB->getHead()];
      // Ignore the case where the SB itself is a two-addr instruction or part
      // of chain of two-addr instructions.
      if (TwoAddrNum <= SBNum)
        continue;
      // Skip sinking/cloning if the current sinking point is beyond where the
      // two-addr instruction overwriting the same register.
      if (CurNumber > TwoAddrNum) {
        LLVM_DEBUG(
            dbgs() << "could not sink/clone as it will cross the two-addr "
                   << "instruction sharing the same operand!");
        CrossTwoAddr = true;
        break;
      }
    }
    if (CrossTwoAddr)
      continue;
    // Add the candidate.
    int Benefit = getSinkBenefit(SB, Cat, Headroom);
    LLVM_DEBUG(dbgs() << "candidate " << SB->getHead()->getName()
                      << " with benefit " << Benefit
                      << " and AllUsesDominatedByHere "
                      << AllUsesDominatedByHere << '\n');
    if (Benefit > 0)
      Candidates.push_back(SinkCandidate(SB, Benefit, AllUsesDominatedByHere));
    else if (AllUsesDominatedByHere)
      SecondRound.push_back(SinkCandidate(SB, Benefit, true));
  }
  if (!Candidates.empty()) {
    // Sort the candidates.
    std::sort(Candidates.begin(), Candidates.end());
    // Try each candidate.
    for (auto i = Candidates.begin(), e = Candidates.end(); i != e; ++i) {
      if (!AllowClone && !i->AllUsesDominatedByHere)
        continue; // Ignore candidate that needs cloning if AllowClone has
      // switched to false (i.e. flag pressure is low)
      if (sink(InsertBefore, i->SB)) {
        switch (Cat) {
        case Liveness::FLAG:
          if (Live->getPressure(Liveness::FLAG) <= FlagThreshold) {
            // Flag pressure is now low so we can stop sinking when a clone
            // is needed.
            AllowClone = false;
          }
          Headroom = FlagGRFTolerance - Live->getPressure();
          if (Headroom <= 0)
            return;
          break;
        case Liveness::ADDR:
          if (Live->getPressure(Liveness::ADDR) < AddrThreshold)
            return;
          break;
        default:
          if (Live->getPressure() < GRFThreshold)
            return;
          break;
        }
      } else if (i->AllUsesDominatedByHere) {
        SecondRound.push_back(*i);
      }
    }
  }
  if (AllowClone) {
    LLVM_DEBUG(dbgs() << "could not do enough sinking to alleviate pressure\n");
    if (Cat == Liveness::FLAG) {
      for (auto i = Candidates.begin(), e = Candidates.end(); i != e; ++i) {
        (void)sink(InsertBefore, i->SB, true);
      }
    }
  } else {
    // Try to sink a group of candidates to reduce register pressure.
    // Do NOT Allow Clone for now.
    for (auto i = SecondRound.begin(), ie = SecondRound.end(); i != ie; ++i) {
      if (i->SB == nullptr)
        continue;
      auto SB = i->SB;
      SmallSet<Value *, 8> OperandSet;
      for (auto k = SB->Operands.begin(), ke = SB->Operands.end(); k != ke; ++k)
        OperandSet.insert(*k);
      // find a group that shares the same input
      auto j = i;
      for (++j; j != ie; ++j) {
        if (j->SB == nullptr)
          continue;
        auto SB2 = j->SB;
        bool EqualInputs = (SB2->Operands.size() == SB->Operands.size());
        for (auto k = SB2->Operands.begin(), ke = SB2->Operands.end();
             EqualInputs && k != ke; ++k) {
          if (OperandSet.count(*k) == 0)
            EqualInputs = false;
        }
        // merge superbale if i covers j
        if (EqualInputs) {
          MergeCandidate(*i, *j);
        }
      }
    }
    // Sort the candidates.
    std::sort(SecondRound.begin(), SecondRound.end());
    // Try each candidate.
    for (auto i = SecondRound.begin(), e = SecondRound.end(); i != e; ++i) {
      if (i->Benefit <= 0 || i->SB == nullptr)
        break;
      if (!sink(InsertBefore, i->SB))
        LLVM_DEBUG(dbgs() << "Superbale sinking failed.";);
    }
  }
}

// Merge the Rhs into the Lhs candidate assuming that Rhs input operands
// are covered by the Lhs candidate
void GenXDepressurizer::MergeCandidate(SinkCandidate &Lhs, SinkCandidate &Rhs) {
  // update the benefit
  Lhs.Benefit += getSuperbaleKillSize(Rhs.SB);
  // merge superbale
  SmallVector<Instruction *, 8> Merge;
  auto a = Lhs.SB->Bales.begin();
  auto ae = Lhs.SB->Bales.end();
  auto b = Rhs.SB->Bales.begin();
  auto be = Rhs.SB->Bales.end();
  while (1) {
    if (a == ae && b == be)
      break;
    if (b == be) {
      Merge.push_back(*a);
      ++a;
    } else if (a == ae) {
      Merge.push_back(*b);
      ++b;
    } else if (InstNumbers[*b] > InstNumbers[*a]) {
      Merge.push_back(*b);
      ++b;
    } else {
      Merge.push_back(*a);
      ++a;
    }
  }
  Lhs.SB->Number = InstNumbers[Merge[0]];
  std::swap(Lhs.SB->Bales, Merge);
  Rhs.SB = nullptr;
  Rhs.Benefit = (-1);
}

void GenXDepressurizer::fillTwoAddrValueMap(BasicBlock *BB) {
  TwoAddrValueMap.clear();

  // Build two-addr operand -> instruction map for checking against two-addr
  // instructions.
  for (auto I = BB->rbegin(), E = BB->rend(); I != E; ++I) {
    auto CI = dyn_cast<CallInst>(&*I);
    if (!CI)
      continue;
    auto OpndNum = getTwoAddressOperandNum(CI);
    if (!OpndNum)
      continue;
    TwoAddrValueMap[I->getOperand(*OpndNum)] = CI;
  }
}

/***********************************************************************
 * sink : sink the superbale if possible
 *
 * Enter:   InsertBefore = instruction to insert before
 *          SB = the superbale to sink
 *
 * Return:  whether succeeded
 */
bool GenXDepressurizer::sink(Instruction *InsertBefore, Superbale *SB,
                             bool AllowClone) {
  if (++SunkCount > LimitGenXDepressurizer)
    return false;
  if (LimitGenXDepressurizer != UINT_MAX)
    dbgs() << "genx depressurizer " << SunkCount << '\n';
  unsigned CurNumber = InstNumbers[InsertBefore];
  LLVM_DEBUG(dbgs() << "sink(" << SB->getHead()->getName() << ")\n");
  // Gather the uses that we are going to modify.
  SmallVector<Use *, 4> UsesDominatedByHere;
  for (auto ui = SB->getHead()->use_begin(), ue = SB->getHead()->use_end();
       ui != ue; ++ui) {
    Use *U = &*ui;
    Instruction *user = cast<Instruction>(U->getUser());
    LLVM_DEBUG(dbgs() << " used in [" << InstNumbers[user] << "] "
                      << user->getName() << '\n');
    unsigned UserNumber = InstNumbers[user];
    if (UserNumber < CurNumber) {
      // Skip this user if cloning is allowed.
      if (AllowClone)
        continue;
      LLVM_DEBUG(dbgs() << "  rejecting: less than CurNumber " << CurNumber
                        << '\n');
      // This code was originally designed to cope with some uses not being
      // dominated by the sink site by cloning the superbale. But this gives an
      // assertion test on frc_iteration6_4x8_ipa. So I am disabling the cloning
      // functionality for now by rejecting the whole sink unless all uses are
      // dominated by the sink site. This also gives a few minor code size
      // improvements in examples too.
      return false;
    }
    UsesDominatedByHere.push_back(U);
  }

  if (UsesDominatedByHere.empty())
    return false;

  if (sinkOnce(InsertBefore, SB, UsesDominatedByHere)) {
    modifyLiveness(Live, SB);
    LLVM_DEBUG(dbgs() << "Successfully sunk " << SB->getHead()->getName()
                      << '\n';
               Live->print(dbgs()); dbgs() << '\n');
    return true;
  }

  return false;
}

/***********************************************************************
 * sinkOnce : do one sinking of a superbale for a group of uses
 *
 * Enter:   InsertBefore = instruction to insert before
 *          SB = superbale to sink
 *          Uses = uses in the group
 *
 * Return:  basic block where sunk superbale was inserted
 *
 * Currently this only copes with the case that the uses are all dominated
 * by InsertBefore, and the moved or cloned def is inserted before InsertBefore
 * and the function returns the basic block containing InsertBefore.
 *
 * However it could be extended to sink for a group of uses that are not
 * dominated by InsertBefore but are reachable from it. Then it would insert
 * the def at a place that is a common dominator of the uses, and return that
 * basic block.
 */
bool GenXDepressurizer::sinkOnce(Instruction *InsertBefore, Superbale *SB,
                                 ArrayRef<Use *> Uses) {
  LLVM_DEBUG(dbgs() << "sinkOnce with uses:";
             for (auto i = Uses.begin(), e = Uses.end(); i != e; ++i) dbgs()
             << " [" << InstNumbers[cast<Instruction>((*i)->getUser())] << ']'
             << (*i)->getUser()->getName();
             dbgs() << '\n');

  for (auto *I : SB->Bales) {
    Bale B;
    Baling->buildBale(I, &B);
    if (!genx::isSafeToSink_CheckAVLoadKill(B, InsertBefore)) {
      LLVM_DEBUG(
          dbgs() << "Will not move this superbale to position of "
                 << *InsertBefore
                 << "since it may result in potential global volatile "
                    "access clbobering\n"
                 << "The bale failed to move is built from this instruction: "
                 << *I << "\n");
      return false;
    }
  }

  // Insert after the current instruction.
  unsigned InsertNum = InstNumbers[InsertBefore];
  IGC_ASSERT(InsertNum != 0);
  LLVM_DEBUG(dbgs() << "InsertBefore: " << InsertBefore->getName() << '\n');
  // Remove this group of uses from the superbale.
  auto Undef = UndefValue::get(SB->getHead()->getType());
  for (auto i = Uses.begin(), e = Uses.end(); i != e; ++i)
    **i = Undef;

  Instruction *Changed = nullptr;

  if (SB->getHead()->use_empty()) {
    // The superbale now has no uses. So we can simply move the instructions.
    for (auto i = SB->Bales.rbegin(), e = SB->Bales.rend(); i != e; ++i) {
      Bale B;
      Baling->buildBale(*i, &B);
      for (auto j = B.begin(), je = B.end(); j != je; ++j) {
        Changed = j->Inst;
        Changed->removeFromParent();
        Changed->insertBefore(InsertBefore);
        InstNumbers[Changed] = InsertNum - 1;
        ++NumSunk;
      }
    }
  } else {
    // The superbale still has uses, so we need to clone it.
    std::map<Instruction *, Instruction *> ClonedInsts;

    for (auto i = SB->Bales.rbegin(), e = SB->Bales.rend(); i != e; ++i) {
      Bale B;
      Baling->buildBale(*i, &B);
      Instruction *InstToClone = nullptr;
      for (auto j = B.begin(), je = B.end(); j != je; ++j) {
        InstToClone = j->Inst;
        Changed = InstToClone->clone();
        Changed->insertBefore(InsertBefore);
        Changed->setName(InstToClone->getName() + ".cloned");
        // Ensure new instruction has the same baling.
        Baling->setBaleInfo(Changed, j->Info);
        for (unsigned k = 0, ke = Changed->getNumOperands(); k != ke; ++k) {
          if (auto O = dyn_cast<Instruction>(Changed->getOperand(k))) {
            auto it = ClonedInsts.find(O);
            if (it != ClonedInsts.end())
              Changed->setOperand(k, it->second);
          }
        }
        ClonedInsts[InstToClone] = Changed;
        InstNumbers[Changed] = InsertNum - 1;
        ++NumCloned;
      }
    }
  }

  // Change our uses to use the moved/cloned superbale.
  for (auto i = Uses.begin(), e = Uses.end(); i != e; ++i)
    **i = Changed;

  if (Changed)
    LLVM_DEBUG(dbgs() << "Sunk/cloned superbale head is " << Changed->getName()
                      << '\n');
  else
    LLVM_DEBUG(dbgs() << "Warning: Changed is nullptr\n");

  return true;
}

/***********************************************************************
 * modifyLiveness : modify liveness (at some point) to reflect the sinking
 *                  of the superbale past it
 *
 * Enter:   Live = the liveness to modify
 *          SB = the superbale
 *
 * Return:  true if the result of the superbale was removed from liveness,
 *          false if it was not live already
 */
bool GenXDepressurizer::modifyLiveness(Liveness *Live, Superbale *SB) {
  // Remove the superbale's result from liveness.
  for (auto i = SB->Bales.begin(), e = SB->Bales.end(); i != e; ++i) {
    Live->removeValue(*i);
  }
  for (auto i = SB->Operands.begin(), e = SB->Operands.end(); i != e; ++i)
    if (*i)
      Live->addValue(*i);
  return true;
}

int GenXDepressurizer::getSuperbaleKillSize(Superbale *SB) {
  int sum = 0;
  for (auto i = SB->Bales.rbegin(), e = SB->Bales.rend(); i != e; ++i) {
    if (GenXIntrinsic::isWrRegion(*i))
      sum += Liveness::getValueSize(
          (*i)->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
    else
      sum += Liveness::getValueSize(*i);
  }
  return sum;
}

/***********************************************************************
 * getSinkBenefit : calculate the benefit of sinking this Superbale
 *
 * Enter:   SB = superbale to consider
 *          Cat = category of value being sunk
 *          Headroom = if flag sinking, the headroom in normal register
 *                     pressure
 *
 * For normal (non-flag) sinking, the benefit is the size of the result
 * minus the total size of the superbale's operands.
 *
 * For flag sinking, the benefit is the size of the flag result minus the
 * total size of any flag operands to the superbale.
 */
int GenXDepressurizer::getSinkBenefit(Superbale *SB, Liveness::Category Cat,
                                      unsigned Headroom) {
  int Benefit = getSuperbaleKillSize(SB);
  unsigned FlagOperandSize = 0, AddrOperandSize = 0, OperandSize = 0;
  for (auto i = SB->Operands.begin(), e = SB->Operands.end(); i != e; ++i) {
    Value *Operand = *i;
    if (!Operand || isa<Constant>(Operand))
      continue;
    if (Live->contains(Operand))
      continue;
    if (Liveness::isFlag(Operand))
      FlagOperandSize += Liveness::getValueSize(Operand);
    else if (Liveness::isAddr(Operand))
      AddrOperandSize += Liveness::getValueSize(Operand);
    else
      OperandSize += Liveness::getValueSize(Operand);
  }
  switch (Cat) {
  case Liveness::FLAG:
    return Benefit - FlagOperandSize; // Flag sinking.
  case Liveness::ADDR:
    return Benefit - AddrOperandSize; // Addr sinking.
  default:
    break;
  }
  return Benefit - OperandSize;
}

/***********************************************************************
 * fillSuperbale : find a chain of instruction to move
 *
 * Return:  false is the chain has side-effect, cannot be moved.
 *
 * For a vector-of-i1 and/or/not instruction, the superbale contains the
 * tree of boolean and/or/not instructions plus the bales for the cmp
 * instructions that created the booleans.
 *
 * For a wrregion, the superbale contains the bale for each wrregion in
 * the chain of wrregion bales with the same inputs.
 *
 * Otherwise, it contains just the present instruction's bale.
 *
 * A bale with an indirect operand also includes the address generating
 * instruction(s) in the superbale, so that, where a superbale is cloned,
 * we maintain the constraint that an address generating instruction has
 * exactly one use between GenXCategory and GenXAddressCommoning.
 */
bool GenXDepressurizer::fillSuperbale(Superbale *SB, Instruction *Inst,
                                      bool IsFlag) {
  // This is a new Superbale. Gather the bale(s) that make the superbale,
  // and record the operands. First get the out-of-bale operands of the bale
  // headed by Inst. We do this in the order such that the "old value of
  // vector" operand of any wrregion heading the bale is the first operand
  // pushed into SB->Operands.
  SB->Number = InstNumbers[Inst];
  SB->Bales.push_back(Inst);
  SmallSet<Value *, 8> OperandSet;
  Bale B;
  Baling->buildBale(Inst, &B);
  bool OnlyRdWrRegion = true;
  for (auto bi = B.rbegin(), be = B.rend(); bi != be; ++bi) {
    BaleInst *BI = &*bi;
    if (BI->Inst->mayHaveSideEffects() || BI->Inst->mayReadOrWriteMemory())
      return false;       // not safe to sink
    if (OnlyRdWrRegion && // Only chk the following conds if still required.
        !GenXIntrinsic::isWrRegion(BI->Inst) &&
        !GenXIntrinsic::isRdRegion(BI->Inst) && !isa<BitCastInst>(BI->Inst) &&
        GenXIntrinsic::getGenXIntrinsicID(BI->Inst) !=
            GenXIntrinsic::genx_add_addr)
      OnlyRdWrRegion = false;
    for (unsigned oi = 0, oe = BI->Inst->getNumOperands(); oi != oe; ++oi) {
      if (BI->Info.isOperandBaled(oi))
        continue;
      Value *Opnd = BI->Inst->getOperand(oi);
      if (!isa<Instruction>(Opnd) && !isa<Argument>(Opnd))
        continue;
      if (OperandSet.insert(Opnd).second)
        SB->Operands.push_back(Opnd);
    }
  }
  if (OnlyRdWrRegion || SB->Operands.empty()) {
    return false; // moving this kind of bale may mess up coalescing
  }
  if (IsFlag) {
    // Boolean operation. For any boolean input, include the instruction that
    // generates it in the bale, as long as this is the only use. A superbale
    // is then potentially a tree of boolean operations combined by and/or/not,
    // and then at each leaf of the tree a cmp or a chain of cmps linked by
    // wrpredregion (i.e. multiple cmps writing to different parts of the same
    // flag register).
    for (unsigned i = 0; i != SB->Operands.size(); ++i) {
      Inst = dyn_cast_or_null<Instruction>(SB->Operands[i]);
      if (!Inst)
        continue;
      if (!Liveness::isFlag(Inst))
        continue;
      if (!Inst->hasOneUse())
        continue;
      if (isa<PHINode>(Inst))
        continue;
      Bale B2;
      Baling->buildBale(Inst, &B2);
      SB->Operands[i] = nullptr;
      SB->Bales.push_back(Inst);
      for (auto bi = B2.rbegin(), be = B2.rend(); bi != be; ++bi) {
        BaleInst *BI = &*bi;
        for (unsigned oi = 0, oe = BI->Inst->getNumOperands(); oi != oe; ++oi) {
          if (BI->Info.isOperandBaled(oi))
            continue;
          Value *Opnd = BI->Inst->getOperand(oi);
          if (OperandSet.insert(Opnd).second)
            SB->Operands.push_back(Opnd);
        }
      }
    }
  } else if (GenXIntrinsic::isWrRegion(Inst)) {
    // Non-boolean operation headed by a wrregion.
    Value *Opnd0 = SB->Operands[0];
    for (;;) {
      if (!GenXIntrinsic::isWrRegion(Opnd0))
        break;
      Inst = cast<Instruction>(Opnd0);
      if (!Inst->hasOneUse())
        break;
      // The "old value of vector" input is another wrregion. Check that all
      // the operands are the same, except the "old value of vector" input
      // to that one.
      Bale B2;
      Baling->buildBale(Inst, &B2);
      Opnd0 = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
      for (auto bi = B2.rbegin(), be = B2.rend(); bi != be; ++bi) {
        BaleInst *BI = &*bi;
        for (unsigned oi = 0, oe = BI->Inst->getNumOperands(); oi != oe; ++oi) {
          if (BI->Info.isOperandBaled(oi))
            continue;
          Value *Opnd = BI->Inst->getOperand(oi);
          if (Opnd == Opnd0)
            continue;
          if (!isa<Instruction>(Opnd) && !isa<Argument>(Opnd))
            continue;
          if (OperandSet.insert(Opnd).second)
            SB->Operands.push_back(Opnd);
        }
      }
      SB->Bales.push_back(Inst);
      // Replace the previous "old value of vector" in SB->Operands.
      SB->Operands[0] = Opnd0; // Opnd0 could be "undef"
    }
  }
  // Now check whether any operand is an address. If so, include the address
  // generating instruction in the superbale, so that, where a superbale is
  // cloned, we maintain the constraint that an address generating instruction
  // has exactly one use between GenXCategory and GenXAddressCommoning.
  for (unsigned oi = 0, oe = SB->Operands.size(); oi != oe; /*EMPTY*/) {
    Value *Opnd = SB->Operands[oi];
    switch (GenXIntrinsic::getGenXIntrinsicID(Opnd)) {
    case GenXIntrinsic::genx_convert_addr:
    case GenXIntrinsic::genx_add_addr: {
      auto Addr = cast<Instruction>(Opnd);
      SB->Bales.push_back(Addr);
      SB->Operands[oi] = Addr->getOperand(0);
      continue;
    }
    default:
      break;
    }
    ++oi;
  }
  return true;
}

/***********************************************************************
 * Superbale::print : debug print
 */
void Superbale::print(raw_ostream &OS) {
  OS << "Superbale[" << Number << ']';
  for (auto i = Bales.begin(), e = Bales.end(); i != e; ++i)
    OS << ' ' << (*i)->getName();
  OS << ", operands:";
  for (auto i = Operands.begin(), e = Operands.end(); i != e; ++i)
    if (*i)
      OS << ' ' << (*i)->getName();
}

/***********************************************************************
 * copyFrom : copy this Liveness from the other one
 */
void Liveness::copyFrom(Liveness *Other) {
  for (auto ci = cat_begin(), ce = cat_end(); ci != ce; ++ci) {
    Values[ci].clear();
    Pressures[ci] = Other->Pressures[ci];
  }
  Pressure = Other->Pressure;
  copyValues(Other);
}

/***********************************************************************
 * getValueSize : get the byte size of a value
 *
 * We round up to an even number of bytes as that's what we need for counting
 * flag pressure, and we may as well do the same for normal pressure.
 */
unsigned Liveness::getValueSize(Value *V) {
  if (isAddr(V))
    switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
    case GenXIntrinsic::genx_add_addr:
      return 0;
    default:
      break;
    }
  return (V->getType()->getPrimitiveSizeInBits() + 15) / 8U & -2U;
}

/***********************************************************************
 * Liveness::addValue : add value to this liveness
 */
void Liveness::addValue(Value *V) {
  auto Cat = GENERAL;
  if (isFlag(V))
    Cat = FLAG;
  else if (isAddr(V))
    Cat = ADDR;
  if (Values[Cat].insert(V).second) {
    Pressure += getValueSize(V);
    Pressures[Cat] += getValueSize(V);
  }
}

/***********************************************************************
 * Liveness::removeValue : remove value from this liveness
 *
 * Return:  true if the value was removed, false if it was not live anyway
 */
bool Liveness::removeValue(Value *V) {
  auto Cat = GENERAL;
  if (isFlag(V))
    Cat = FLAG;
  else if (isAddr(V))
    Cat = ADDR;
  if (!Values[Cat].erase(V))
    return false;
  Pressure -= getValueSize(V);
  Pressures[Cat] -= getValueSize(V);
  return true;
}

/***********************************************************************
 * Liveness::copyValues : copy values from Other into this liveness
 */
void Liveness::copyValues(Liveness *Other) {
  for (auto ci = cat_begin(), ce = cat_end(); ci != ce; ++ci) {
    for (auto i = Other->Values[ci].begin(), e = Other->Values[ci].end();
         i != e; ++i)
      addValue(*i);
  }
}

/***********************************************************************
 * Liveness::print : debug print
 */
void Liveness::print(raw_ostream &OS) {
  OS << "[addrpressure=" << Pressures[ADDR]
     << ",flagpressure=" << Pressures[FLAG] << ",pressure=" << Pressure << ']';
  for (int Cat = NUMCATS; Cat-- > 0; /*EMPTY*/) {
    if (!Values[Cat].empty()) {
      const char *CatName = (Cat == FLAG   ? "flag."
                             : Cat == ADDR ? "addr."
                                           : "");
      OS << ' ' << CatName << "live:";
      for (auto i = begin(Cat), e = end(Cat); i != e; ++i)
        OS << ' ' << (*i)->getName();
    }
  }
}

/***********************************************************************
 * PseudoCFG::Node::removeSucc : remove block from the node's successor
 *      list (if it is in the list at all)
 *
 * This is only used when removing edges to unstick ourselves when there is
 * irreducible flow.
 */
void PseudoCFG::Node::removeSucc(BasicBlock *Succ) {
  for (unsigned i = 0, e = Succs.size(); i != e; ++i) {
    if (Succ == Succs[i]) {
      Succs[i] = Succs[Succs.size() - 1];
      Succs.pop_back();
      break;
    }
  }
}

/***********************************************************************
 * PseudoCFG::Node::removePred : remove block from the node's predecessor
 *      list (if it is in the list at all)
 *
 * The only case when this is possibly called with a Pred that is not on the
 * list is when attempting to remove loop backedges but flow is irreducible.
 * This happens in compute_first_def_bug_5.
 */
void PseudoCFG::Node::removePred(BasicBlock *Pred) {
  for (unsigned i = 0, e = Preds.size(); i != e; ++i) {
    if (Pred == Preds[i]) {
      Preds[i] = Preds[Preds.size() - 1];
      Preds.pop_back();
      break;
    }
  }
}

/***********************************************************************
 * PseudoCFG::compute : compute the pseudo CFG for the function
 */
void PseudoCFG::compute(Function *F, DominatorTree *DT,
                        LoopInfoBase<BasicBlock, Loop> *LI) {
  clear();
  // Initialize the graph to the same as the CFG. While we're scanning the
  // CFG, remember the natural loop backedges.
  SmallVector<BasicBlock *, 4> Backedges;
  for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    auto TI = BB->getTerminator();
    // Remember BB if it is a backedge.
    if (TI->getNumSuccessors() == 1 && DT->dominates(TI->getSuccessor(0), BB))
      Backedges.push_back(BB);
    // Add the edges out of BB.
    auto BBNode = getNode(BB);
    for (unsigned i = 0, e = TI->getNumSuccessors(); i != e; ++i) {
      BasicBlock *Succ = TI->getSuccessor(i);
      BBNode->Succs.push_back(Succ);
      getNode(Succ)->Preds.push_back(BB);
    }
  }
  // For each natural loop backedge, remove the backedge and add the loop
  // exit edges. This is all the changes we need to make if the CFG is
  // reducible. If it is irreducible, we will need to remove more edges
  // when we derive the ordering.
  for (unsigned i = 0, e = Backedges.size(); i != e; ++i) {
    BasicBlock *BB = Backedges[i];
    auto BBNode = getNode(BB);
    IGC_ASSERT_MESSAGE(BBNode->Succs.size() == 1,
                       "expecting backedge to have one successor as we have "
                       "split critical edges");
    BasicBlock *Header = BBNode->Succs[0];
    BBNode->LoopHeader = Header;
    BBNode->Succs.clear(); // This removes Header as BB's only successor.
    getNode(Header)->removePred(BB);
    Loop *L = LI->getLoopFor(Header);
    SmallVector<BasicBlock *, 4> ExitBlocks;
    IGC_ASSERT(L);
    L->getExitBlocks(ExitBlocks);
    for (unsigned j = 0, je = ExitBlocks.size(); j != je; ++j) {
      BasicBlock *Exit = ExitBlocks[j];
      BBNode->Succs.push_back(Exit);
      getNode(Exit)->Preds.push_back(BB);
    }
  }
  // Derive the ordering.
  std::map<BasicBlock *, unsigned> Pending;
  SmallVector<BasicBlock *, 4> Ready;
  std::set<BasicBlock *> Done;
  Ready.push_back(&F->front());
  for (;;) {
    if (Ready.empty()) {
      if (Pending.empty())
        break; // finished
      // We have got stuck. The CFG must be irreducible. Unstick ourselves
      // by choosing the pending block that is earliest in the function,
      // removing any pending edges from it, and making it ready.
      BasicBlock *BB = nullptr;
      for (auto fi = F->begin();; ++fi) {
        BB = &*fi;
        if (Pending.find(BB) != Pending.end())
          break;
      }
      std::set<BasicBlock *> UnseenPreds;
      std::copy_if(
          getNode(BB)->pred_begin(), getNode(BB)->pred_end(),
          std::inserter(UnseenPreds, UnseenPreds.begin()),
          [&Done](BasicBlock *BB) { return Done.find(BB) == Done.end(); });
      for (auto i = UnseenPreds.begin(), e = UnseenPreds.end(); i != e; ++i) {
        getNode(BB)->removePred(*i);
        getNode(*i)->removeSucc(BB);
      }
      Pending.erase(BB);
      Ready.push_back(BB);
      continue;
    }
    // Pop a ready block off the stack.
    auto *BB = Ready.pop_back_val();
    Ordering.push_back(BB);
    Done.insert(BB);
    // For each successor, decrement the pending count. If it becomes 0, the
    // successor becomes ready.
    auto BBNode = getNode(BB);
    for (auto si = BBNode->succ_begin(), se = BBNode->succ_end(); si != se;
         ++si) {
      BasicBlock *Succ = *si;
      auto PendingEntry = &Pending[Succ];
      if (!*PendingEntry) {
        // New entry in the pending map. Count the predecessors.
        for (auto pi = getNode(Succ)->pred_begin(),
                  pe = getNode(Succ)->pred_end();
             pi != pe; ++pi)
          ++*PendingEntry;
      }
      if (--*PendingEntry)
        continue;
      // Successor needs to become ready.
      Pending.erase(Succ);
      Ready.push_back(Succ);
      IGC_ASSERT_MESSAGE(Done.find(Succ) == Done.end(),
                         "Adding already handled node!");
    }
  }
}

/***********************************************************************
 * PseudoCFG::print : print the pseudo-CFG
 */
void PseudoCFG::print(raw_ostream &OS) {
  OS << "PseudoCFG:\n";
  for (auto i = Ordering.begin(), e = Ordering.end(); i != e; ++i) {
    auto BB = *i;
    auto Node = getNode(BB);
    OS << BB->getName();
    if (Node->LoopHeader)
      OS << " loop header " << Node->LoopHeader->getName();
    OS << "\n  preds:";
    for (auto pi = Node->pred_begin(), pe = Node->pred_end(); pi != pe; ++pi)
      OS << ' ' << (*pi)->getName();
    OS << "\n  succs:";
    for (auto si = Node->succ_begin(), se = Node->succ_end(); si != se; ++si)
      OS << ' ' << (*si)->getName();
    OS << '\n';
  }
}
