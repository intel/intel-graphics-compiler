/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXDeadVectorRemoval
/// ---------------------
///
/// GenXDeadVectorRemoval is an aggressive dead code removal pass that analyzes
/// individual elements of a vector rather than whole values.
///
/// As a result of this analysis, the pass can then make the two following
/// modifications to the code:
///
/// 1. If all vector elements of an instruction result turn out to be unused,
///    the instruction is removed. In fact, this pass just sets all its uses to
///    undef, relying on the subsequent dead code removal pass to actually
///    remove it.
///
/// 2. If all vector elements of the "old value" input (even a constant) of a
///    wrregion turn out to be unused, then that input is set to undef. This
///    covers further cases over (1) above:
///
///    a. the "old value" input is constant, and we want to turn it into undef
///       to save a useless constant load;
///
///    b. the "old value" input is an instruction that does have elements used
///       elsewhere, and we want to turn it into undef to detach the two webs
///       of defs and uses from each other to reduce register pressure in
///       between.
///
/// Besides this pass removes all write intrinsics (wrregion, wrpredregion)
/// that have undef input value and replaces their uses with the old value.
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "GENX_DEAD_VECTOR_REMOVAL"

#include "GenX.h"
#include "GenXBaling.h"
#include "GenXUtil.h"
#include "vc/GenXOpts/GenXAnalysis.h"

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#include <queue>
#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace genx;
using namespace GenXIntrinsic::GenXRegion;

static cl::opt<unsigned> LimitGenXDeadVectorRemoval("limit-genx-dead-vector-removal", cl::init(UINT_MAX), cl::Hidden,
                                      cl::desc("Limit GenX dead element removal."));

namespace {

// LiveBitsStorage : encapsulate how live bits for a vector value are stored
// For 31/63 elements or fewer, the bitmap is inside the LiveBitsStorage
// object. For 32/64 elements or more, the bitmap is separately allocated.
class LiveBitsStorage {
  uintptr_t V;
public:
  LiveBitsStorage() : V(0) {}
  ~LiveBitsStorage() {
    if (auto P = getExternal())
      delete[] P;
    V = 0;
  }
private:
  // getExternal : get the external pointer, 0 if none
  // Whether we have an external pointer is encoded in the top bit.
  // The pointer itself is shifted down one and stored in the other bits.
  uintptr_t *getExternal() {
    if ((intptr_t)V >= 0)
      return nullptr; // top bit not set, not external
    return (uintptr_t *)(V * 2);
  }
  // setExternal : set the external pointer
  void setExternal(uintptr_t *P) {
    IGC_ASSERT(!getExternal());
    V = (uintptr_t)P >> 1 | (uintptr_t)1U << (sizeof(uintptr_t) * 8 - 1);
  }
public:
  // setNumElements : set the number of elements to be stored in this
  // LiveBitsStorage. Allocate external storage if necessary.
  void setNumElements(unsigned NumElements) {
    if (NumElements >= sizeof(uintptr_t) * 8 - 1) {
      unsigned Size = NumElements + sizeof(uintptr_t) * 8 - 1
            / (sizeof(uintptr_t) * 8);
      setExternal(new uintptr_t[Size]);
      memset(getExternal(), 0, Size * sizeof(uintptr_t));
    }
  }
  // get : get the pointer to the bitmap
  uintptr_t *get() {
    if (auto P = getExternal())
      return P;
    return &V;
  }
};

// LiveBits : encapsulate a pointer to a bitmap of element liveness and its size
class LiveBits {
  uintptr_t *P;
  unsigned NumElements;
public:
  static const unsigned BitsPerWord = sizeof(uintptr_t) * 8;
  LiveBits() : P(nullptr), NumElements(0) {}
  LiveBits(LiveBitsStorage *LBS, unsigned NumElements)
    : P(LBS->get()), NumElements(NumElements) {}
  // getNumElements : get the number of elements in this bitmap
  unsigned getNumElements() const { return NumElements; }
  // get : get a bit value
  bool get(unsigned Idx) const {
    IGC_ASSERT(Idx < NumElements);
    IGC_ASSERT(BitsPerWord);
    return P[Idx / BitsPerWord] >> (Idx % BitsPerWord) & 1;
  }
  // isAllZero : return true if all bits zero
  bool isAllZero() const;
  // set : set a bit value
  // Returns true if value changed
  bool set(unsigned Idx, bool Val = true);
  // copy : copy all bits from another LiveBits
  // Returns true if value changed
  bool copy(LiveBits Src);
  // orBits : or all bits from another LiveBits into this one
  // Returns true if value changed
  bool orBits(LiveBits Src);
  // setRange : set range of bits, returning true if any changed
  bool setRange(unsigned Start, unsigned Len);
  // debug print
  void print(raw_ostream &OS) const;
};

#ifndef NDEBUG
static raw_ostream &operator<<(raw_ostream &OS, const LiveBits &LB) {
  LB.print(OS);
  return OS;
}
#endif

// GenXDeadVectorRemoval : dead vector element removal pass
class GenXDeadVectorRemoval : public FunctionPass {
  std::map<Instruction *, LiveBitsStorage> InstMap;
  std::set<Instruction *> WorkListSet;
  std::queue<Instruction *> WorkList;
  std::set<Instruction *> WrRegionsWithUsedOldInput;
  bool WorkListPhase = false;
  unsigned RemovedCount = 0;

public:
  static char ID;
  explicit GenXDeadVectorRemoval() : FunctionPass(ID) { }
  StringRef getPassName() const override {
    return "GenX dead vector element removal pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

private:
  void clear() {
    InstMap.clear();
    WorkListSet.clear();
    IGC_ASSERT(WorkList.empty());
    WrRegionsWithUsedOldInput.clear();
    WorkListPhase = false;
    RemovedCount = 0;
  }
  bool nullOutInstructions(Function *F);
  void processInst(Instruction *Inst);
  void processRdRegion(Instruction *Inst, LiveBits LB);
  void processWrRegion(Instruction *Inst, LiveBits LB);
  void processBitCast(Instruction *Inst, LiveBits LB);
  void processElementwise(Instruction *Inst, LiveBits LB);
  void markWhollyLive(Value *V);
  void addToWorkList(Instruction *Inst);
  LiveBits createLiveBits(Instruction *Inst);
  LiveBits getLiveBits(Instruction *Inst);
};

} // end anonymous namespace


char GenXDeadVectorRemoval::ID = 0;
namespace llvm { void initializeGenXDeadVectorRemovalPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXDeadVectorRemoval, "GenXDeadVectorRemoval", "GenXDeadVectorRemoval", false, false)
INITIALIZE_PASS_END(GenXDeadVectorRemoval, "GenXDeadVectorRemoval", "GenXDeadVectorRemoval", false, false)

FunctionPass *llvm::createGenXDeadVectorRemovalPass()
{
  initializeGenXDeadVectorRemovalPass(*PassRegistry::getPassRegistry());
  return new GenXDeadVectorRemoval();
}

void GenXDeadVectorRemoval::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.setPreservesCFG();
}

/***********************************************************************
 * isRootInst : check if this is a "root" instruction, one that we want to
 *    keep even if unused
 */
static bool isRootInst(Instruction *Inst) {
  if (isa<ReturnInst>(Inst) || isa<BranchInst>(Inst) ||
      Inst->isTerminator() || Inst->mayHaveSideEffects())
    return true;

  // Even if the whole region is overwritten by a chain of wrregions, wrregions
  // to predefined register must not be optimized as they are extremely
  // specific.
  if (GenXIntrinsic::isWrRegion(Inst) &&
      GenXIntrinsic::isReadPredefReg(
          Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum)))
    return true;

  if (auto CI = dyn_cast<CallInst>(Inst))
    return !CI->onlyReadsMemory();
  return false;
}

/***********************************************************************
 * GenXDeadVectorRemoval::runOnFunction : process one function
 */
bool GenXDeadVectorRemoval::runOnFunction(Function &F)
{
  // First scan all the code to compute the initial live set
  WorkListPhase = false;
  for (po_iterator<BasicBlock *> i = po_begin(&F.getEntryBlock()),
    e = po_end(&F.getEntryBlock()); i != e; ++i) {
    BasicBlock *BB = *i;
    for (Instruction *Inst = BB->getTerminator(); Inst;) {
      if (isRootInst(Inst))
        processInst(Inst);
      else if (WorkListSet.count(Inst)) {
        if (!isa<PHINode>(Inst))
          WorkListSet.erase(Inst);
        processInst(Inst);
      }
      Inst = (Inst == &BB->front()) ? nullptr : Inst->getPrevNode();
    }
  }

  WorkListPhase = true;
  // initialize the worklist
  for (auto Inst : WorkListSet) {
    WorkList.push(Inst);
  }
  // process until the work list is empty.
  LLVM_DEBUG(dbgs() << "GenXDeadVectorRemoval: process work list\n");
  while (!WorkList.empty()) {
    Instruction *Inst = WorkList.front();
    WorkList.pop();
    WorkListSet.erase(Inst);
    processInst(Inst);
  }
  // Null out unused instructions so the subsequent dead code removal pass
  // removes them.
  LLVM_DEBUG(dbgs() << "GenXDeadVectorRemoval: null out instructions\n");
  bool Modified = nullOutInstructions(&F);
  Modified |= simplifyWritesWithUndefInput(F);
  clear();
  return Modified;
}

/***********************************************************************
 * nullOutInstructions : null out unused instructions so the subsequent dead
 * code removal pass removes them
 *
 * For wrregion, there are two special cases:
 * - when no elements in the "new value" input of a wrregion are use,
 *   then bypass the wrregion with the "old value".
 * - when no elements in the "old value" input of a wrregion are used,
 *   then changes the input to undef.
 */
bool GenXDeadVectorRemoval::nullOutInstructions(Function *F) {
  bool Modified = false;
  for (auto fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
    for (auto bi = fi->begin(), be = fi->end(); bi != be; ++bi) {
      Instruction *Inst = &*bi;
      // Ignore "root" instructions.
      if (isRootInst(Inst))
        continue;
      // See if the instruction has no used elements. If so, null out its uses.
      auto LB = getLiveBits(Inst);
      if (LB.isAllZero()) {
        if (++RemovedCount > LimitGenXDeadVectorRemoval)
          return Modified;
        if (LimitGenXDeadVectorRemoval != UINT_MAX)
          dbgs() << "-limit-genx-dead-vector-removal " << RemovedCount << "\n";
        LLVM_DEBUG(if (!Inst->use_empty())
          dbgs() << "nulled out uses of " << *Inst << "\n");
        while (!Inst->use_empty()) {
          Use *U = &*Inst->use_begin();
          *U = UndefValue::get((*U)->getType());
        }
        Modified = true;
      } else if (GenXIntrinsic::isWrRegion(Inst)) {
        if (!Inst->use_empty()) {
          auto *SI = dyn_cast<StoreInst>(Inst->user_back());
          if (SI && genx::isGlobalStore(SI)) {
            IGC_ASSERT_MESSAGE(Inst->hasOneUse(),
              "Wrregion in gstore bale has more than one use");
            continue;
          }
        }
        // Otherwise, for a wrregion, check if it is in the old input used set.
        // If not, then no element of the "old value" input is used by this
        // instruction (even if it has bits set from other uses), and we can
        // undef out the input.
        Use *U = &Inst->getOperandUse(GenXIntrinsic::GenXRegion::OldValueOperandNum);
        if (WrRegionsWithUsedOldInput.find(Inst)
          == WrRegionsWithUsedOldInput.end()) {
          if (!isa<UndefValue>(*U)) {
            if (++RemovedCount > LimitGenXDeadVectorRemoval)
              return Modified;
            if (LimitGenXDeadVectorRemoval != UINT_MAX)
              dbgs() << "-limit-genx-dead-vector-removal " << RemovedCount
                     << "\n";
            *U = UndefValue::get((*U)->getType());
            LLVM_DEBUG(dbgs() << "null out old value input in " << *Inst << "\n");
            Modified = true;
          }
        }
        // when no elements in the "new value" input of a wrregion are use,
        // then bypass the wrregion with the "old value".
        bool bypass = true;
        Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
        if (R.Mask || R.Indirect)
          bypass = false;
        else {
          for (unsigned RowIdx = R.getOffsetInElements(), Row = 0,
                        NumRows = R.NumElements / R.Width;
               Row != NumRows && bypass; RowIdx += R.VStride, ++Row) {
            for (unsigned Idx = RowIdx, Col = 0; Col != R.Width && bypass;
              Idx += R.Stride, ++Col) {
              if (Idx < LB.getNumElements() && LB.get(Idx))
                bypass = false;
            }
          }
        }
        if (bypass) {
          Inst->replaceAllUsesWith(Inst->getOperandUse(GenXIntrinsic::GenXRegion::OldValueOperandNum));
          Modified = true;
        }
      }
    }
  }
  return Modified;
}

/***********************************************************************
 * processInst : process an instruction in the dead element removal pass
 */
void GenXDeadVectorRemoval::processInst(Instruction *Inst)
{
  LLVM_DEBUG(dbgs() << "  " << *Inst << "\n       has bits " << getLiveBits(Inst) << "\n");
  if (isRootInst(Inst)) {
    // This is a "root" instruction. Mark its inputs as wholly live.
    for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi)
      markWhollyLive(Inst->getOperand(oi));
    return;
  }
  // Check for the result of the instruction not being used at all.
  auto LB = getLiveBits(Inst);
  if (!LB.getNumElements())
    return;
  // Handle phi node.
  if (auto Phi = dyn_cast<PHINode>(Inst)) {
    processElementwise(Phi, LB);
    return;
  }
  // Special case for bitcast.
  if (auto BC = dyn_cast<BitCastInst>(Inst)) {
    processBitCast(BC, LB);
    return;
  }
  // Check for element-wise instructions.
  if (isa<BinaryOperator>(Inst) || isa<CastInst>(Inst)
      || isa<SelectInst>(Inst) || isa<CmpInst>(Inst)) {
    processElementwise(Inst, LB);
    return;
  }
  // Check for rdregion and wrregion.
  switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_rdregionf:
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdpredregion:
      processRdRegion(Inst, LB);
      return;
    case GenXIntrinsic::genx_wrregionf:
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrconstregion:
    case GenXIntrinsic::genx_wrpredregion:
      processWrRegion(Inst, LB);
      return;
    default:
      break;
  }
  // For any other instruction, just mark all operands as wholly live.
  for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi)
    markWhollyLive(Inst->getOperand(oi));
}

/***********************************************************************
 * processRdRegion : process a rdregion instruction for element liveness
 */
void GenXDeadVectorRemoval::processRdRegion(Instruction *Inst, LiveBits LB)
{
  auto InInst = dyn_cast<Instruction>(
      Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
  Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
  if (R.Indirect) {
    markWhollyLive(InInst);
    markWhollyLive(Inst->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum));
    return;
  }
  if (!InInst)
    return;
  // Set bits in InLB (InInst's livebits) for live elements read by the
  // rdregion.
  bool Modified = false;
  LiveBits InLB = createLiveBits(InInst);
  for (unsigned RowIdx = R.getOffsetInElements(), Row = 0,
                NumRows = R.NumElements / R.Width;
       Row != NumRows; RowIdx += R.VStride, ++Row)
    for (unsigned Idx = RowIdx, Col = 0; Col != R.Width; Idx += R.Stride, ++Col)
      if (LB.get(Row * R.Width + Col))
        if (Idx < InLB.getNumElements())
          Modified |= InLB.set(Idx);
  if (Modified)
    addToWorkList(InInst);
}

static Constant *undefDeadConstElements(Constant *C, LiveBits LB) {
  if (isa<UndefValue>(C) || isa<ConstantAggregateZero>(C))
    return C;
  if (!C->getType()->isVectorTy()) {
    IGC_ASSERT(LB.getNumElements() == 1);
    return LB.get(0) ? C : UndefValue::get(C->getType());
  }
  SmallVector<Constant *, 8> NewElems;
  for (unsigned i = 0; i < LB.getNumElements(); ++i)
    NewElems.push_back(LB.get(i)
                           ? C->getAggregateElement(i)
                           : UndefValue::get(C->getType()->getScalarType()));
  return ConstantVector::get(NewElems);
}

/***********************************************************************
 * processWrRegion : process a wrregion instruction for element liveness
 */
void GenXDeadVectorRemoval::processWrRegion(Instruction *Inst, LiveBits LB)
{
  Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
  if (R.Mask)
    markWhollyLive(Inst->getOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum));
  auto NewInInst = dyn_cast<Instruction>(
        Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
  if (R.Indirect) {
    markWhollyLive(NewInInst);
    markWhollyLive(Inst->getOperand(GenXIntrinsic::GenXRegion::WrIndexOperandNum));
  } else if (NewInInst) {
    // Set bits in NewInLB (NewInInst's livebits) for live elements read by
    // the wrregion in the "new value" input.
    bool Modified = false;
    LiveBits NewInLB = createLiveBits(NewInInst);
    for (unsigned RowIdx = R.getOffsetInElements(), Row = 0,
                  NumRows = R.NumElements / R.Width;
         Row != NumRows; RowIdx += R.VStride, ++Row)
      for (unsigned Idx = RowIdx, Col = 0; Col != R.Width;
          Idx += R.Stride, ++Col)
        if (Idx < LB.getNumElements() && LB.get(Idx))
          Modified |= NewInLB.set(Row * R.Width + Col);
    if (Modified)
      addToWorkList(NewInInst);
  }
  // For the "old value" input, we want to see if any elements are used even if
  // the input is a constant, since we want to be able to turn it into undef
  // later on if it is not used. In the non-instruction case, OldInLB is left
  // in a state where it contains no bits and OldInLB.getNumElements() is 0.
  LiveBits OldInLB;
  LiveBitsStorage ConstVecLBS;
  auto OldInVal = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  if (auto OldInInst = dyn_cast<Instruction>(OldInVal))
    OldInLB = createLiveBits(OldInInst);
  else if (auto OldInConst = dyn_cast<Constant>(OldInVal)) {
    ConstVecLBS.setNumElements(LB.getNumElements());
    OldInLB = LiveBits(&ConstVecLBS, LB.getNumElements());
  }
  bool Modified = false;
  bool UsedOldInput = false;
  if (R.Indirect) {
    if (OldInLB.getNumElements())
      Modified = OldInLB.orBits(LB);
    UsedOldInput = true;
  } else {
    // Set bits in OldLB (OldInInst's livebits) for live elements read by the
    // wrregion in the "old value" input, excluding ones that come from the
    // "new value" input.
    unsigned NextRow = 0, NextCol = 0, NextIdx = R.getOffsetInElements(),
             NextRowIdx = NextIdx, NumRows = R.NumElements / R.Width;
    for (unsigned Idx = 0, End = LB.getNumElements(); Idx != End; ++Idx) {
      if (Idx == NextIdx) {
        // This element comes from the "new value" input, unless the wrregion
        // is predicated in which case it could come from either.
        if (R.Mask && LB.get(Idx)) {
          UsedOldInput = true;
          if (OldInLB.getNumElements())
            Modified |= OldInLB.set(Idx);
        }
        if (++NextCol == R.Width) {
          if (++NextRow == NumRows)
            NextIdx = End;
          else
            NextIdx = NextRowIdx += R.VStride;
          NextCol = 0;
        } else
          NextIdx += R.Stride;
      } else {
        // This element comes from the "old value" input.
        if (LB.get(Idx)) {
          UsedOldInput = true;
          if (OldInLB.getNumElements())
            Modified |= OldInLB.set(Idx);
        }
      }
    }
  }
  if (Modified) {
    if (auto OldInInst = dyn_cast<Instruction>(OldInVal))
      addToWorkList(OldInInst);
    // If some constant values are not in use, set it to undef so ConstantLoader
    // can benefit from it.
    else if (auto OldInConst = dyn_cast<Constant>(OldInVal))
      Inst->setOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum,
                       undefDeadConstElements(OldInConst, OldInLB));
  }
  if (UsedOldInput) {
    // We know that at least one element of the "old value" input is used,
    // so add the wrregion to the used old input set.
    WrRegionsWithUsedOldInput.insert(Inst);
  }
}

/***********************************************************************
 * processBitCast : process a bitcast instruction for element liveness
 */
void GenXDeadVectorRemoval::processBitCast(Instruction *Inst, LiveBits LB)
{
  LiveBits InLB;
  LiveBitsStorage ConstVecLBS;
  auto InVal = Inst->getOperand(0);
  if (auto InInst = dyn_cast<Instruction>(InVal))
    InLB = createLiveBits(InInst);
  else if (isa<Constant>(InVal)) {
    auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(InVal->getType());
    unsigned NumElems = VTy ? VTy->getNumElements() : 1;
    ConstVecLBS.setNumElements(NumElems);
    InLB = LiveBits(&ConstVecLBS, NumElems);
  } else
    return;
  bool Modified = false;
  if (InLB.getNumElements() == LB.getNumElements())
    Modified = InLB.orBits(LB);
  else if (InLB.getNumElements() > LB.getNumElements()) {
    IGC_ASSERT(LB.getNumElements());
    IGC_ASSERT((InLB.getNumElements() % LB.getNumElements()) == 0);
    int Scale = InLB.getNumElements() / LB.getNumElements();
    // Input element is smaller than result element.
    for (unsigned Idx = 0, End = LB.getNumElements(); Idx != End; ++Idx)
      if (LB.get(Idx))
        Modified |= InLB.setRange(Idx * Scale, Scale);
  } else {
    IGC_ASSERT(InLB.getNumElements());
    IGC_ASSERT((LB.getNumElements() % InLB.getNumElements()) == 0);
    int Scale = LB.getNumElements() / InLB.getNumElements();
    // Input element is bigger than result element.
    for (unsigned Idx = 0, End = InLB.getNumElements(); Idx != End; ++Idx) {
      bool IsSet = false;
      for (unsigned Idx2 = 0; Idx2 != Scale; ++Idx2)
        IsSet |= LB.get(Idx*Scale | Idx2);
      if (IsSet)
        Modified |= InLB.set(Idx);
    }
  }
  if (Modified) {
    if (auto InInst = dyn_cast<Instruction>(InVal))
      addToWorkList(InInst);
    else if (auto InConst = dyn_cast<Constant>(InVal))
      Inst->setOperand(0, undefDeadConstElements(InConst, InLB));
  }
}

/***********************************************************************
 * processElementwise : process an element-wise instruction such as add or
 *      a phi node
 */
void GenXDeadVectorRemoval::processElementwise(Instruction *Inst, LiveBits LB)
{
  for (unsigned oi = 0, oe = Inst->getNumOperands(); oi != oe; ++oi) {
    auto OpndInst = dyn_cast<Instruction>(Inst->getOperand(oi));
    if (!OpndInst)
      continue;
    auto OpndLB = createLiveBits(OpndInst);
    if (isa<SelectInst>(Inst) && oi == 0 &&
        !OpndInst->getType()->isVectorTy()) {
      // First operand of select inst can be scalar, ignore it
      markWhollyLive(OpndInst);
      continue;
    }

    if (OpndLB.orBits(LB))
      addToWorkList(OpndInst);
  }
}

/***********************************************************************
 * markWhollyLive : mark a value as wholly live (all elements live)
 */
void GenXDeadVectorRemoval::markWhollyLive(Value *V)
{
  auto Inst = dyn_cast_or_null<Instruction>(V);
  if (!Inst)
    return;
  auto LB = createLiveBits(Inst);
  if (LB.setRange(0, LB.getNumElements()))
    addToWorkList(Inst);
}

/***********************************************************************
 * addToWorkList : add instruction to work list if not already there
 *
 * Enter:   Inst = the instruction
 *
 * This does not actually add to the work list in the initial scan through
 * the whole code.
 */
void GenXDeadVectorRemoval::addToWorkList(Instruction *Inst)
{
  LLVM_DEBUG(dbgs() << "    " << Inst->getName() << " now " << getLiveBits(Inst) << "\n");
  if (WorkListSet.insert(Inst).second && WorkListPhase) {
    LLVM_DEBUG(dbgs() << "    adding " << Inst->getName() << " to work list\n");
    WorkList.push(Inst);
  }
}

/***********************************************************************
 * createLiveBits : create the bitmap of live elements for the given
 *               instruction if it doesn't exist.
 *
 * Return:  LiveBits object, which contains a pointer to the bitmap for
 *          this instruction, and a size which is set to 0 if there is no
 *          bitmap allocated yet for this instruction and Create is false
 */
LiveBits GenXDeadVectorRemoval::createLiveBits(Instruction *Inst)
{
  unsigned NumElements = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType()))
    NumElements = VT->getNumElements();
  decltype(InstMap)::iterator Iter;
  bool WasAnInsertion;
  std::tie(Iter, WasAnInsertion) = InstMap.insert(std::make_pair(Inst, LiveBitsStorage{}));
  LiveBitsStorage *LBS = &Iter->second;
  if (WasAnInsertion) {
    // New entry. Set its number of elements.
    LBS->setNumElements(NumElements);
  }
  return LiveBits{LBS, NumElements};
}

/***********************************************************************
 * getLiveBits : get the bitmap of live elements for the given instruction
 *
 * Return:  LiveBits object, which contains a pointer to the bitmap for
 *          this instruction, and a size which is set to 0 if there is no
 *          bitmap allocated yet for this instruction and Create is false
 */
LiveBits GenXDeadVectorRemoval::getLiveBits(Instruction *Inst)
{
  unsigned NumElements = 1;
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType()))
    NumElements = VT->getNumElements();
  auto i = InstMap.find(Inst);
  if (i == InstMap.end())
    return LiveBits();
  LiveBitsStorage *LBS = &i->second;
  return LiveBits(LBS, NumElements);
}

/***********************************************************************
 * LiveBits::isAllZero : return true if all bits zero
 */
bool LiveBits::isAllZero() const
{
  for (unsigned Idx = 0, End = (NumElements + BitsPerWord - 1) / BitsPerWord;
      Idx != End; ++Idx)
    if (P[Idx])
      return false;
  return true;
}

/***********************************************************************
 * LiveBits::set : set (or clear) bit
 *
 * Enter:   Idx = element number
 *          Val = true to set, false to clear, default true
 *
 * Return:  true if the bitmap changed
 */
bool LiveBits::set(unsigned Idx, bool Val)
{
  IGC_ASSERT(Idx < NumElements);
  IGC_ASSERT(BitsPerWord);
  uintptr_t *Ptr = P + Idx / BitsPerWord;
  uintptr_t Bit = 1ULL << (Idx % BitsPerWord);
  uintptr_t Entry = *Ptr;
  if (Val)
    Entry |= Bit;
  else
    Entry &= ~Bit;
  bool Ret = Entry != *Ptr;
  *Ptr = Entry;
  return Ret;
}

/***********************************************************************
 * LiveBits::copy : copy all bits from another LiveBits
 */
bool LiveBits::copy(LiveBits Src)
{
  IGC_ASSERT(NumElements == Src.NumElements);
  IGC_ASSERT(BitsPerWord);
  bool Modified = false;
  for (unsigned Idx = 0, End = (NumElements + BitsPerWord - 1) / BitsPerWord;
      Idx != End; ++Idx) {
    Modified |= P[Idx] != Src.P[Idx];
    P[Idx] = Src.P[Idx];
  }
  return Modified;
}

/***********************************************************************
 * LiveBits::orBits : or all bits from another LiveBits into this one
 */
bool LiveBits::orBits(LiveBits Src)
{
  IGC_ASSERT(NumElements == Src.NumElements);
  bool Modified = false;
  for (unsigned Idx = 0, End = (NumElements + BitsPerWord - 1) / BitsPerWord;
      Idx != End; ++Idx) {
    uintptr_t Word = P[Idx] | Src.P[Idx];
    Modified |= P[Idx] != Word;
    P[Idx] = Word;
  }
  return Modified;
}

/***********************************************************************
 * LiveBits::setRange : set range of bits, returning true if any changed
 */
bool LiveBits::setRange(unsigned Start, unsigned Len)
{
  bool Modified = false;
  unsigned End = Start + Len;
  IGC_ASSERT(End <= NumElements);
  while (Start != End) {
    unsigned ThisLen = BitsPerWord - (Start & (BitsPerWord - 1));
    if (ThisLen > End - Start)
      ThisLen = End - Start;
    uintptr_t *Entry = P + (Start / BitsPerWord);
    uintptr_t Updated = *Entry
          | ((uintptr_t)-1LL >> (BitsPerWord - ThisLen))
              << (Start & (BitsPerWord - 1));
    if (Updated != *Entry) {
      Modified = true;
      *Entry = Updated;
    }
    Start += ThisLen;
  }
  return Modified;
}

/***********************************************************************
 * LiveBits::print : debug print
 */
void LiveBits::print(raw_ostream &OS) const
{
  for (unsigned Idx = 0, End = getNumElements(); Idx != End; ++Idx)
    OS << get(Idx);
}

