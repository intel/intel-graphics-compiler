/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXReduceIntSize
/// -----------------
///
/// GenXReduceIntSize is a function pass that reduces the size of vector int
/// values where it can.
///
/// The semantics of the source language usually involve an operator such as +
/// promoting its operands before performing the calculation. Typically, the
/// front end compiler generates IR for the promotion without bothering to work
/// out if it is unnecessary, as it is easier to work out if it is unnecessary
/// in a later LLVM pass.
///
/// For scalar operations, LLVM already contains passes to do this. But it does
/// not seem to for vectors, possibly because OpenCL does not have C-like
/// promotion rules for vectors. CM does have C-like promotion rules for vectors,
/// so we need to cope with unnecessarily promoted operations.
///
/// Operation of the pass
/// ^^^^^^^^^^^^^^^^^^^^^
///
/// First it does a backwards scan, spotting where an instruction can be
/// converted to a smaller int size because its result is used in other
/// instructions that only use the lower part of the value (trunc, or an "and"
/// with e.g. 0xff). The modified instruction with a smaller int size then
/// needs a trunc inserting for each operand. When the pass reaches the
/// instruction that is the input to that new trunc, it may be able to
/// modify that one too. Thus a reduced int size gets propagated backwards.
///
/// Then it does a forwards scan, spotting where an instruction can be converted
/// to a smaller int size because the operands have only the lower part of the
/// value set (zext/sext, or an "and" with e.g. 0xff). The modified instruction with
/// a smaller int size then needs a ZExt/SExt inserting. Thus the reduced int size
/// is propagated forwards.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "GenXUtil.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/PatternMatch.h"
#include <llvm/InitializePasses.h>
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Local.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#define DEBUG_TYPE "GENX_REDUCEINTSIZE"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace genx;

namespace {

// GenXReduceIntSize : reduce integer size
class GenXReduceIntSize : public FunctionPass {
  struct ValueNumBits {
    unsigned NumBits;
    bool IsSignExtended;
    ValueNumBits(unsigned NumBits) : NumBits(NumBits), IsSignExtended(false) {}
    ValueNumBits(unsigned NumBits, bool IsSignExtended)
        : NumBits(NumBits), IsSignExtended(IsSignExtended) {}
  };
  DominatorTree *DT = nullptr;
  bool Modified = false;
public:
  static char ID;
  explicit GenXReduceIntSize() : FunctionPass(ID) { }
  StringRef getPassName() const override { return "GenX reduce integer size"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

private:
  Instruction *reverseProcessInst(Instruction *Inst);
  Value *truncValue(Value *V, unsigned NumBits, Instruction *InsertBefore,
                    const DebugLoc &DL);
  Instruction *forwardProcessInst(Instruction *Inst);
  ValueNumBits getValueNumBits(Value *V, bool PreferSigned = false);
  Value *getSplatValue(ShuffleVectorInst *SVI) const;
};

} // end anonymous namespace

char GenXReduceIntSize::ID = 0;
namespace llvm { void initializeGenXReduceIntSizePass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXReduceIntSize, "GenXReduceIntSize", "GenXReduceIntSize", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXReduceIntSize, "GenXReduceIntSize", "GenXReduceIntSize", false, false)

class ExtOperator : public Operator {
public:
  static bool isExtOpcode(unsigned Opc) {
    return Opc == Instruction::SExt || Opc == Instruction::ZExt;
  }
  static inline bool classof(const Instruction *I) {
    return isExtOpcode(I->getOpcode());
  }
  static inline bool classof(const ConstantExpr *CE) {
    return isExtOpcode(CE->getOpcode());
  }
  static inline bool classof(const Value *V) {
    return (isa<Instruction>(V) && classof(cast<Instruction>(V))) ||
           (isa<ConstantExpr>(V) && classof(cast<ConstantExpr>(V)));
  }
};

FunctionPass *llvm::createGenXReduceIntSizePass()
{
  initializeGenXReduceIntSizePass(*PassRegistry::getPassRegistry());
  return new GenXReduceIntSize();
}

void GenXReduceIntSize::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * GenXReduceIntSize::runOnFunction : process one function to
 *    reduce integer size where possible
 */
bool GenXReduceIntSize::runOnFunction(Function &F)
{
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  // Reverse scan: This does a postordered depth first traversal of the CFG,
  // processing instructions within a basic block in reverse, to ensure that we
  // see a def after its uses (ignoring phi node uses).
  Modified = false;
  for (po_iterator<BasicBlock *> i = po_begin(&F.getEntryBlock()),
      e = po_end(&F.getEntryBlock()); i != e; ++i) {
    BasicBlock *BB = *i;
    // This loop scans the BB in reverse, and allows processReverseInst to
    // erase Inst and other instructions.
    for (auto Inst = &BB->back(); Inst; )
      Inst = reverseProcessInst(Inst);
  }
  // Forward scan: This does a preordered depth first traversal of the CFG to
  // ensure that we see a def before its uses (ignoring phi node uses).
  for (df_iterator<BasicBlock *> i = df_begin(&F.getEntryBlock()),
      e = df_end(&F.getEntryBlock()); i != e; ++i) {
    BasicBlock *BB = *i;
    // This loop scans the BB forward, and allows processForwardInst to erase
    // Inst and other instructions.
    for (auto Inst = &BB->front(); Inst; )
      Inst = forwardProcessInst(Inst);
  }
  return Modified;
}

/***********************************************************************
 * getAndNumBits : get the number of lower bits set by an "and" instruction
 */
static unsigned getAndNumBits(Instruction *Inst)
{
  if (auto C = dyn_cast<Constant>(Inst->getOperand(1))) {
    if ((C = C->getSplatValue())) {
      uint64_t Val = cast<ConstantInt>(C)->getZExtValue();
      return 64 - countLeadingZeros(Val, ZB_Width);
    }
  }
  return Inst->getType()->getScalarType()->getPrimitiveSizeInBits();
}

/***********************************************************************
 * getPrev : get the previous instruction, or 0 if at start of BB
 * getNext : get the next instruction, or 0 if at end of BB
 */
static Instruction *getPrev(Instruction *Inst)
{
  if (&Inst->getParent()->front() == Inst)
    return nullptr;
  return Inst->getPrevNode();
}

static Instruction *getNext(Instruction *Inst)
{
  if (&Inst->getParent()->back() == Inst)
    return nullptr;
  return Inst->getNextNode();
}

/***********************************************************************
 * reverseProcessInst : process one instruction in GenXReduceIntSize's
 *      reverse scan
 *
 * Enter:   Inst = the instruction to process
 *
 * Return:  the previous instruction (after any erases done in here), 0 if
 *          at start of block
 */
Instruction *GenXReduceIntSize::reverseProcessInst(Instruction *Inst)
{
  Instruction *Prev = getPrev(Inst);
  // Ignore if not at least a 4 vector.
  auto VT = dyn_cast<VectorType>(Inst->getType());
  if (!VT)
    return Prev;
  if (!VT->getElementType()->isIntegerTy())
    return Prev;
  unsigned NumBits = VT->getElementType()->getPrimitiveSizeInBits();
  if (NumBits == 1)
      return Prev;
  unsigned TruncBits = 0;
  // See if the value is only used in instructions that use fewer bits (trunc,
  // and, shl).  Get the max truncated size.
  for (auto ui = Inst->use_begin(), ue = Inst->use_end(); ui != ue; ++ui) {
    unsigned ThisTruncBits = NumBits;
    auto user = cast<Instruction>(ui->getUser());
    switch (user->getOpcode()) {
    case Instruction::Trunc:
      ThisTruncBits = user->getType()->getScalarType()->getPrimitiveSizeInBits();
      break;
    case Instruction::And:
      ThisTruncBits = getAndNumBits(user);
      break;
    default:
      ThisTruncBits = NumBits;
      break;
    }
    TruncBits = std::max(TruncBits, ThisTruncBits);
    if (TruncBits == NumBits)
      break;
  }
  if (!TruncBits)
    return Prev; // Inst is unused
  // Round TruncBits up to next power of two no smaller than 8.
  TruncBits = std::max(8, 1 << genx::log2(TruncBits * 2 - 1));
  // If the instruction is not min/max, truncate to no smaller than 16.
  switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
  case GenXIntrinsic::genx_smin:
  case GenXIntrinsic::genx_umin:
  case GenXIntrinsic::genx_smax:
  case GenXIntrinsic::genx_umax:
    break;
  default:
    TruncBits = std::max(TruncBits, 16U);
    break;
  }
  if (TruncBits >= NumBits)
    return Prev; // Inst is used somewhere that cannot truncate.
  LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: can truncate to "
      << TruncBits << " bits: " << *Inst << "\n");
  Value *NewVal = nullptr;
  Instruction *NewInst = nullptr;
  // Put new code _after_ original instruction, so we don't see it again in
  // this backwards pass.
  Instruction *InsertBefore = Inst->getNextNode();
  const DebugLoc &DL = Inst->getDebugLoc();
  switch (Inst->getOpcode()) {
  case Instruction::LShr:
  case Instruction::AShr:
    // An shr by constant needs N more bits, where N is the constant.
    // That might still allow some truncation.
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1))) {
      if ((C = C->getSplatValue())) {
        TruncBits += cast<ConstantInt>(C)->getSExtValue();
        // Round TruncBits up to next power of two no smaller than 8.
        TruncBits = std::max(8, 1 << genx::log2(TruncBits * 2 - 1));
        LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: actually can only truncate right shift to "
            << TruncBits << " bits\n");
        if (TruncBits < NumBits) {
          NewInst = BinaryOperator::Create(
              (Instruction::BinaryOps)Inst->getOpcode(),
              truncValue(Inst->getOperand(0), TruncBits, InsertBefore, DL),
              truncValue(Inst->getOperand(1), TruncBits, InsertBefore, DL),
              "", InsertBefore);
          break;
        }
      }
    }
    // Other shr cannot truncate.
    return Prev;
  case Instruction::And:
    // An "and" by constant might be completely removable if the rhs truncates
    // to all ones.
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1))) {
      if (cast<Constant>(truncValue(C, TruncBits, InsertBefore, DL))
          ->isAllOnesValue()) {
        // Remove the "and".
        NewVal = truncValue(Inst->getOperand(0), TruncBits, InsertBefore, DL);
        break;
      }
    }
    // Otherwise, fall through to treat "and" like the other truncatable
    // binary ops.
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::Shl: {
      // These binary operators can just truncate.
      Value *Fst = truncValue(Inst->getOperand(0), TruncBits, InsertBefore, DL),
            *Snd = truncValue(Inst->getOperand(1), TruncBits, InsertBefore, DL);
      NewInst = BinaryOperator::Create(
          (Instruction::BinaryOps)Inst->getOpcode(), Fst, Snd, "", InsertBefore);
    }
    break;
  case Instruction::ZExt:
  case Instruction::SExt: {
      NewVal = Inst->getOperand(0);
      unsigned NewBits = NewVal->getType()->getScalarType()
          ->getPrimitiveSizeInBits();
      if (TruncBits != NewBits) {
        // The value still needs extending, just not as much as before. Or it
        // might need to be truncated.
        unsigned NumElements =
            cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
        int Opcode = Instruction::Trunc;
        if (TruncBits > NewBits)
          Opcode = Inst->getOpcode();
        auto ElTy = Type::getIntNTy(InsertBefore->getContext(), TruncBits);
        auto Ty = IGCLLVM::FixedVectorType::get(ElTy, NumElements);
        NewInst = CastInst::Create((Instruction::CastOps)Opcode, NewVal,
            Ty, "", InsertBefore);
      }
    }
    break;
  case Instruction::ShuffleVector: {
    auto *Shuffle = cast<ShuffleVectorInst>(Inst);
    if (!Shuffle->isZeroEltSplat())
      return Prev;
    if (cast<IGCLLVM::FixedVectorType>(Shuffle->getOperand(0)->getType())
            ->getNumElements() == 1) {
      // This shufflevector is a splat from a 1-vector.
      auto TruncatedInput = truncValue(Shuffle->getOperand(0), TruncBits,
          InsertBefore, DL);
      NewInst = new ShuffleVectorInst(TruncatedInput,
          UndefValue::get(TruncatedInput->getType()), IGCLLVM::getShuffleMaskForBitcode(Shuffle), "",
          InsertBefore);
      break;
    }
    // Detect when the shufflevector is the second half of an
    // insertelement+shufflevector sequence being used to implement
    // a splat (and the insertelement has no other use). For example:
    //  %splat.splatinsert.i = insertelement <16 x i32> undef, i32 %direction, i32 0, !dbg !355
    //  %splat.splat.i = shufflevector <16 x i32> %splat.splatinsert.i, <16 x i32> undef, <16 x i32> zeroinitializer, !dbg !355
    if (auto IE = dyn_cast<InsertElementInst>(Shuffle->getOperand(0))) {
      if (IE->hasOneUse()) {
        if (auto C = dyn_cast<Constant>(IE->getOperand(2))) {
          if (C->isNullValue()) {
            // This is a splat, and we can truncate it by creating new
            // insertelement and shufflevector instructions.
            unsigned NumElements =
                cast<IGCLLVM::FixedVectorType>(Shuffle->getType())
                    ->getNumElements();
            auto ElTy = Type::getIntNTy(InsertBefore->getContext(),
                  TruncBits);
            auto Ty = IGCLLVM::FixedVectorType::get(ElTy, NumElements);
            auto NewScalar = CastInst::Create(Instruction::Trunc,
                IE->getOperand(1), ElTy,
                IE->getOperand(1)->getName() + ".reduceintsize", InsertBefore);
            NewScalar->setDebugLoc(IE->getDebugLoc());
            auto NewIE = InsertElementInst::Create(UndefValue::get(Ty),
                NewScalar, IE->getOperand(2), "", InsertBefore);
            NewIE->setDebugLoc(IE->getDebugLoc());
            NewIE->takeName(IE);
            NewInst = new ShuffleVectorInst(NewIE, UndefValue::get(Ty),
                IGCLLVM::getShuffleMaskForBitcode(Shuffle), "", InsertBefore);
            break;
          }
        }
      }
    }
    return Prev;
  }
  default:
    return Prev;
  }
  if (NewInst) {
    NewInst->setDebugLoc(DL);
    NewInst->takeName(Inst);
    NewVal = NewInst;
  }
  IGC_ASSERT(NewVal);
  // NewVal is the replacement for Inst with a smaller int size.
  LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: NewVal: " << *NewVal << "\n");
  // Replace the uses of Inst, which we know are all things that
  // have a reduced size requirement (trunc, and).
  while (!Inst->use_empty()) {
    Instruction *user = cast<Instruction>(Inst->use_begin()->getUser());
    unsigned ThisTruncBits =
        user->getType()->getScalarType()->getPrimitiveSizeInBits();
    switch (user->getOpcode()) {
    case Instruction::Trunc: {
        auto ThisNewVal = NewVal;
        if (ThisTruncBits != TruncBits) {
          // We need a new trunc.
          auto NewTI = CastInst::Create(Instruction::Trunc, NewVal, user->getType(),
              "", user);
          NewTI->takeName(user);
          NewTI->setDebugLoc(user->getDebugLoc());
          LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: NewTI: " << *NewTI << "\n");
          ThisNewVal = NewTI;
        }
        user->replaceAllUsesWith(ThisNewVal);
        user->eraseFromParent();
      }
      break;
    case Instruction::And: {
        auto ThisNewVal = NewVal;
        if (ThisTruncBits != TruncBits) {
          // We need a replacement "and" instruction with a different type.
          auto NewAnd = BinaryOperator::Create(Instruction::And, NewVal,
              truncValue(user->getOperand(1), TruncBits,
                user, user->getDebugLoc()),
              "", user);
          NewAnd->takeName(user);
          NewAnd->setDebugLoc(user->getDebugLoc());
          LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: NewAnd: " << *NewAnd << "\n");
          ThisNewVal = NewAnd;

          // Need to trunc or extend our new instruction's result to match
          // the result of the "and".
          IGC_ASSERT(ThisNewVal);
          auto NewCast = CastInst::Create(
              ThisTruncBits > TruncBits ? Instruction::ZExt : Instruction::Trunc,
              ThisNewVal, user->getType(), "", user);
          if (NewVal == ThisNewVal)
            NewCast->takeName(user);
          else
            NewCast->setName(ThisNewVal->getName() + ".cast");
          NewCast->setDebugLoc(user->getDebugLoc());
          LLVM_DEBUG(dbgs() << "GenXReduceIntSize::reverse: NewCast: " << *NewCast << "\n");
          ThisNewVal = NewCast;
        }
        user->replaceAllUsesWith(ThisNewVal);
        user->eraseFromParent();
      }
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "unexpected use");
      break;
    }
  }
  // Erase Inst. Its operands may now become unused, in which case remove
  // those too.
  auto Opnd0Inst = dyn_cast<Instruction>(Inst->getOperand(0));
  Instruction *Opnd1Inst = nullptr;
  if (Inst->getNumOperands() >= 2)
    Opnd1Inst = dyn_cast<Instruction>(Inst->getOperand(1));
  Inst->eraseFromParent();
  if (Opnd0Inst && Opnd0Inst->use_empty()) {
    if (Opnd0Inst == Prev)
      Prev = getPrev(Prev);
    Opnd0Inst->eraseFromParent();
    if (Opnd0Inst == Opnd1Inst)
      Opnd1Inst = nullptr;
  }
  if (Opnd1Inst && Opnd1Inst->use_empty()) {
    if (Opnd1Inst == Prev)
      Prev = getPrev(Prev);
    Opnd1Inst->eraseFromParent();
  }
  Modified = true;
  return Prev;
}

/***********************************************************************
 * truncValue : get truncated version of value
 *
 * Enter:   V = value to truncate (might be constant)
 *          NumBits = integer bit size to truncate to
 *          InsertBefore = insert any new instruction before here
 *          DL = debug loc for any new instruction
 */
Value *GenXReduceIntSize::truncValue(Value *V, unsigned NumBits,
    Instruction *InsertBefore, const DebugLoc &DL)
{
  unsigned NumElements =
      cast<IGCLLVM::FixedVectorType>(V->getType())->getNumElements();
  auto ElTy = Type::getIntNTy(InsertBefore->getContext(), NumBits);
  auto Ty = IGCLLVM::FixedVectorType::get(ElTy, NumElements);
  if (Ty == V->getType())
    return V;
  if (auto C = dyn_cast<Constant>(V)) {
    if (isa<UndefValue>(C))
      return UndefValue::get(Ty);
    if (auto SV = C->getSplatValue()) {
      auto AI = cast<ConstantInt>(SV)->getValue();
      AI = AI.trunc(NumBits);
      C = Constant::getIntegerValue(Ty, AI);
      return C;
    }
    SmallVector<Constant *, 8> Vals;
    if (auto CV = dyn_cast<ConstantVector>(C)) {
      for (unsigned i = 0, e = CV->getNumOperands(); i != e; ++i)
        Vals.push_back(CV->getOperand(i));
      return ConstantVector::get(Vals);
    } else if (auto CDV = dyn_cast<ConstantDataVector>(C)) {
      for (unsigned i = 0, e = CDV->getNumElements(); i != e; ++i)
        Vals.push_back(Constant::getIntegerValue(ElTy,
              APInt(NumBits, CDV->getElementAsInteger(i))));
      return ConstantVector::get(Vals);
    }
  }
  // Not a constant.
  if (auto Inst = dyn_cast<Instruction>(V)) {
    switch (Inst->getOpcode()) {
    case Instruction::Trunc:
    case Instruction::ZExt:
    case Instruction::SExt: {
        // The value is the result of a truncate or extend.
        // See if the input is already the right size.
        Value *Input = Inst->getOperand(0);
        if (Input->getType() == Ty)
          return Input;
        // Instead of truncating the value, truncate or extend the input.
        auto NewInst = CastInst::Create(
            Input->getType()->getScalarType()->getPrimitiveSizeInBits()
              < NumBits ? (Instruction::CastOps)Inst->getOpcode()
              : Instruction::Trunc,
            Input, Ty, Inst->getName() + ".reduceintsize", InsertBefore);
        NewInst->setDebugLoc(DL);
        LLVM_DEBUG(dbgs() << "GenXReduceIntSize::truncVal: " << *NewInst << "\n");
        return NewInst;
      }
    case Instruction::And:
      if (auto C = dyn_cast<Constant>(Inst->getOperand(1))) {
        auto VNB = getValueNumBits(C);
        if (!VNB.IsSignExtended && VNB.NumBits >= NumBits) {
          C = C->getSplatValue();
          if (C) {
            APInt Mask = C->getUniqueInteger();
            if (Mask.isMask(NumBits))
              // The value is the result of an "and" that only keeps bits
              // within the truncated size. Just use its input.
              return
                  truncValue(Inst->getOperand(0), NumBits, InsertBefore, DL);
          }
        }
      }
      break;
    default:
      break;
    }
  }
  // Create a new trunc instruction.
  auto NewInst = CastInst::Create(Instruction::Trunc, V, Ty,
      V->getName() + ".reduceintsize", InsertBefore);
  NewInst->setDebugLoc(DL);
  LLVM_DEBUG(dbgs() << "GenXReduceIntSize::truncVal: " << *NewInst << "\n");
  return NewInst;
}

/***********************************************************************
 * forwardProcessInst : process one instruction in GenXReduceIntSize's
 *      forward scan
 *
 * Enter:   Inst = the instruction to process
 *
 * Return:  the next instruction (after any erases done in here), 0 if
 *          at end of block
 */
Instruction *GenXReduceIntSize::forwardProcessInst(Instruction *Inst) {
  Instruction *Next = getNext(Inst);
  // Ignore if not at least a 4 vector.
  auto VT = dyn_cast<VectorType>(Inst->getType());
  if (!VT) {
    Type *Ty = Inst->getType();
    Value *A;
    const APInt *Val;
    // Transform (add zext(A), Val) to (zext (add zext(A), Val)).
    if (Ty->isIntegerTy(32) &&
        match(Inst, m_Add(m_ZExt(m_Value(A)), m_APInt(Val))))
      if (A->getType()->isIntegerTy(8) && Val->isNonNegative() && Val->isIntN(8)) {
        IRBuilder<> Builder(Inst);
        IntegerType *I16Ty = Builder.getInt16Ty();
        APInt NVal = Val->trunc(16);
        Instruction *NewInst = cast<Instruction>(
            Builder.CreateZExt(
              Builder.CreateAdd(Builder.CreateZExt(A, I16Ty),
                                ConstantInt::get(I16Ty, NVal)), Ty));
        NewInst->takeName(Inst);
        Inst->replaceAllUsesWith(NewInst);
        Inst->eraseFromParent();
        Modified = true;
      }
    return Next;
  }
  if (!VT->getElementType()->isIntegerTy())
    return Next;
  unsigned NumBits = VT->getElementType()->getPrimitiveSizeInBits();
  if (NumBits == 1)
    return Next;
  unsigned TruncBits = NumBits;
  bool NeedSignExtend = false;
  Instruction *InsertBefore = Inst;
  Instruction *NewInst = nullptr;
  Value *NewVal = nullptr;
  const DebugLoc &DL = Inst->getDebugLoc();
  switch (Inst->getOpcode()) {
  case Instruction::LShr:
  case Instruction::AShr:
    // Convert shl+shr pair back into trunc+ext here, because it makes it
    // easier to handle an op that uses the result of it.
    if (auto NewInst = convertShlShr(Inst)) {
      auto Shl = cast<Instruction>(Inst->getOperand(0));
      Inst->eraseFromParent();
      if (Shl->use_empty())
        Shl->eraseFromParent();
      Inst = NewInst;
    }
    break;
  default:
    break;
  }
  auto IID = GenXIntrinsic::not_any_intrinsic;
  switch (Inst->getOpcode()) {
  case Instruction::ShuffleVector:
    if (Value *V = getSplatValue(cast<ShuffleVectorInst>(Inst))) {
      // Transform "splat (ext v)" to "ext (splat v)".
      if (auto Ext = dyn_cast<ExtOperator>(V)) {
        unsigned NumElts =
            cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
        IntegerType *I32Ty = Type::getInt32Ty(Inst->getContext());
        VectorType *MaskTy = IGCLLVM::FixedVectorType::get(I32Ty, NumElts);
        Value *Mask = Constant::getNullValue(MaskTy);
        Value *Src = Ext->getOperand(0);
        if (!isa<VectorType>(Src->getType())) {
          VectorType *VTy =
              IGCLLVM::FixedVectorType::get(Src->getType(), NumElts);
          Src =
              InsertElementInst::Create(UndefValue::get(VTy), Src,
                                        Constant::getNullValue(I32Ty), "",
                                        InsertBefore);
        }
        NewInst =
            new ShuffleVectorInst(Src, UndefValue::get(Src->getType()),
                                  Mask, "", InsertBefore);
        if (Ext->getOpcode() == Instruction::ZExt)
          NewInst = new ZExtInst(NewInst, Inst->getType(), "", InsertBefore);
        else
          NewInst = new SExtInst(NewInst, Inst->getType(), "", InsertBefore);
      }
    }
    break;
  case Instruction::LShr:
    /*fallthrough*/
  case Instruction::AShr:
    goto binop;
  case Instruction::And:
    {
      Value *A;
      const APInt *Val;
      if (match(Inst, m_And(m_Value(A), m_APInt(Val))) &&
          Val->isMask(Val->getActiveBits())) {
        TruncBits = std::max(16, 1 << genx::log2(Val->getActiveBits() * 2 - 1));
        NeedSignExtend = false;
        goto binop;
      }
      // "And" can just truncate, if both operands are truncated, and need the
      // same kind of extension.
      auto VNB0 = getValueNumBits(Inst->getOperand(0));
      auto VNB1 = getValueNumBits(Inst->getOperand(1),
            /*PreferSigned=*/VNB0.IsSignExtended);
      if (VNB0.IsSignExtended == VNB1.IsSignExtended) {
        TruncBits = std::max(VNB0.NumBits, VNB1.NumBits);
        NeedSignExtend = VNB0.IsSignExtended;
        // Round TruncBits up to next power of two no smaller than 8.
        TruncBits = std::max(8, 1 << genx::log2(TruncBits * 2 - 1));
        if (TruncBits < NumBits) {
          auto Opnd1 = truncValue(Inst->getOperand(1), TruncBits, InsertBefore, DL);
          if (auto C = dyn_cast<Constant>(Opnd1)) {
            if (C->isAllOnesValue()) {
              // An "and" with constant that is now all ones can be omitted.
              // This bypasses the usual rule that an "and", like most other
              // operators, should not be truncated smaller than 16.
              LLVM_DEBUG(dbgs() << "GenXReduceIntSize::forward: can truncate to " << TruncBits
                  << " bits and remove completely: " << *Inst << "\n");
              NewVal = truncValue(Inst->getOperand(0), TruncBits, InsertBefore, DL);
              break;
            }
          }
        }
      }
    }
    goto binop;
  case Instruction::Or:
  case Instruction::Xor:
    // These binary operators can just truncate, if both operands are
    // truncated, and need the same kind of extension.
    {
      auto VNB0 = getValueNumBits(Inst->getOperand(0));
      auto VNB1 = getValueNumBits(Inst->getOperand(1),
            /*PreferSigned=*/VNB0.IsSignExtended);
      if (VNB0.IsSignExtended == VNB1.IsSignExtended) {
        TruncBits = std::max(VNB0.NumBits, VNB1.NumBits);
        NeedSignExtend = VNB0.IsSignExtended;
      }
    }
    goto binop;
  case Instruction::Sub: {
    Value *A;
    Value *B;
    // Transforms (sub nuw (zext A), (zext B)) to (zext (sub A, B))
    if (match(Inst, m_NUWSub(m_ZExt(m_Value(A)), m_ZExt(m_Value(B))))) {
      unsigned ASize = A->getType()->getScalarSizeInBits();
      unsigned BSize = B->getType()->getScalarSizeInBits();
      TruncBits = std::max(ASize, BSize);
      goto binop;
    }
    break;
  }
  case Instruction::Call:
    IID = GenXIntrinsic::getGenXIntrinsicID(Inst);
    switch (IID) {
    case GenXIntrinsic::genx_umin:
    case GenXIntrinsic::genx_umax:
    case GenXIntrinsic::genx_smin:
    case GenXIntrinsic::genx_smax: {
        // umin/umax/smin/smax can just truncate as long as both operands
        // have the same type of extension. The type of extension (zero
        // or signed) determines whether the truncated op is umin/umax or
        // smin/smax:
        //
        // a = zext i16  1 to i32 = 0x00000001
        // b = zext i16 -1 to i32 = 0x0000FFFF
        // umax(a, b) = b = umax(trunc(a), trunc(b))
        // smax(a, b) = b = umax(trunc(a), trunc(b))
        //
        // c = sext i16  1 to i32 = 0x00000001
        // d = sext i16 -1 to i32 = 0xFFFFFFFF
        // umax(c, d) = d = smax(trunc(c), trunc(d))
        // smax(c, d) = c = smax(trunc(c), trunc(d))
        //
        auto VNB0 = getValueNumBits(Inst->getOperand(0));
        auto VNB1 = getValueNumBits(Inst->getOperand(1),
                                    /*PreferSigned=*/VNB0.IsSignExtended);
        if (VNB0.IsSignExtended == VNB1.IsSignExtended) {
          // Round TruncBits up to next power of two no smaller than 8.
          // For min and max, allow byte operands.
          TruncBits = std::max(VNB0.NumBits, VNB1.NumBits);
          TruncBits = std::max(8, 1 << genx::log2(TruncBits * 2 - 1));

          Type *SrcTy = Inst->getOperand(0)->getType();
          unsigned SrcBits = SrcTy->getScalarSizeInBits();
          // Only update IID when there is truncation in the source.
          if (TruncBits < SrcBits) {
            switch (IID) {
            case GenXIntrinsic::genx_smax:
            case GenXIntrinsic::genx_umax:
              IID = VNB0.IsSignExtended ? GenXIntrinsic::genx_smax
                                        : GenXIntrinsic::genx_umax;
              break;
            case GenXIntrinsic::genx_smin:
            case GenXIntrinsic::genx_umin:
              IID = VNB0.IsSignExtended ? GenXIntrinsic::genx_smin
                                        : GenXIntrinsic::genx_umin;
              break;
            default:
              break;
            }
          }
        }
      }
      goto binop_truncate;
    default:
      break;
    }
    break;

  binop:
    // Round TruncBits up to next power of two no smaller than 16.
    // Truncating to 8 bits often makes worse gen code because of the
    // restrictions on byte operands in gen.
    TruncBits = std::max(16, 1 << genx::log2(TruncBits * 2 - 1));
  binop_truncate:
    if (TruncBits < NumBits) {
      LLVM_DEBUG(dbgs() << "GenXReduceIntSize::forward: can truncate to " << TruncBits
          << " bits: " << *Inst << "\n");
      auto Opnd0 = truncValue(Inst->getOperand(0), TruncBits, InsertBefore, DL);
      auto Opnd1 = truncValue(Inst->getOperand(1), TruncBits, InsertBefore, DL);
      if (isa<BinaryOperator>(Inst)) {
        // Create the replacement instruction: binary operator.
        NewInst = BinaryOperator::Create(
            (Instruction::BinaryOps)Inst->getOpcode(),
            Opnd0, Opnd1, "", InsertBefore);
      } else {
        // Create the replacement instruction: intrinsic.
        // If it is not the case that all uses trunc to TruncBits, then
        // use the original size as the result type.
        Type *ResTy = Opnd0->getType();
        bool IsOneEltVecTy = false;
        if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(ResTy))
          IsOneEltVecTy = VTy->getNumElements() == 1;
        for (auto ui = Inst->use_begin(), ue = Inst->use_end();
            ui != ue; ++ui) {
          auto User = cast<Instruction>(ui->getUser());
          // Trace through 'extractelement' on single-element vector values.
          if (IsOneEltVecTy &&
              User->getOpcode() == Instruction::ExtractElement &&
              User->hasOneUse())
            User = User->user_back();
          switch (User->getOpcode()) {
          case Instruction::Trunc:
            if (User->getType()->getScalarType()
                ->getPrimitiveSizeInBits() == TruncBits) {
              // Use is trunc to TruncBits: allow truncated result type
              // for intrinsic.
              continue;
            }
            break;
          case Instruction::And:
            if (auto C = dyn_cast<Constant>(User->getOperand(1))) {
              auto VNB = getValueNumBits(C);
              if (!VNB.IsSignExtended && VNB.NumBits <= TruncBits) {
                // Use is and with no bits remaining outside bottom
                // TruncBits: allow truncated result type for intrinsic.
                continue;
              }
            }
            break;
          }
          // Other cases: use the original size as the result type.
          ResTy = Inst->getType();
        }
        TruncBits = ResTy->getScalarType()->getPrimitiveSizeInBits();
        Type *Tys[] = { ResTy, Opnd0->getType() };
        Function *Decl = GenXIntrinsic::getGenXDeclaration(
            Inst->getParent()->getParent()->getParent(),
            IID, Tys);
        Value *Args[] = { Opnd0, Opnd1 };
        NewInst = CallInst::Create(Decl, Args, "", InsertBefore);
      }
    }
    break;
  default:
    break;
  }
  if (NewInst) {
    NewInst->takeName(Inst);
    NewInst->setDebugLoc(DL);
    NewVal = NewInst;
  }
  if (!NewVal)
    return Next;
  LLVM_DEBUG(dbgs() << "GenXReduceIntSize::forward: NewVal: " << *NewVal << "\n");
  // Replace uses of Inst. The default is that we zero/sign extend back to the
  // original size. However, if the use is in a trunc or zext/sext, then we can
  // combine.
  Instruction *Extended = nullptr;
  while (!Inst->use_empty()) {
    auto user = cast<Instruction>(Inst->use_begin()->getUser());
    auto ThisNewVal = NewVal;
    switch (user->getOpcode()) {
    case Instruction::ZExt:
      if (NeedSignExtend)
        break;
      goto combine;
    case Instruction::SExt:
      if (!NeedSignExtend)
        break;
      goto combine;
    case Instruction::Trunc:
    combine: {
        unsigned TargetNumBits = user->getType()->getScalarType()
            ->getPrimitiveSizeInBits();
        if (TargetNumBits != TruncBits) {
          auto NewCast = CastInst::Create(
              TargetNumBits > TruncBits
                  ? (NeedSignExtend ? Instruction::SExt : Instruction::ZExt)
                  : Instruction::Trunc,
              NewVal, user->getType(), "", user);
          NewCast->takeName(user);
          NewCast->setDebugLoc(user->getDebugLoc());
          LLVM_DEBUG(dbgs() << "GenXReduceIntSize::forward: NewCast: "
              << *NewCast << "\n");
          ThisNewVal = NewCast;
        }
        user->replaceAllUsesWith(ThisNewVal);
        if (user == Next)
          Next = getNext(Next);
        user->eraseFromParent();
      }
      continue;
    }
    // Default case.
    if (!Extended && NewVal->getType() == Inst->getType())
      Extended = NewInst;
    if (!Extended) {
      Extended = CastInst::Create(
          NeedSignExtend ? Instruction::SExt : Instruction::ZExt, NewVal,
          Inst->getType(), NewVal->getName() + ".reduceintsize_extend",
          Inst->getNextNode());
      Extended->setDebugLoc(Inst->getDebugLoc());
      LLVM_DEBUG(dbgs() << "GenXReduceIntSize::forward: Extended: "
          << *Extended << "\n");
      replaceAllDbgUsesWith(*Inst, *Extended, *Inst, *DT);
    }
    *Inst->use_begin() = Extended;
  }
  // Erase Inst. Its operands may now become unused, in which case remove
  // those too.
  auto Opnd0Inst = dyn_cast<Instruction>(Inst->getOperand(0));
  Instruction *Opnd1Inst = nullptr;
  if (Inst->getNumOperands() >= 2)
    Opnd1Inst = dyn_cast<Instruction>(Inst->getOperand(1));
  Inst->eraseFromParent();
  if (Opnd0Inst && Opnd0Inst->use_empty()) {
    if (Opnd0Inst == Next)
      Next = getPrev(Next);
    Opnd0Inst->eraseFromParent();
  }
  if (Opnd1Inst && Opnd1Inst->use_empty()) {
    if (Opnd1Inst == Next)
      Next = getPrev(Next);
    Opnd1Inst->eraseFromParent();
  }
  Modified = true;
  return Next;
}

/***********************************************************************
 * getValueNumBits : get the number of bits needed for the vector int value
 *
 * Enter:   PreferSigned = return ValueNumBits with IsSignExtended true
 *                         (and NumBits one greater) for a non-negative
 *                         constant
 *
 * This just returns the number of bits in an element of the value, except
 * for these special cases:
 *
 * 1. A splatted constant returns the number of bits required to represent
 *    an element of the constant.
 *
 * 2. A ZExt returns the number of bits in an element of the _input_.
 *
 * 3. A SExt returns the number of bits in an element of the _input_, with the
 *    flag to say it needs sign extending.
 *
 * 4. An "and" with splatted constant returns the number of bits required
 *    to represent an element of that constant.
 *
 * This function returns a ValueNumBits, which contains:
 *   - NumBits, number of bits required
 *   - IsSignExtended, true if the missing bits are derived by sign extending
 *        rather than zero extending
 */
GenXReduceIntSize::ValueNumBits GenXReduceIntSize::getValueNumBits(
      Value *V, bool PreferSigned)
{
  unsigned NumBits = V->getType()->getScalarType()->getPrimitiveSizeInBits();
  if (auto C = dyn_cast<Constant>(V)) {
    if (C->getType()->isVectorTy())
      C = C->getSplatValue();
    if (C) {
      int64_t Val = cast<ConstantInt>(C)->getSExtValue();
      if (Val >= 0)
        return ValueNumBits(64 - countLeadingZeros((uint64_t)Val, ZB_Width)
            + PreferSigned, /*IsSignExtended=*/PreferSigned);
      if (Val == std::numeric_limits<int64_t>::min())
        return ValueNumBits(64, /*isSignExtended=*/false);
      unsigned const BitsWithSignBit =
          64 - countLeadingOnes(static_cast<uint64_t>(Val), ZB_Undefined) + 1;
      return ValueNumBits(BitsWithSignBit, true);
    }
    return NumBits;
  }
  auto Inst = dyn_cast<Instruction>(V);
  if (!Inst)
    return NumBits;
  switch (Inst->getOpcode()) {
  case Instruction::ZExt:
    return static_cast<unsigned>(Inst->getOperand(0)
                                     ->getType()
                                     ->getScalarType()
                                     ->getPrimitiveSizeInBits());
  case Instruction::SExt:
    return ValueNumBits(Inst->getOperand(0)->getType()->getScalarType()
      ->getPrimitiveSizeInBits(), /*IsSignExtended=*/true);
  case Instruction::And:
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1))) {
      ValueNumBits VNB = getValueNumBits(C);
      if (!VNB.IsSignExtended)
        return VNB;
    }
    break;
  }
  return NumBits;
}

Value *GenXReduceIntSize::getSplatValue(ShuffleVectorInst *SVI) const {
  auto ShuffleMask = SVI->getShuffleMask();
  if (std::any_of(ShuffleMask.begin(), ShuffleMask.end(), [](int V) { return V != 0; }))
    return nullptr;

  Value *Src = SVI->getOperand(0);

  if (auto IEI = dyn_cast<InsertElementInst>(Src)) {
    auto C = dyn_cast<Constant>(IEI->getOperand(2));
    if (C && C->isNullValue())
      return IEI->getOperand(1);
  }

  if (cast<IGCLLVM::FixedVectorType>(Src->getType())->getNumElements() == 1)
    return Src;

  return nullptr;
}
