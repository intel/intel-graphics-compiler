/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXUnbaling
/// ------------
///
/// After live range building, GenXUnbaling spots cases where baling is harmful
/// due to extending the live range of a big vector.
///
/// The need for the unbaling pass
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// A *two address operation* (mainly wrregion, but also a few intrinsics that
/// do a predicated read from a shared function unit such as memory) is one
/// where the result needs to be in the same register as one operand, the *two
/// address operand*. GenXCoalescing attempts to coalesce the two together, but
/// it fails if the live range of the two address operand extends beyond the
/// two address instruction. Failure of coalescing means that you get extra
/// code inserted before to copy the whole big vector, and increased register
/// pressure because two values of the big vector are live at the same time.
///
/// Similarly, with a phi node incoming, GenXCoalescing attempts to coalesce
/// the incoming with the phi node result. Failure means that you get extra
/// code inserted to copy the value at the end of the incoming block.
///
/// The existence of this problem is due to our use of SSA. Both the input and
/// the output of the wrregion (or the phi incoming and result) are probably
/// the same big vector variable in the source code, and a more traditional
/// compiler would treat the variable as a single (non-SSA) value assigned to
/// its own register, avoiding the need to treat the wrregion specially as a
/// two address operation.
///
/// With the traditional approach, code motion is more difficult, as an
/// instruction cannot be moved past any other instruction that modifies any of
/// the potentially moving instruction's uses.
///
/// With our SSA approach, code motion (of an instruction with no memory
/// interaction) is much easier, and we use that in GenXBaling to bale an
/// instruction into another one without needing to check anything in between.
/// (Even though GenXBaling often does not actually move the baled in
/// instruction in IR, it must be treated as if it is at the position of the
/// head of the bale.)
///
/// The price we pay for that flexibility is that sometimes we move code even
/// when it is harmful to do so.
///
/// The most common situation where it would fail to coalesce is where
/// legalization has created a sequence of wrregions, and the "old value" input
/// to the first one is also used in a rdregion baled in to each one of the
/// wrregions.
///
/// Other situations include where some other rdregion use of the two address
/// operand is user code that has been baled to after the instruction, and
/// where the user code actually takes a copy of the big vector and then uses
/// one or more regions out of it after the two address instruction.
///
/// The GenXUnbaling pass implements two transformations: the non-overlapping
/// region optimization, and the unbaling transformation.
///
/// Non-overlapping region optimization
/// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
///
/// A common cause of the two address operand, the "old value" input, of a
/// wrregion extending beyond the wrregion is that the wrregion is the first in
/// a sequence created by GenXLegalization, and the same vector is used as an
/// input to rdregions baled in to subsequent bales in the sequence.
///
/// In this case, a baled in rdregion is reading a part of the vector that has
/// not been overwritten by any earlier wrregion in the sequence.
///
/// The non-overlapping region optimization detects this case by checking which
/// regions of the vector have been overwritten by earlier wrregions in the
/// sequence. If the region read by the rdregion has not been overwritten, then
/// the optimization can change the input to the rdregion to the result of the
/// previous wrregion in the sequence without changing the semantics.
///
/// If this succeeds for all the rdregions from the same vector in the
/// sequence, then the live range no longer reaches beyond the first wrregion
/// and it can be two address coalesced.
///
/// The non-overlapping region optimization also handles a similar case where
/// the "old value" input to the first wrregion in the sequence is undef, but
/// of the same type as the input to rdregions through the sequence. As well as
/// modifying each rdregion input to the result of the previous wrregion, it
/// changes the undef input to the first wrregion to the same input vector.
/// This also stops the live range of the inputs to the rdregions overlapping
/// the result, and thus saves register pressure. However it can make the code
/// worse if there are further uses of the input after the sequence, so it only
/// makes the transformation if there are no further uses.
///
/// Unbaling transformation
/// ^^^^^^^^^^^^^^^^^^^^^^^
///
/// At its simplest, the unbaling transformation looks at each two address
/// instruction and phi node incoming, and then looks at the uses of the "old
/// value" input:
///
/// * A use before the original two address instruction can be ignored as it
///   does not cause the "old value" input to be live beyond that instruction.
///
/// * A use after the original two address instruction that is not a rdregion
///   cannot be handled, so causes the pass to abandon processing this original
///   two address operation.
///
/// * A rdregion use after the original two address instruction is unbaled so
///   it regains its pre-baling position before the original two address
///   instruction.
///
/// Thus the use of the "old value" input in the two address instruction
/// becomes a kill use, and coalescing at that instruction will succeed. Or the
/// phi incoming becomes a kill use, and coalescing it with the phi result will
/// succeed.
///
/// But there are complications:
///
/// Moving the unbaled instruction
/// """"""""""""""""""""""""""""""
///
/// Unbaling an instruction means that its position in the code is now
/// considered to be the same as its position in the IR. Sometimes that is
/// where we want it (before the original two address instruction), since
/// baling tends not to move code. But sometimes it is still after the original
/// two address instruction, most likely because of the order of code split by
/// GenXLegalization.
///
/// Thus we may need to move the unbaled instruction up to before the original
/// two address instruction. In fact we need to move the whole sub-bale (the
/// new bale headed by the instruction we are unbaling). A rdregion can have an
/// llvm.genx.add.address intrinsic baled in if it has a variable index.
///
/// If the unbaled instruction is dominated by the original two address
/// instruction, we move it to just before that. Otherwise, we move it to the
/// end of the basic block that is the nearest common dominator of the two
/// locations.
///
/// To move a bale up, we need to ensure that all outside-bale operands are
/// defined before where we are going to move it to. If that is not the case,
/// then unbaling for the original two address instruction fails.
///
/// Moving when there is a variable index
/// """""""""""""""""""""""""""""""""""""
///
/// For a rdregion with a variable index, there is an llvm.genx.conv.address
/// intrinsic, which represents the setting of an address register relative to
/// the base register that the rdregion will access. GenXCategory ensures that
/// there is one llvm.genx.conv.address intrinsic for each variable index
/// rdregion or wrregion, since it does not know which region accesses are
/// going to be in the same register. Commoning up of address conversions is
/// done later, after coalescing has decided which ones are in the same base
/// register.
///
/// The problem for GenXUnbaling is that the llvm.genx.conv.address is likely
/// to be just before the rdregion, which stops the rdregion being moved to
/// before the original two address instruction.
///
/// The solution is to pretend that the llvm.genx.conv.address (and anything it
/// bales in, probably a zext/sext) is part of the rdregion's bale, just for
/// GenXUnbaling's purposes of telling whether it is OK to move it, and then
/// actually moving it. GenXBaling::buildBale() has an extra IncludeAddr flag
/// to enable this behavior.
///
/// What is before and after?
/// """""""""""""""""""""""""
///
/// The notion of whether an instruction is before or after the original two
/// address instruction is more complex in the presence of control flow.
///
/// This pass distinguishes the following cases:
///
/// * Before: The instruction dominates the original two address instruction,
///   so can be considered before it. No use in the instruction reaches back to
///   the original two address instruction.
///
/// * After: The original two address instruction dominates the instruction, so
///   can be considered after it. A use in the instruction causes liveness to
///   reach back to the original two address instruction (as long as the use's
///   definition is before that).
///
/// * Reaches: Neither dominates the other, but a use in the instruction causes
///   liveness to reach back to the original two address instruction anyway.
///   This is determined by actually tracing back all the branches through the
///   control graph, abandoning a branch when it rejoins with another one or
///   reaches the definition.
///
/// * Not reaches: Neither dominates the other, but we can prove that a use in
///   the instruction does not cause liveness to reach back to the original two
///   address instruction.
///
/// When processing a phi incoming rather than a two address instruction, it is
/// considered to be at the end of the corresponding incoming block, rather
/// than at the site of the phi node.
///
/// If we have "not reaches", then the use can be ignored in the same way as a
/// "before" use.
///
/// If we have "reaches", then we can still unbale it. If the new sub-bale
/// needs moving, then we move it to the end of the block that is the nearest
/// common dominator of its old location and the original two address
/// instruction.
///
/// A use in a phi node is considered to be at the end of the incoming block
/// for the purposes of determining its position.
///
/// Commoning up unbaled sub-bales
/// """"""""""""""""""""""""""""""
///
/// It is often the case that baling has caused the same rdregion to be cloned
/// (because a baled in instruction can only have a single use), so unbaling
/// the baled in rdregions causes duplicate instructions. No CSE is run after
/// this point, as that would cause various problems including messing up the
/// baling and the address conversion.
///
/// Therefore, this pass needs to spot when it is unbaling duplicate sub-bales
/// and common them up.
///
/// Unbaling the main instruction instead of the rdregion
/// """""""""""""""""""""""""""""""""""""""""""""""""""""
///
/// In some cases, the rdregion is in a bale that also contains another
/// rdregion of the same big vector. Unbaling the two rdregions separately
/// would create two extra instructions. We can reduce that to one extra
/// instruction by instead unbaling the main instruction from the wrregion at
/// the head, so only the wrregion is left at its original position in the code
/// and the rest of the bale is moved up.
///
/// The pass only does that if it detects more than one use of the big vector
/// in the bale.
///
/// When trying to do this, and the proposed sub-bale needs to be moved rather
/// than just unbaled, we may see that not all outside-bale operands are
/// defined before the original two address instruction. In that case, we
/// abandon the attempt to unbale the main instruction, and instead go back to
/// unbaling just the rdregion, which may succeed.
///
/// Bitcasts
/// """"""""
///
/// Because GenXCoalescing does "copy coalescing" of bitcasts first, we need to
/// consider not just the rdregion uses of the input to the original two
/// address instruction, but also uses of the whole tree of bitcasts containing
/// it. Not doing that stops the optimization working when the source CM code
/// contains format() functions.
///
/// Such bitcasts may need to be moved up to just before the original two
/// address instruction, in case any use of it is moved. In fact we just move
/// the whole tree of bitcasts to just after the definition of the root of the
/// tree.  This does not worsen code quality because the bitcasts will all be
/// copy coalesced together anyway.
///
//===----------------------------------------------------------------------===//
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXGotoJoin.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include "GenXNumbering.h"
#include "GenXUtil.h"

#include "vc/Support/BackendConfig.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"

#include <numeric>
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#define DEBUG_TYPE "GENX_UNBALING"

using namespace llvm;
using namespace genx;

namespace {

class GenXUnbaling : public FGPassImplInterface, public IDMixin<GenXUnbaling> {
  enum { UNKNOWN, BEFORE, AFTER, NOTREACHES, REACHES };

  const GenXBackendConfig *BackendConfig = nullptr;
  GenXBaling *Baling = nullptr;
  GenXLiveness *Liveness = nullptr;
  GenXNumbering *Numbering = nullptr;
  DominatorTree *DT = nullptr;
  bool Modified = false;;
  BasicBlock *CurBlock = nullptr;
  std::map<BasicBlock *, int> ReachabilityCache;
  std::set<Instruction *> InstSeen;
  ValueMap<Instruction *, bool> InstSeenInProcessNonOverlappingRegion;
  SmallVector<Instruction *, 4> ToErase;
  // Fields used to process a single two address instruction.
  struct ToUnbaleEntry {
    Instruction *Inst; // instruction to unbale
    Instruction *InsertBefore; // where to move it to, 0 if no move
    ToUnbaleEntry(Instruction *Inst, Instruction *InsertBefore)
        : Inst(Inst), InsertBefore(InsertBefore) {}
  };
  SmallVector<ToUnbaleEntry, 4> ToUnbale;
  std::map<Instruction *, Instruction *> CommonBaleMap;
public:
  explicit GenXUnbaling() {}
  static StringRef getPassName() { return "GenX unbaling"; }
  static void getAnalysisUsage(AnalysisUsage &AU);
  bool runOnFunctionGroup(FunctionGroup &FG) override;

private:
  void processFunc(Function *F);
  void shortenLiveRanges(Function *F);
  bool interfere(Value *V1, Value *V2);
  void processTwoAddrOrPhi(Instruction *Inst, unsigned TwoAddrOperandNum);
  bool scanUsesForUnbaleAndMove(Instruction *Inst, Value *TwoAddrOperand);
  int getReachability(Instruction *Inst, Instruction *Def);
  void processNonOverlappingRegion(CallInst *Wr);
};

} // end anonymous namespace

namespace llvm {
void initializeGenXUnbalingWrapperPass(PassRegistry &);
using GenXUnbalingWrapper = FunctionGroupWrapperPass<GenXUnbaling>;
} // namespace llvm
INITIALIZE_PASS_BEGIN(GenXUnbalingWrapper, "GenXUnbalingWrapper",
                      "GenXUnbalingWrapper", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeGroupWrapperPassWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_DEPENDENCY(GenXGroupBalingWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXLivenessWrapper)
INITIALIZE_PASS_DEPENDENCY(GenXNumberingWrapper)
INITIALIZE_PASS_END(GenXUnbalingWrapper, "GenXUnbalingWrapper",
                    "GenXUnbalingWrapper", false, false)

ModulePass *llvm::createGenXUnbalingWrapperPass() {
  initializeGenXUnbalingWrapperPass(*PassRegistry::getPassRegistry());
  return new GenXUnbalingWrapper();
}

void GenXUnbaling::getAnalysisUsage(AnalysisUsage &AU) {
  AU.addRequired<DominatorTreeGroupWrapperPass>();
  AU.addRequired<GenXBackendConfig>();
  AU.addRequired<GenXGroupBaling>();
  AU.addRequired<GenXLiveness>();
  AU.addRequired<GenXNumbering>();
  AU.addPreserved<DominatorTreeGroupWrapperPass>();
  AU.addPreserved<GenXGroupBaling>();
  AU.addPreserved<GenXLiveness>();
  AU.addPreserved<GenXModule>();
  AU.addPreserved<FunctionGroupAnalysis>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * runOnFunctionGroup : run the liveness analysis for this FunctionGroup
 */
bool GenXUnbaling::runOnFunctionGroup(FunctionGroup &FG) {
  BackendConfig = &getAnalysis<GenXBackendConfig>();
  Baling = &getAnalysis<GenXGroupBaling>();
  Liveness = &getAnalysis<GenXLiveness>();
  Numbering = &getAnalysis<GenXNumbering>();
  Modified = false;
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi)
    processFunc(*fgi);
  return Modified;
}

/***********************************************************************
 * processFunc : process one function in GenXUnbaling
 *
 * This does a postordered depth first traversal of the CFG, processing
 * instructions within a basic block in reverse, to ensure that we see a def
 * after its uses (ignoring phi node uses).  That is required for the
 * non-overlapping region optimization, as we need to perform that on a bale
 * before an earlier wrregion sees the use in the rdregion and unbales it.
 */
void GenXUnbaling::processFunc(Function *F) {
  LLVM_DEBUG(dbgs() << "GenXUnbaling on " << F->getName() << "\n");
  DT = getAnalysis<DominatorTreeGroupWrapperPass>().getDomTree(F);
  for (po_iterator<BasicBlock *> i = po_begin(&F->getEntryBlock()),
      e = po_end(&F->getEntryBlock()); i != e; ++i) {
    CurBlock = *i;
    // Process our incomings of successors' phi nodes.
    auto TI = CurBlock->getTerminator();
    for (unsigned si = 0, se = TI->getNumSuccessors(); si != se; ++si) {
      BasicBlock *Succ = TI->getSuccessor(si);
      for (auto bi = Succ->begin(); ; ++bi) {
        auto Phi = dyn_cast<PHINode>(bi);
        if (!Phi)
          break;
        unsigned IncomingNum = Phi->getBasicBlockIndex(CurBlock);
        processTwoAddrOrPhi(Phi, IncomingNum);
      }
    }
    for (auto Inst = &CurBlock->back(); Inst;
        Inst = Inst == &CurBlock->front() ? nullptr : Inst->getPrevNode()) {
      // Process a two address instruction. (All two address instructions are
      // intrinsics and thus calls.)
      if (auto CI = dyn_cast<CallInst>(Inst)) {
        if (auto TwoAddrOperandNum = getTwoAddressOperandNum(CI)) {
          processTwoAddrOrPhi(CI, *TwoAddrOperandNum);
          if (GenXIntrinsic::isWrRegion(CI))
            processNonOverlappingRegion(CI);
        }
      }
      // Mark the instruction as seen.
      InstSeen.insert(Inst);
    }
    InstSeen.clear();
    InstSeenInProcessNonOverlappingRegion.clear();
    ReachabilityCache.clear();
    for (auto i = ToErase.begin(), e = ToErase.end(); i != e; ++i)
      (*i)->eraseFromParent();
    ToErase.clear();
  }

  shortenLiveRanges(F);
}

/* Checks whether Inst can be placed before InsertBefore without invalidating
 * dominance relations in IR.
 * InsertBefore must come before Inst in IR */
bool canBeSafelyHoisted(Instruction *Inst, Instruction *InsertBefore) {
  if (Inst->getParent() != InsertBefore->getParent())
    return false;
#if LLVM_VERSION_MAJOR <= 10
  // There is no simple way to check order of instructions before llvm 11. Thus
  // handling only cases, where U is a Constant/Declaration/etc
  auto IsDefinedAtInsertPoint = [](Value *V) { return !isa<Instruction>(V); };
#else
  IGC_ASSERT_MESSAGE(InsertBefore->comesBefore(Inst),
                     "InsertBefore must come before Inst in IR");
  auto IsDefinedAtInsertPoint = [InsertBefore](Value *V) {
    return !isa<Instruction>(V) ||
           cast<Instruction>(V)->comesBefore(InsertBefore);
  };
#endif
  return std::all_of(Inst->value_op_begin(), Inst->value_op_end(),
                     IsDefinedAtInsertPoint);
}

/***********************************************************************
 * shortenLiveRanges : hoist rdregions if this helps to avoid copy coalescing.
 *
 * %1 = wrregion ...
 * ...
 * %2 = wrregion(%1, ...)
 * ...
 * %3 = rdregion (%1, ...)
 * no other uses of %1 except rdregions
 *
 * In this situation, compiler will do copy coalescing(See GenXCoalescing) %2
 * from %1. If %1 is a big region, we will have a lot of movs. But if %3 reads
 * a small region, it's cheaper to hoist it between %1 and %2. Compiler will
 * generate a copy for this small region, but %2 will be coalesced without
 * copying.
 */
void GenXUnbaling::shortenLiveRanges(Function *F) {
  for (po_iterator<BasicBlock *> i = po_begin(&F->getEntryBlock()),
                                 e = po_end(&F->getEntryBlock());
       i != e; ++i) {
    BasicBlock *BB = *i;
    for (Instruction &Inst : *BB) {
      auto DstRegion = dyn_cast<CallInst>(&Inst);
      if (DstRegion && GenXIntrinsic::isWrRegion(DstRegion)) {
        // now we've found %2 = wrregion. Firstly, let's check that %1 and %2
        // interfere and after search for rdregions(%3 and others).
        auto SrcRegion = dyn_cast<CallInst>(DstRegion->getOperand(0));
        if (!SrcRegion || !GenXIntrinsic::isWrRegion(SrcRegion) ||
            !interfere(SrcRegion, DstRegion))
          continue;

        // Collect all %1 users that are "under" %2.
        unsigned DstNumber = Numbering->getNumber(DstRegion);
        SmallVector<User *, 4> ToHoist;
        std::copy_if(SrcRegion->user_begin(), SrcRegion->user_end(),
                     std::back_inserter(ToHoist),
                     [DstNumber, N = Numbering](User *U) {
                       return DstNumber < N->getNumber(U);
                     });
        bool CanHoist = std::all_of(
            ToHoist.begin(), ToHoist.end(), [BB, DstRegion](User *U) {
              return U->isUsedInBasicBlock(BB) &&
                     GenXIntrinsic::isRdRegion(U) &&
                     canBeSafelyHoisted(cast<Instruction>(U), DstRegion);
            });
        if (!CanHoist || ToHoist.empty())
          continue;

        // Is it reasonable to hoist rdregions? Let's compare the number of
        // elements to copy in both cases.
        unsigned NumEltsToCopy = std::accumulate(
            ToHoist.begin(), ToHoist.end(), 0u, [](unsigned Init, User *U) {
              unsigned NumElts = 1;
              auto *VU = dyn_cast<IGCLLVM::FixedVectorType>(U->getType());
              if (VU)
                NumElts = VU->getNumElements();
              return Init + NumElts;
            });
        if (NumEltsToCopy >=
            cast<IGCLLVM::FixedVectorType>(SrcRegion->getType())
                ->getNumElements())
          continue;

        // Unbale and hoist
        for (User *U : ToHoist) {
          auto RdR = dyn_cast<CallInst>(U);
          IGC_ASSERT(RdR && GenXIntrinsic::isRdRegion(RdR));
          Instruction *InsertBefore = DstRegion;
          if (auto UnbaleFrom = Baling->getBaleParent(RdR)) {
            BaleInfo BI = Baling->getBaleInfo(UnbaleFrom);
            BI.clearOperandBaled(RdR->use_begin()->getOperandNo());
            Baling->setBaleInfo(UnbaleFrom, BI);
          }
          RdR->moveBefore(InsertBefore);
          Modified = true;
        }
      }
    }
  }
}

bool GenXUnbaling::interfere(Value *V1, Value *V2) {
  IGC_ASSERT(V1);
  IGC_ASSERT(V2);

  LiveRange *V1LR = Liveness->getLiveRangeOrNull(V1);
  LiveRange *V2LR = Liveness->getLiveRangeOrNull(V2);
  // We cannot analyze without LR.
  if (!V1LR || !V2LR)
    return false;
  return Liveness->twoAddrInterfere(V1LR, V2LR);
}

/***********************************************************************
 * processTwoAddrOrPhi : process a two address instruction or phi node
 *    incoming
 *
 * Enter:   Inst = two address inst or phi node
 *          TwoAddrOperandNum = two address operand number (incoming number
 *                              for phi)
 *
 * For a phi node incoming, this is called when CurBlock and InstSeen reflect
 * that processing has reached the end of the incoming's block, rather than the
 * start of the block containing the phi node itself.
 */
void GenXUnbaling::processTwoAddrOrPhi(Instruction *Inst,
    unsigned TwoAddrOperandNum) {
  Value *TwoAddrOperand = Inst->getOperand(TwoAddrOperandNum);
  if (isa<Constant>(TwoAddrOperand))
    return;
  LLVM_DEBUG(dbgs() << "\nGenXUnbaling::processTwoAddrOrPhi[" << TwoAddrOperandNum
               << "]: " << *Inst << "\n");
  if (!scanUsesForUnbaleAndMove(Inst, TwoAddrOperand))
    return;
  // Move the tree of bitcasts containing TwoAddrOperand to just after its def.
  // (If that would be before a phi node, because the def is a phi node other
  // than the last in its block, then insert just before first non-phi in the
  // block. If the def is an Argument, insert at the start of the code.) We may
  // need to move some of them earlier if their uses are going to be moved, and
  // just moving them all as early as possible is easiest.  That does not
  // affect register pressure or code size as a bitcast generates no code and
  // is copy coalesced together.
  //
  // We do not worry about the possibility of moving the bitcasts into a join
  // label block. Although a join label block must start with a join after the
  // phi nodes, bitcasts are allowed as they generate no code.
  Value *Root = TwoAddrOperand;
  while (auto BC = dyn_cast<BitCastInst>(Root))
    Root = BC->getOperand(0);
  Value *V = Root;
  Instruction *InsertBefore = nullptr;
  if (auto I = dyn_cast<Instruction>(Root)) {
    InsertBefore = I->getNextNode();
    if (isa<PHINode>(InsertBefore))
      InsertBefore = InsertBefore->getParent()->getFirstNonPHI();
  } else
    InsertBefore = Inst->getParent()->getParent()->front().getFirstNonPHI();
  SmallVector<Instruction *, 4> BitCastQueue;
  for (unsigned bci = 0;;) {
    // For this value, find uses that are bitcast and save them.
    for (auto ui = V->use_begin(), ue = V->use_end(); ui != ue; ++ui)
      if (auto BC = dyn_cast<BitCastInst>(ui->getUser()))
        BitCastQueue.push_back(BC);
    // Go on to the next bitcast in the queue.
    if (bci == BitCastQueue.size())
      break;
    auto BC = BitCastQueue[bci++];
    // Move this bitcast.
    if (BC == InsertBefore)
      InsertBefore = BC->getNextNode();
    else
      BC->moveBefore(InsertBefore);
    V = BC;
  }
  // Unbale and/or move uses found in scanUsesForUnbaleAndMove().
  for (auto ti = ToUnbale.begin(), te = ToUnbale.end(); ti != te; ++ti) {
    Instruction *Unbale = ti->Inst;
    Instruction *InsertBefore = ti->InsertBefore;
    LLVM_DEBUG(dbgs() << "Unbaling and/or moving " << Unbale->getName()
                 << " (or removing if it is a duplicate)\n");
    // Unbale from its bale parent (if any).
    if (auto UnbaleFrom = Baling->getBaleParent(Unbale)) {
      LLVM_DEBUG(dbgs() << "Unbaling " << Unbale->getName() << " from "
                   << UnbaleFrom->getName() << " in bale "
                   << Baling->getBaleHead(UnbaleFrom)->getName() << "\n");
      BaleInfo BI = Baling->getBaleInfo(UnbaleFrom);
      BI.clearOperandBaled(Unbale->use_begin()->getOperandNo());
      Baling->setBaleInfo(UnbaleFrom, BI);
    }
    auto Found = CommonBaleMap.find(Unbale);
    if (Found != CommonBaleMap.end()) {
      LLVM_DEBUG(dbgs() << "Duplicate of " << Found->second->getName()
                   << ", removing\n");
      Unbale->replaceAllUsesWith(Found->second);
      Bale B;
      Baling->buildBale(Unbale, &B, /*IncludeAddr=*/true);
      Liveness->removeBale(B);
      B.eraseFromParent();
    } else {
      // Move it if necessary.
      if (InsertBefore) {
        LLVM_DEBUG(dbgs() << "Moving bale at " << Unbale->getName()
            << " to before " << InsertBefore->getName()
            << " in " << InsertBefore->getParent()->getName() << "\n");
        Bale B;
        Baling->buildBale(Unbale, &B, /*IncludeAddr=*/true);
        for (auto bi = B.begin(), be = B.end(); bi != be; ++bi) {
          auto MoveInst = bi->Inst;
          LLVM_DEBUG(dbgs() << "  moving " << MoveInst->getName() << "\n");
          MoveInst->moveBefore(InsertBefore);
        }
      }
    }
  }
  Modified = true;
}

/***********************************************************************
 * scanUsesForUnbaleAndMove : scan uses of TwoAddrOperand to see if we can
 *      unbale and/or move them to before the current position
 *
 * Enter:   Inst = instruction at current position
 *          TwoAddrOperand : value whose uses we scan
 *
 * Return:  true if we want to unbale/move some uses
 *
 * This function clears then populates the following GenXUnbaling fields:
 *
 * ToUnbale = vector to store instructions that want to be unbaled and/or moved.
 * CommonBaleMap = map to store mapping for common bales.
 *
 * A duplicate instruction is also in ToUnbale, but after the instruction it
 * duplicates.
 *
 * The function spots the following cases (picking the first that applies):
 *
 * 1. All uses already before Inst. Returns false.
 * 2. There is some use whose liveness reaches back to Inst, but is not
 *    dominated by Inst, so we cannot do anything. Returns false.
 * 3. There is some use in an instruction after Inst which we cannot unbale
 *    and/or move so it is before Inst because it has an outside-bale operand
 *    whose def is not before Inst. Returns false.
 * 4. All uses after Inst can be unbaled and/or moved, but (after commoning
 *    them up) that would result in a number of extra instructions that
 *    outweights the number saved by failing to coalesce Inst. Returns false.
 * 5. There is some use in an instruction after Inst that is not a rdregion
 *    use. We cannot do anything with that. Returns false.
 * 6. Otherwise, return true to tell the caller to go ahead and unbale/move
 *    the instructions in ToUnbale (or common up with another one if it is
 *    in CommonBaleMap).
 *
 * We also need to look at uses of a tree of bitcasts of TwoAddrOperand, as
 * they will be copy coalesced.
 */
bool GenXUnbaling::scanUsesForUnbaleAndMove(Instruction *Inst,
                                            Value *TwoAddrOperand) {
  ToUnbale.clear();
  CommonBaleMap.clear();
  std::set<Instruction *> UseSeen;
  std::set<Bale> CommonBales;
  unsigned UnbaleCount = 0;
  // Scan uses of TwoAddrOperand, and, if any use is a bitcast, scan its uses,
  // and so on through the tree of bitcasts. If TwoAddrOperand is itself the
  // result of a bitcast, scan up to the root of the bitcast tree first.
  SmallVector<Instruction *, 4> BitCasts;
  Value *Root = TwoAddrOperand;
  while (auto BC = dyn_cast<BitCastInst>(Root))
    Root = BC->getOperand(0);
  for (unsigned bci = 0;;) {
    for (auto ui = Root->use_begin(), ue = Root->use_end();
        ui != ue; ++ui) {
      auto User = cast<Instruction>(ui->getUser());
      if (auto Phi = dyn_cast<PHINode>(User)) {
        if (Phi == Inst)
          continue; // Ignore use in phi node that we started at.
        // For a phi node, determine the use's position relative to the current
        // position as if it is at the end of the incoming block.
        int Position = getReachability(
            Phi->getIncomingBlock(*ui)->getTerminator(),
            dyn_cast<Instruction>(TwoAddrOperand));
        LLVM_DEBUG(dbgs() << "phi use in " << User->getName() << " is "
            << (Position == BEFORE ? "before" : (Position == AFTER ? "after"
                : (Position == REACHES ? "reaches" : (Position == NOTREACHES
                    ? "notreaches" : "unknown")))) << "\n");
        if (Position == BEFORE || Position == NOTREACHES)
          continue;
        return false;
      }
      auto UserHead = Baling->getBaleHead(User);
      if (UserHead == Inst)
        continue; // Ignore use in wrregion Inst that we started at.
      LLVM_DEBUG(dbgs() << "use in " << *User << "\n");
      if (!UseSeen.insert(User).second) {
        LLVM_DEBUG(dbgs() << "use in " << User->getName()
                     << " has already been accounted for\n");
        continue;
      }
      // Determine the use's position relative to the current position. We use
      // the bale head's position.
      int Position =
          getReachability(UserHead, dyn_cast<Instruction>(TwoAddrOperand));
      LLVM_DEBUG(dbgs() << "use in " << User->getName() << " is "
          << (Position == BEFORE ? "before" : (Position == AFTER ? "after"
              : (Position == REACHES ? "reaches" : (Position == NOTREACHES
                  ? "notreaches" : "unknown")))) << "\n");
      if (Position == NOTREACHES)
        continue; // ignore use unreachable from Inst
      if (isa<BitCastInst>(User)) {
        // This is a bitcast -- add it to BitCasts so we use it as a Root later
        // and scan its uses (even if it is before Inst, as its uses might
        // still be after Inst).
        LLVM_DEBUG(dbgs() << "use in " << User->getName() << " is bitcast\n");
        BitCasts.push_back(User);
        continue;
      }
      if (Position == BEFORE)
        continue; // Ignore use that is already before Inst.
      // Check that the use is operand 0 of rdregion.
      if (ui->getOperandNo() || !GenXIntrinsic::isRdRegion(User)) {
        LLVM_DEBUG(dbgs() << "use in " << User->getName()
                     << " is after but is not rdregion\n");
        return false;
      }
      // If the result of the rdregion is too big (more than 32 elements or
      // more than 2 GRFs), we cannot unbale it. This happens with an rdregion
      // baled in to a raw operand of a shared function intrinsic. Unbaling it
      // would result in an illegally wide instruction.
      if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(User->getType())) {
        if (VT->getNumElements() > 32U
            || VT->getPrimitiveSizeInBits() > 512U) {
          LLVM_DEBUG(dbgs() << User->getName() << " is too wide to unbale\n");
          return false;
        }
      }
      // We have decided that this use needs unbaling and/or moving. Decide how
      // we are going to do it, without actually doing it yet.  First assume
      // that we're going to unbale User from its bale parent, if it is baled
      // at all.
      Instruction *Unbale = User;
      Bale B;
      if (GenXIntrinsic::isWrRegion(UserHead)) {
        // The bale head is a wrregion. Unbale the main instruction from it,
        // rather than just the user of the overlapping vector, as long as the
        // resulting smaller bale contains at least two uses of TwoAddrOperand
        // (or a bitcast thereof), and each outside-bale operand in the bale is
        // defined before Inst.
        Unbale = dyn_cast<Instruction>(
            UserHead->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
        if (Unbale) {
          // We use IncludeAddr=true on the buildBale. That makes it include
          // any address calculation (convert.addr and add.addr ops), even
          // though they are not baled in. What that gives us is:
          //
          // 1. When comparing bales in the CommonUses set to find another bale
          //    that we can common up with, it makes two rdregions look the
          //    same even though they have separate copies of their address
          //    calculation.
          //
          // 2. The code here that checks if all the outside-bale operands are
          //    defined early enough and then moves the bale also moves the
          //    address calculation, which is what we want.
          Baling->buildBale(Unbale, &B, /*IncludeAddr=*/true);
          B.hash();
          LLVM_DEBUG(B.print(dbgs()));
          // Check for multiple uses. (A use is always in operand 0 of
          // rdregion.)
          unsigned UseCount = 0;
          for (auto bi = B.begin(), be = B.end(); bi != be; ++bi) {
            if (bi->Info.Type != BaleInfo::RDREGION)
              continue;
            Value *Opnd = bi->Inst->getOperand(0);
            if (Opnd == Root)
              ++UseCount;
            else
              for (auto ri = BitCasts.begin(),
                        re = BitCasts.end(); ri != re; ++ri)
                if (bi->Inst->getOperand(0) == *ri)
                  ++UseCount;
          }
          IGC_ASSERT(UseCount >= 1);
          if (UseCount <= 1) {
            // Did not get multiple uses. Just unbale the rdregion use.
            if (Unbale != User) {
              B.clear();
              Unbale = User;
            }
          } else {
            LLVM_DEBUG(dbgs() << "Trying unbale from wrregion\n");
            if (!UseSeen.insert(Unbale).second) {
              LLVM_DEBUG(dbgs() << "use (unbale from wrregion) in "
                           << User->getName()
                           << " has already been accounted for\n");
              continue;
            }
          }
        }
      }
      if (!Unbale)
        return false;
      // Loop to try unbaling from wrregion first, then try just unbaling the
      // rdregion.
      Instruction *InsertBefore = nullptr; // start assuming not moving sub-bale
      for (;;) {
        // Build the sub-bale we are proposing to unbale (if not already built
        // in the code above). See comment above about using IncludeAddr=true.
        if (B.empty()) {
          Baling->buildBale(Unbale, &B, /*IncludeAddr=*/true);
          B.hash();
          LLVM_DEBUG(B.print(dbgs()));
        }
        // Get the position relative to Inst of the sub-bale we propose to
        // unbale. If it is already BEFORE, then we don't need to check for all
        // outside-bale operands being before Inst.
        int UnbalePos = getReachability(Unbale,
              dyn_cast<Instruction>(TwoAddrOperand));
        LLVM_DEBUG(dbgs() << "proposed unbale " << Unbale->getName() << " is "
            << (Position == BEFORE ? "before" : (Position == AFTER ? "after"
                : (Position == REACHES ? "reaches" : (Position == NOTREACHES
                    ? "notreaches" : "unknown")))) << "\n");
        if (UnbalePos == BEFORE) {
          InsertBefore = nullptr; // no need to move instruction
          break; // ok to unbale here
        }
        // We need to move the unbaled instruction. Work out where we need to
        // move it to.
        if (UnbalePos == AFTER && !isa<PHINode>(Inst))
          InsertBefore = Inst; // insert before original two addr inst
        else {
          // The instruction to be unbaled is not dominated by the original two
          // addr inst, or we were processing a phi incoming rather than a two
          // addr inst. We want to find the nearest common dominator and insert
          // at the end of that block.
          InsertBefore = DT->findNearestCommonDominator(
                CurBlock, Unbale->getParent())->getTerminator();
          // Ensure we have a legal insertion point in the presence of SIMD CF.
          InsertBefore = GotoJoin::getLegalInsertionPoint(InsertBefore, DT);
        }
        // We will need to move the unbaled instruction to before Inst.  Check
        // that each outside-bale operand in the bale is defined before the
        // insert point.
        bool IsBeforeInst = true;
        for (auto bi = B.begin(), be = B.end(); bi != be; ++bi) {
          for (unsigned oi = 0, oe = bi->Inst->getNumOperands();
              oi != oe && IsBeforeInst; ++oi) {
            if (!bi->Info.isOperandBaled(oi)) {
              auto Opnd = bi->Inst->getOperand(oi);
              // Check for Opnd's definition being before the insert point:
              //
              // 1. If it is an Argument rather than an Instruction, it is
              //    before.
              if (auto OpndInst = dyn_cast<Instruction>(Opnd)) {
                // 2. If in same basic block:
                // 2a. If insert point is Inst (the original two addr inst),
                //     use InstSeen to work out if it is before or after.
                // 2b. Otherwise, it is always before because InsertBefore is
                //     at the end of its basic block.
                if (OpndInst->getParent() == InsertBefore->getParent()) {
                  if (InsertBefore == Inst)
                    IsBeforeInst &= OpndInst != Inst
                        && InstSeen.find(OpndInst) == InstSeen.end();
                } else
                  // 3. If in different basic block, check dominance.
                  IsBeforeInst &= DT->dominates(
                      OpndInst->getParent(), InsertBefore->getParent());
              }
              if (!IsBeforeInst) {
                LLVM_DEBUG(dbgs() << "  outside-bale operand " << Opnd->getName()
                    << " is not before Inst\n");
                break;
              }
            }
          }
        }
        if (IsBeforeInst) {
          // OK to unbale and move to InsertBefore.
          break;
        }
        // We have failed, either by Unbale's position being REACHES so we
        // can't move it, or by its position being AFTER so we need to move it
        // but there is an outside-bale operand that is not before Inst.
        if (Unbale != User) {
          // This is the case that we were trying to unbale out of the
          // wrregion. This has now failed, and we re-try unbaling just the
          // rdregion use.
          LLVM_DEBUG(dbgs() << "Failed to unbale out of wrregion; "
                       << "retrying at rdregion\n");
          Unbale = User;
          B.clear();
          continue;
        }
        // We have found an outside-bale operand that is not defined before
        // Inst, presumably an operand to the address calculation of the
        // rdregion.  We have to give up at this point.
        LLVM_DEBUG(dbgs() << "Failed to unbale rdregion; abandon\n");
        return false;
      }
      LLVM_DEBUG(dbgs() << "Can unbale and/or move\n");
      // See if we already have a common bale. If so, point this use at it.
      auto Found = CommonBales.find(B);
      if (Found != CommonBales.end()) {
        LLVM_DEBUG(dbgs() << "Found common bale "
                     << Found->getHead()->Inst->getName() << "\n");
        CommonBaleMap[Unbale] = Found->getHead()->Inst;
      } else {
        CommonBales.insert(B);
        // If there will actually be an unbale, count it.
        UnbaleCount += Baling->isBaled(Unbale);
      }
      // Add this bale to the list of bales to unbale and/or move.
      LLVM_DEBUG(
        if (!InsertBefore)
          dbgs() << "Adding " << Unbale->getName() << " to ToUnbale list\n";
        else
          dbgs() << "Adding " << Unbale->getName() << " (with move to before "
              << InsertBefore->getName() << " in "
              << InsertBefore->getParent()->getName() << ") to Unbale list\n";
      );
      ToUnbale.push_back(ToUnbaleEntry(Unbale, InsertBefore));
    }
    // Also look at uses of bitcasts in the bitcast tree.
    if (bci == BitCasts.size())
      break;
    Root = BitCasts[bci++];
  }
  if (ToUnbale.empty()) {
    LLVM_DEBUG(dbgs() << "Nothing to unbale/move, "
                 << "must already be kill use at Inst\n");
    return false;
  }
  // Calculate how many instructions would be needed for the copy caused by
  // TwoAddrOperand failing to coalesce with Inst, and compare that with the
  // number of extra instructions caused by the unbaling that we propose to do
  // to avoid it.
  unsigned NumBytes = TwoAddrOperand->getType()->getPrimitiveSizeInBits() / 8U;
  unsigned NumCopies = NumBytes / 64U; // one copy per 2 GRFs
  NumBytes -= NumCopies * 64U;
  NumCopies += countPopulation(NumBytes); // extra copy per power of 2
  LLVM_DEBUG(dbgs() << NumCopies << " copy insts, vs "
               << UnbaleCount << " unbales\n");
  if (NumCopies < UnbaleCount) {
    LLVM_DEBUG(dbgs() << "Too many new instructions, code would be worse.\n");
    return false;
  }
  LLVM_DEBUG(dbgs() << "We have uses to unbale/move.\n");
  return true;
}

/***********************************************************************
 * getReachability : determine relationship of Inst with current position
 *
 * Enter:   Inst = instruction to get position of
 *          Def = 0 else instruction that defines use whose liveness we are
 *                interested in
 *
 * Return:  BEFORE: Inst is before current pos (Inst dominates current pos)
 *          AFTER: Inst is after current pos (current pos dominates Inst)
 *          REACHES: no dominance, and liveness of use in Inst reaches back to
 *              current pos without passing through Def
 *          NOTREACHES: no dominance, and liveness of use in Inst does not reach
 *              back to current pos without passing through Def
 *
 * In the case that there is no simple dominance relationship between Inst and
 * the current position, Def is used to stop the backwards scan. For a value
 * defined inside a loop, if you don't supply def then this function will
 * always return REACHES as it will trace backwards round the loop.
 *
 * The current position is represented by CurBlock and which already seen
 * instructions in that block are in InstSeen.
 *
 * We keep a cache of results. This is cleared when the current basic block
 * changes.
 */
int GenXUnbaling::getReachability(Instruction *Inst, Instruction *Def)
{
  auto Block = Inst->getParent();
  // Check simple case of same basic block.
  if (CurBlock == Block)
    return InstSeen.find(Inst) != InstSeen.end() ? AFTER : BEFORE;
  // Check ReachabilityCache.
  auto It = ReachabilityCache.insert(
      std::pair<BasicBlock *, int>(Block, UNKNOWN)).first;
  if (It->second != UNKNOWN)
    return It->second;
  // Check dominance.
  if (DT->dominates(Block, CurBlock))
    return It->second = BEFORE;
  if (DT->dominates(CurBlock, Block))
    return It->second = AFTER;
  // Trace liveness of use in Inst backwards and see if we reach CurBlock.
  BasicBlock *DefBlock = nullptr;
  if (Def)
    DefBlock = Def->getParent();
  SmallVector<BasicBlock *, 4> Stack;
  std::set<BasicBlock *> BlockSeen;
  Stack.push_back(Block);
  while (!Stack.empty()) {
    Block = Stack.back();
    Stack.pop_back();
    if (!BlockSeen.insert(Block).second)
      continue; // already seen, terminate this branch of the scan
    if (Block == CurBlock)
      return It->second = REACHES; // reached current pos
    if (Block == DefBlock)
      continue; // reached def, terminate this branch of the scan
    // Add the predecessors of this block to the stack.
    std::copy(pred_begin(Block), pred_end(Block), std::back_inserter(Stack));
  }
  return It->second = NOTREACHES;
}

/***********************************************************************
 * processNonOverlappingRegion : perform the non-overlapping region optimization
 *
 * Enter:   EndWr = wrregion instruction for possible end of wrregion sequence
 *
 * If EndWr is head of a bale that includes a rdregion, and it is part of a
 * sequence of wrregions whose first "old value" input is the same as the input
 * to the rdregion, then check whether the rdregion's region has been
 * overwritten in the sequence. If not, change the rdregion's input to the same
 * as that of Wr.
 *
 * The idea is that we can avoid overlapping live ranges and hence unbaling.
 *
 * This also handles the case that the "old value" input to the start wrregion
 * is undef, and we want to make the transformation (and change that start
 * wrregion input too) to save a live range overlap in the sequence. However,
 * we only do that if we can prove that it does not make the code worse, which
 * it does if the rdregion input is still live after the sequence.
 */
void GenXUnbaling::processNonOverlappingRegion(CallInst *EndWr)
{
  // Avoid processing a sequence of N wrregions N times, giving O(N^2)
  // complexity -- only process when we see the end of the sequence.
  if (InstSeenInProcessNonOverlappingRegion.find(EndWr)
      != InstSeenInProcessNonOverlappingRegion.end())
    return;
  // Find the sequence of wrregions, each except the last having the next as
  // its only use.
  CallInst *StartWr = EndWr;
  Value *StartWrInput = nullptr;
  bool WrVariableIndex = false;
  for (;;) {
    WrVariableIndex |=!isa<Constant>(
          StartWr->getOperand(GenXIntrinsic::GenXRegion::WrIndexOperandNum));
    StartWrInput =
        StartWr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    if (!GenXIntrinsic::isWrRegion(StartWrInput))
      break;
    if (!StartWrInput->hasOneUse())
      break;
    StartWr = cast<CallInst>(StartWrInput);
    InstSeenInProcessNonOverlappingRegion[StartWr] = true;
  }
  if (StartWr == EndWr)
    return; // no sequence
  if (WrVariableIndex)
    return; // Can't deal with variable index
  Value *RdInput = StartWrInput;
  if (isa<UndefValue>(StartWrInput)) {
    if (BackendConfig->disableNonOverlappingRegionOpt())
      return;
    // In the case that the input to the start wrregion is undef, we need to
    // find a rdregion input that is the same type.
    RdInput = nullptr;
    Bale B;
    Baling->buildBale(StartWr, &B);
    for (auto bi = B.begin(), be = B.end(); bi != be; ++bi) {
      if (bi->Info.Type != BaleInfo::RDREGION)
        continue;
      Value *Input = bi->Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
      if (Input->getType() != StartWrInput->getType())
        continue;
      RdInput = Input;
      if (auto PhiNode = dyn_cast<PHINode>(Input)) {
        // Prefer to save a live-range on Phi with EndWr value wrapped around,
        // which may help to save phi copies. This is observed on Histogram1.
        if (std::any_of(PhiNode->incoming_values().begin(),
                        PhiNode->incoming_values().end(),
                        [EndWr](Value *Val) { return Val == EndWr; }))
          break;
      }
    }
    // TODO: in some cases this method produces incorrect code
    // when predefined regs are involved, so isReadPredefReg(RdInput)
    // is necessary. We need to investigate this further, some bug
    // unrelated to the predef regs may exist here
    if (!RdInput || GenXIntrinsic::isReadPredefReg(RdInput))
      return; // no such input found
    // We need to check that RdInput is not used again after this sequence,
    // otherwise we could be making the code worse. The use of RdInput is
    // counted as being at its user's bale head.
    auto Def = dyn_cast<Instruction>(RdInput);
    for (auto ui = RdInput->use_begin(), ue = RdInput->use_end();
        ui != ue; ++ui) {
      auto User = cast<Instruction>(ui->getUser());
      auto UserHead = Baling->getBaleHead(User);
      switch (getReachability(UserHead, Def)) {
        case AFTER:
        case REACHES:
          return;
      }
    }
  }
  // Scan forwards through the wrregion sequence, keeping track of which
  // elements of the vector keep their original values. Then for each one see
  // if it has a rdregion whose input is the same as the first wrregion's "old
  // value" input. If so, and the region has not been overwritten by wrregions
  // so far, remember it as one that we want to change.  We calculate which
  // regions have been overwritten by starting with a vector of all 0s and then
  // simulating the writes by writing -1s. If the region we want at the end is
  // still all 0s, then it has not been overwritten.
  SmallVector<std::pair<Instruction *, Value *>, 4> RdsToModify;
  Constant *C = Constant::getNullValue(EndWr->getType());
  for (auto ThisWr = StartWr;;) {
    // For elements overwritten by Wr, change corresponding elements in C to
    // undef.
    Region R = makeRegionFromBaleInfo(ThisWr, BaleInfo());
    C = R.evaluateConstantWrRegion(C,
        Constant::getAllOnesValue(ThisWr->getOperand(1)->getType()));
    // Move on to next wrregion.
    if (ThisWr == EndWr)
      break;
    ThisWr = cast<CallInst>(ThisWr->use_begin()->getUser());
    // Scan the rdregions in ThisWr's bale.
    Bale B;
    Baling->buildBale(ThisWr, &B);
    for (auto bi = B.begin(), be = B.end(); bi != be; ++bi) {
      if (bi->Info.Type != BaleInfo::RDREGION)
        continue;
      if (bi->Inst->getOperand(0) != RdInput)
        continue;
      Instruction *Rd = bi->Inst;
      // See if the rdregion only reads a region that has not been overwritten
      // by any wrregion up to now.
      Region RdR = makeRegionFromBaleInfo(Rd, BaleInfo());
      if (RdR.Indirect)
        return; // Fail if rdregion is indirect
      Constant *SubC = RdR.evaluateConstantRdRegion(C, /*AllowScalar=*/false);
      if (!SubC->isNullValue())
        return; // Fail if reads overwritten region
      // Remember this rdregion for modifying.
      RdsToModify.push_back(
          std::pair<Instruction *, Value *>(Rd, ThisWr->getOperand(0)));
    }
  }
  // No failures, so do the modification.
  if (RdsToModify.empty())
    return;
  Modified = true;
  SmallVector<Instruction *, 4> UselessWrRegions;
  for (auto ri = RdsToModify.begin(), re = RdsToModify.end(); ri != re; ++ri) {
    // Change the input to the rdregion.
    auto Rd = ri->first;
    auto RdInput = ri->second;
    Rd->setOperand(0, RdInput);
    // Check for the case that we have a rdregion-wrregion bale that is now
    // uesless because it reads and writes the same region.
    auto Wr = Baling->getBaleParent(Rd);
    if (GenXIntrinsic::isWrRegion(Wr) &&
        (makeRegionFromBaleInfo(Wr, BaleInfo()) ==
         makeRegionFromBaleInfo(Rd, BaleInfo()))) {
      UselessWrRegions.push_back(Wr);
      continue;
    }
    // We already know that the rdregion's position in generated code (as
    // reflected by the order of heads of bales) is after the instruction
    // generating its new input. However, ignoring baling, it might actually be
    // _before_ that instruction in the IR, which causes the verifier pass to
    // complain. We work around that by moving the rdregion (and any other
    // instruction in the bale between it and the head) to just before the head
    // of its bale.
    SmallVector<Instruction *, 4> BaleTrace;
    BaleTrace.push_back(Rd);
    for (;;) {
      auto Parent = Baling->getBaleParent(BaleTrace.back());
      if (!Parent)
        break;
      BaleTrace.push_back(Parent);
    }
    for (unsigned i = 0, e = BaleTrace.size() - 1; i != e; ++i) {
      auto InstToMove = BaleTrace[i];
      InstToMove->moveBefore(BaleTrace.back());
    }
  }
  // For the undef input case, also modify that.
  if (isa<UndefValue>(StartWrInput))
    StartWr->setOperand(0, RdInput);
  // Now remove the useless wrregions found above.
  for (auto i = UselessWrRegions.begin(), e = UselessWrRegions.end();
      i != e; ++i) {
    auto Wr = *i;
    auto Rd = cast<Instruction>(
        Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
    Wr->replaceAllUsesWith(
        Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    Liveness->removeValue(Wr);
    Liveness->removeValue(Rd);
    ToErase.push_back(Wr);
    ToErase.push_back(Rd);
  }
}
