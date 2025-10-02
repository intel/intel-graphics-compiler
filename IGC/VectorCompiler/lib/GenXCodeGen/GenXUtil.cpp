/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Utility functions for the GenX backend.
//
//===----------------------------------------------------------------------===//
#include "GenXUtil.h"
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXBaling.h"
#include "GenXIntrinsics.h"
#include "GenXLiveness.h"
#include <optional>

#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/GenX/InternalMetadata.h"
#include "vc/Utils/GenX/PredefinedVariable.h"
#include "vc/Utils/GenX/Printf.h"
#include "vc/Utils/General/Types.h"

#include "llvmWrapper/IR/Instructions.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Value.h"
#include "llvmWrapper/IR/Function.h"
#include <llvmWrapper/ADT/Optional.h>

#include "Probe/Assertion.h"

#include <cstddef>
#include <iterator>

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "GENX_UTILS"

namespace {
struct InstScanner {
  Instruction *Original;
  Instruction *Current;
  InstScanner(Instruction *Inst) : Original(Inst), Current(Inst) {}
};

} // namespace

/***********************************************************************
 * createConvert : create a genx_convert intrinsic call
 *
 * Enter:   In = value to convert
 *          Name = name to give convert instruction
 *          InsertBefore = instruction to insert before else 0
 *          M = Module (can be 0 as long as InsertBefore is not 0)
 */
CallInst *genx::createConvert(Value *In, const Twine &Name,
                              Instruction *InsertBefore, Module *M) {
  if (!M)
    M = InsertBefore->getModule();
  Function *Decl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_convert, In->getType());
  return CallInst::Create(Decl, In, Name, InsertBefore);
}

/***********************************************************************
 * createConvertAddr : create a genx_convert_addr intrinsic call
 *
 * Enter:   In = value to convert
 *          Offset = constant offset
 *          Name = name to give convert instruction
 *          InsertBefore = instruction to insert before else 0
 *          M = Module (can be 0 as long as InsertBefore is not 0)
 */
CallInst *genx::createConvertAddr(Value *In, int Offset, const Twine &Name,
                                  Instruction *InsertBefore, Module *M) {
  if (!M)
    M = InsertBefore->getModule();
  auto OffsetVal = ConstantInt::get(In->getType()->getScalarType(), Offset);
  Function *Decl = GenXIntrinsic::getGenXDeclaration(
      M, GenXIntrinsic::genx_convert_addr, In->getType());
  Value *Args[] = {In, OffsetVal};
  return CallInst::Create(Decl, Args, Name, InsertBefore);
}

/***********************************************************************
 * createAddAddr : create a genx_add_addr intrinsic call
 *
 * InsertBefore can be 0 so the new instruction is not inserted anywhere,
 * but in that case M must be non-0 and set to the Module.
 */
CallInst *genx::createAddAddr(Value *Lhs, Value *Rhs, const Twine &Name,
                              Instruction *InsertBefore, Module *M) {
  if (!M)
    M = InsertBefore->getModule();
  Value *Args[] = {Lhs, Rhs};
  Type *Tys[] = {Rhs->getType(), Lhs->getType()};
  Function *Decl =
      GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_add_addr, Tys);
  return CallInst::Create(Decl, Args, Name, InsertBefore);
}

/***********************************************************************
 * createUnifiedRet : create a dummy instruction that produces dummy
 * unified return value.
 *
 * %Name.unifiedret = call Ty @llvm.ssa_copy(Ty undef)
 */
CallInst *genx::createUnifiedRet(Type *Ty, const Twine &Name, Module *M) {
  IGC_ASSERT_MESSAGE(Ty, "wrong argument");
  IGC_ASSERT_MESSAGE(M, "wrong argument");
  auto G = Intrinsic::getDeclaration(M, Intrinsic::ssa_copy, Ty);
  return CallInst::Create(G, UndefValue::get(Ty), Name + ".unifiedret",
                          static_cast<Instruction *>(nullptr));
}

/***********************************************************************
 * getPredicateConstantAsInt : get an i1 or vXi1 constant's value as a single
 * integer
 *
 * Elements of constant \p C are encoded as least significant bits of the
 * result. For scalar case only LSB of the result is set to corresponding value.
 */
unsigned genx::getPredicateConstantAsInt(const Constant *C) {
  IGC_ASSERT_MESSAGE(
      C->getType()->isIntOrIntVectorTy(1),
      "wrong argument: constant of i1 or Nxi1 type was expected");
  if (auto CI = dyn_cast<ConstantInt>(C))
    return CI->getZExtValue(); // scalar
  unsigned Bits = 0;
  unsigned NumElements =
      cast<IGCLLVM::FixedVectorType>(C->getType())->getNumElements();
  IGC_ASSERT_MESSAGE(NumElements <= sizeof(Bits) * CHAR_BIT,
                     "vector has too much elements, it won't fit into Bits");
  for (unsigned i = 0; i != NumElements; ++i) {
    auto El = C->getAggregateElement(i);
    if (!isa<UndefValue>(El))
      Bits |= (cast<ConstantInt>(El)->getZExtValue() & 1) << i;
  }
  return Bits;
}

/***********************************************************************
 * getConstantSubvector : get a contiguous region from a vector constant
 */
Constant *genx::getConstantSubvector(const Constant *V, unsigned StartIdx,
                                     unsigned Size) {
  Type *ElTy = cast<VectorType>(V->getType())->getElementType();
  Type *RegionTy = IGCLLVM::FixedVectorType::get(ElTy, Size);
  Constant *SubVec = nullptr;
  if (isa<UndefValue>(V))
    SubVec = UndefValue::get(RegionTy);
  else if (isa<ConstantAggregateZero>(V))
    SubVec = ConstantAggregateZero::get(RegionTy);
  else {
    SmallVector<Constant *, 32> Val;
    for (unsigned i = 0; i != Size; ++i)
      Val.push_back(V->getAggregateElement(i + StartIdx));
    SubVec = ConstantVector::get(Val);
  }
  return SubVec;
}

/***********************************************************************
 * concatConstants : concatenate two possibly vector constants, giving a
 *      vector constant
 */
Constant *genx::concatConstants(Constant *C1, Constant *C2) {
  IGC_ASSERT(C1->getType()->getScalarType() == C2->getType()->getScalarType());
  Constant *CC[] = {C1, C2};
  SmallVector<Constant *, 8> Vec;
  bool AllUndef = true;
  for (unsigned Idx = 0; Idx != 2; ++Idx) {
    Constant *C = CC[Idx];
    if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(C->getType())) {
      for (unsigned i = 0, e = VT->getNumElements(); i != e; ++i) {
        Constant *El = C->getAggregateElement(i);
        Vec.push_back(El);
        AllUndef &= isa<UndefValue>(El);
      }
    } else {
      Vec.push_back(C);
      AllUndef &= isa<UndefValue>(C);
    }
  }
  auto Res = ConstantVector::get(Vec);
  if (AllUndef)
    Res = UndefValue::get(Res->getType());
  return Res;
}

/***********************************************************************
 * findClosestCommonDominator : find closest common dominator of some
 * instructions
 *
 * Enter:   DT = dominator tree
 *          Insts = the instructions
 *
 * Return:  The one instruction that dominates all the others, if any.
 *          Otherwise the terminator of the closest common dominating basic
 *          block.
 */
Instruction *genx::findClosestCommonDominator(const DominatorTree *DT,
                                              ArrayRef<Instruction *> Insts) {
  IGC_ASSERT(!Insts.empty());
  SmallVector<InstScanner, 8> InstScanners;
  // Find the closest common dominating basic block.
  Instruction *Inst0 = Insts[0];
  BasicBlock *NCD = Inst0->getParent();
  InstScanners.push_back(InstScanner(Inst0));
  for (unsigned ii = 1, ie = Insts.size(); ii != ie; ++ii) {
    Instruction *Inst = Insts[ii];
    if (Inst->getParent() != NCD) {
      auto NewNCD = DT->findNearestCommonDominator(NCD, Inst->getParent());
      if (NewNCD != NCD)
        InstScanners.clear();
      NCD = NewNCD;
    }
    if (NCD == Inst->getParent())
      InstScanners.push_back(Inst);
  }
  // Now we have NCD = the closest common dominating basic block, and
  // InstScanners populated with the instructions from Insts that are
  // in that block.
  if (InstScanners.empty()) {
    // No instructions in that block. Return the block's terminator.
    return NCD->getTerminator();
  }
  if (InstScanners.size() == 1) {
    // Only one instruction in that block. Return it.
    return InstScanners[0].Original;
  }
  // Create a set of the original instructions.
  std::set<Instruction *> OrigInsts;
  for (auto i = InstScanners.begin(), e = InstScanners.end(); i != e; ++i)
    OrigInsts.insert(i->Original);
  // Scan back one instruction at a time for each scanner. If a scanner reaches
  // another original instruction, the scanner can be removed, and when we are
  // left with one scanner, that must be the earliest of the original
  // instructions.  If a scanner reaches the start of the basic block, that was
  // the earliest of the original instructions.
  //
  // In the worst case, this algorithm could scan all the instructions in a
  // basic block, but it is designed to be better than that in the common case
  // that the original instructions are close to each other.
  for (;;) {
    for (auto i = InstScanners.begin(), e = InstScanners.end(); i != e; ++i) {
      if (i->Current == &i->Current->getParent()->front())
        return i->Original; // reached start of basic block
      i->Current = i->Current->getPrevNode();
      if (OrigInsts.find(i->Current) != OrigInsts.end()) {
        // Scanned back to another instruction in our original set. Remove
        // this scanner.
        *i = InstScanners.back();
        InstScanners.pop_back();
        if (InstScanners.size() == 1)
          return InstScanners[0].Original; // only one scanner left
        break; // restart loop so as not to confuse the iterator
      }
    }
  }
}

/***********************************************************************
 * getTwoAddressOperandNum : get operand number of two address operand
 *
 * If an intrinsic has a "two address operand", then that operand must be
 * in the same register as the result. This function returns the operand number
 * of the two address operand if any, or None if not.
 */
std::optional<unsigned> genx::getTwoAddressOperandNum(CallInst *CI) {
  auto IntrinsicID = vc::getAnyIntrinsicID(CI);
  if (!vc::isAnyNonTrivialIntrinsic(IntrinsicID))
    return std::nullopt; // not intrinsic
  // wr(pred(pred))region has operand 0 as two address operand
  if (GenXIntrinsic::isWrRegion(IntrinsicID) ||
      IntrinsicID == GenXIntrinsic::genx_wrpredregion ||
      IntrinsicID == GenXIntrinsic::genx_wrpredpredregion)
    return GenXIntrinsic::GenXRegion::OldValueOperandNum;
  if (CI->getType()->isVoidTy())
    return std::nullopt; // no return value
  GenXIntrinsicInfo II(IntrinsicID);
  unsigned Num = IGCLLVM::getNumArgOperands(CI);
  if (!Num)
    return std::nullopt; // no args
  --Num;                 // Num = last arg number, could be two address operand
  if (isa<UndefValue>(CI->getOperand(Num)))
    return std::nullopt; // operand is undef, must be RAW_NULLALLOWED
  if (II.getArgInfo(Num).getCategory() != GenXIntrinsicInfo::TWOADDR)
    return std::nullopt; // not two addr operand
  if (CI->use_empty() && II.getRetInfo().rawNullAllowed())
    return std::nullopt; // unused result will be V0
  return Num;            // it is two addr
}

/***********************************************************************
 * isPredicate : test whether an instruction has predicate (i1 or vector of i1)
 * type
 */
bool genx::isPredicate(Instruction *Inst) {
  return Inst->getType()->isIntOrIntVectorTy(1);
}

/***********************************************************************
 * isNot : test whether an instruction is a "not" instruction (an xor with
 *    constant all ones)
 */
bool genx::isNot(Instruction *Inst) {
  if (Inst->getOpcode() == Instruction::Xor)
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1)))
      if (C->isAllOnesValue())
        return true;
  return false;
}

/***********************************************************************
 * isPredNot : test whether an instruction is a "not" instruction (an xor
 *    with constant all ones) with predicate (i1 or vector of i1) type
 */
bool genx::isPredNot(Instruction *Inst) {
  return isPredicate(Inst) && isNot(Inst);
}

/***********************************************************************
 * isIntNot : test whether an instruction is a "not" instruction (an xor
 *    with constant all ones) with non-predicate type
 */
bool genx::isIntNot(Instruction *Inst) {
  return !isPredicate(Inst) && isNot(Inst);
}

/***********************************************************************
 * invertCondition : Invert the given predicate value, possibly reusing
 * an existing copy.
 */
Value *genx::invertCondition(Value *Condition) {
  IGC_ASSERT_MESSAGE(Condition->getType()->getScalarType()->isIntegerTy(1),
                     "Condition is not of predicate type");
  // First: Check if it's a constant.
  if (Constant *C = dyn_cast<Constant>(Condition))
    return ConstantExpr::getNot(C);

  // Second: If the condition is already inverted, return the original value.
  Instruction *Inst = dyn_cast<Instruction>(Condition);
  if (Inst && isPredNot(Inst))
    return Inst->getOperand(0);

  // Last option: Create a new instruction.
  auto *Inverted =
      BinaryOperator::CreateNot(Condition, Condition->getName() + ".inv");
  if (Inst && !isa<PHINode>(Inst))
    Inverted->insertAfter(Inst);
  else {
    BasicBlock *Parent = nullptr;
    if (Inst)
      Parent = Inst->getParent();
    else if (Argument *Arg = dyn_cast<Argument>(Condition))
      Parent = &Arg->getParent()->getEntryBlock();
    IGC_ASSERT_MESSAGE(Parent, "Unsupported condition to invert");
    Inverted->insertBefore(&*Parent->getFirstInsertionPt());
  }
  return Inverted;
}

/***********************************************************************
 * isNoopCast : test if cast operation doesn't modify bitwise representation
 * of value (in other words, it can be copy-coalesced).
 * NOTE: LLVM has CastInst::isNoopCast method, but it conservatively treats
 * AddrSpaceCast as modifying operation; this function can be more aggresive
 * relying on DataLayout information.
 */
bool genx::isNoopCast(const CastInst *CI) {
  const DataLayout &DL = CI->getModule()->getDataLayout();
  switch (CI->getOpcode()) {
  case Instruction::BitCast:
    return true;
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::AddrSpaceCast:
    return vc::getTypeSize(CI->getDestTy(), &DL) ==
           vc::getTypeSize(CI->getSrcTy(), &DL);
  default:
    return false;
  }
}

bool genx::isBFloat16Cast(const Instruction *I) {
  Type *Ty = nullptr;
  if (isa<FPTruncInst>(I))
    Ty = I->getType();
  else if (isa<FPExtInst>(I))
    Ty = I->getOperand(0)->getType();
  else
    return false;
  IGC_ASSERT_EXIT(Ty);
  return Ty->getScalarType()->isBFloatTy();
}

/***********************************************************************
 * ShuffleVectorAnalyzer::getAsSlice : see if the shufflevector is a slice on
 *    operand 0, and if so return the start index, or -1 if it is not a slice
 */
int ShuffleVectorAnalyzer::getAsSlice() {
  unsigned WholeWidth =
      cast<IGCLLVM::FixedVectorType>(SI->getOperand(0)->getType())
          ->getNumElements();
  Constant *Selector = IGCLLVM::getShuffleMaskForBitcode(SI);
  unsigned Width =
      cast<IGCLLVM::FixedVectorType>(SI->getType())->getNumElements();
  auto *Aggr = Selector->getAggregateElement(0u);
  if (isa<UndefValue>(Aggr))
    return -1; // operand 0 is undef value
  unsigned StartIdx = cast<ConstantInt>(Aggr)->getZExtValue();
  if (StartIdx >= WholeWidth)
    return -1; // start index beyond operand 0
  unsigned SliceWidth;
  for (SliceWidth = 1; SliceWidth != Width; ++SliceWidth) {
    auto CI = dyn_cast<ConstantInt>(Selector->getAggregateElement(SliceWidth));
    if (!CI)
      break;
    if (CI->getZExtValue() != StartIdx + SliceWidth)
      return -1; // not slice
  }
  return StartIdx;
}

/***********************************************************************
 * ShuffleVectorAnalyzer::isReplicatedSlice : check if the shufflevector
 * is a replicated slice on operand 0.
 */
bool ShuffleVectorAnalyzer::isReplicatedSlice() const {
  const auto MaskVals = SI->getShuffleMask();
  auto Begin = MaskVals.begin();
  auto End = MaskVals.end();

  // Check for undefs.
  if (std::find(Begin, End, -1) != End)
    return false;

  if (MaskVals.size() == 1)
    return true;

  // Slice should not touch second operand.
  auto MaxIndex = static_cast<size_t>(MaskVals.back());
  if (MaxIndex >= cast<IGCLLVM::FixedVectorType>(SI->getOperand(0)->getType())
                      ->getNumElements())
    return false;

  // Find first non-one difference.
  auto SliceEnd = std::adjacent_find(
      Begin, End, [](int Prev, int Next) { return Next - Prev != 1; });
  // If not found, then it is simple slice.
  if (SliceEnd == End)
    return true;

  // Compare slice with parts of sequence to prove that it is periodic.
  ++SliceEnd;
  unsigned SliceSize = std::distance(Begin, SliceEnd);
  // Slice should be replicated.
  if (MaskVals.size() % SliceSize != 0)
    return false;

  for (auto It = SliceEnd; It != End; std::advance(It, SliceSize))
    if (!std::equal(Begin, SliceEnd, It))
      return false;

  return true;
}

Value *genx::getMaskOperand(const Instruction *Inst) {
  IGC_ASSERT(Inst);

  // return null for any other intrusction except
  // genx intrinsics
  auto *CI = dyn_cast<CallInst>(Inst);
  auto IID = vc::getAnyIntrinsicID(Inst);
  if (!CI || !vc::isAnyNonTrivialIntrinsic(IID))
    return nullptr;

  auto MaskOpIt = llvm::find_if(CI->operands(), [](const Use &U) {
    Value *Operand = U.get();
    if (auto *VT = dyn_cast<VectorType>(Operand->getType()))
      return VT->getElementType()->isIntegerTy(1);
    return false;
  });

  // No mask among opernads
  if (MaskOpIt == CI->op_end())
    return nullptr;

  return *MaskOpIt;
}

// Based on the value of a shufflevector mask element defines in which of
// 2 operands it points. The operand is returned.
static Value *getOperandByMaskValue(const ShuffleVectorInst &SI,
                                    int MaskValue) {
  IGC_ASSERT_MESSAGE(MaskValue >= 0, "invalid index");
  int FirstOpSize = cast<IGCLLVM::FixedVectorType>(SI.getOperand(0)->getType())
                        ->getNumElements();
  if (MaskValue < FirstOpSize)
    return SI.getOperand(0);
  else {
    int SecondOpSize =
        cast<IGCLLVM::FixedVectorType>(SI.getOperand(1)->getType())
            ->getNumElements();
    IGC_ASSERT_MESSAGE(MaskValue < FirstOpSize + SecondOpSize, "invalid index");
    return SI.getOperand(1);
  }
}

// safe advance
// If adding \p N results in bound violation, \p Last is written to \p It
template <typename Iter> void advanceSafe(Iter &It, Iter Last, int N) {
  if (N > std::distance(It, Last)) {
    It = Last;
    return;
  }
  std::advance(It, N);
}

// Returns operand and its region of 1 element that is referenced by
// \p MaskVal element of shufflevector mask.
static ShuffleVectorAnalyzer::OperandRegionInfo
matchOneElemRegion(const ShuffleVectorInst &SI, int MaskVal) {
  ShuffleVectorAnalyzer::OperandRegionInfo Init;
  Init.Op = getOperandByMaskValue(SI, MaskVal);
  Init.R = Region(Init.Op);
  Init.R.NumElements = Init.R.Width = 1;
  if (Init.Op == SI.getOperand(0))
    Init.R.Offset = MaskVal * Init.R.ElementBytes;
  else {
    auto FirstOpSize =
        cast<IGCLLVM::FixedVectorType>(SI.getOperand(0)->getType())
            ->getNumElements();
    Init.R.Offset = (MaskVal - FirstOpSize) * Init.R.ElementBytes;
  }
  return Init;
}

class MaskIndex {
  int Idx;
  static constexpr const int Undef = -1;
  static constexpr const int AnotherOp = -2;

public:
  explicit MaskIndex(int InitIdx = 0) : Idx(InitIdx) {
    IGC_ASSERT_MESSAGE(Idx >= 0, "Defined index must not be negative");
  }

  static MaskIndex getUndef() {
    MaskIndex Ret;
    Ret.Idx = Undef;
    return Ret;
  }

  static MaskIndex getAnotherOp() {
    MaskIndex Ret;
    Ret.Idx = AnotherOp;
    return Ret;
  }
  bool isUndef() const { return Idx == Undef; }
  bool isAnotherOp() const { return Idx == AnotherOp; }
  bool isDefined() const { return Idx >= 0; }

  int get() const {
    IGC_ASSERT_MESSAGE(Idx >= 0, "Can't call get() on invalid index");
    return Idx;
  }

  int operator-(MaskIndex const &rhs) const {
    IGC_ASSERT_MESSAGE(isDefined(), "All operand indices must be valid");
    IGC_ASSERT_MESSAGE(rhs.isDefined(), "All operand indices must be valid");
    return Idx - rhs.Idx;
  }
};

// Takes shufflevector mask indexes from [\p FirstIt, \p LastIt),
// converts them to the indices of \p Operand of \p SI instruction
// and writes them to \p OutIt. Value type of OutIt is MaskIndex.
template <typename ForwardIter, typename OutputIter>
void makeSVIIndexesOperandIndexes(const ShuffleVectorInst &SI,
                                  const Value &Operand, ForwardIter FirstIt,
                                  ForwardIter LastIt, OutputIter OutIt) {
  int FirstOpSize = cast<IGCLLVM::FixedVectorType>(SI.getOperand(0)->getType())
                        ->getNumElements();
  if (&Operand == SI.getOperand(0)) {
    std::transform(FirstIt, LastIt, OutIt, [FirstOpSize](int MaskVal) {
      if (MaskVal >= FirstOpSize)
        return MaskIndex::getAnotherOp();
      return MaskVal < 0 ? MaskIndex::getUndef() : MaskIndex{MaskVal};
    });
    return;
  }
  IGC_ASSERT_MESSAGE(&Operand == SI.getOperand(1),
                     "wrong argument: a shufflevector operand was expected");
  std::transform(FirstIt, LastIt, OutIt, [FirstOpSize](int MaskVal) {
    if (MaskVal < 0)
      return MaskIndex::getUndef();
    return MaskVal >= FirstOpSize ? MaskIndex{MaskVal - FirstOpSize}
                                  : MaskIndex::getAnotherOp();
  });
}

// Calculates horisontal stride for region by scanning mask indices in
// range [\p FirstIt, \p LastIt).
//
// Arguments:
//    [\p FirstIt, \p LastIt) is the range of MaskIndex. There must not be
//    any AnotherOp indices in the range.
//    \P FirstIt must point to a defined index.
// Return value:
//    std::pair with first element to be Iterator to next defined element
//    or std::next(FirstIt) if there is no such one and second element to
//    be estimated stride if positive and integer, empty value otherwise.
template <typename ForwardIter>
std::pair<ForwardIter, std::optional<int>>
estimateHorizontalStride(ForwardIter FirstIt, ForwardIter LastIt) {

  IGC_ASSERT_MESSAGE(FirstIt != LastIt,
                     "the range must contain at least 1 element");
  IGC_ASSERT_MESSAGE(
      std::none_of(FirstIt, LastIt,
                   [](MaskIndex Idx) { return Idx.isAnotherOp(); }),
      "There must not be any AnotherOp indices in the range");
  IGC_ASSERT_MESSAGE(FirstIt->isDefined(),
                     "first element in range must be a valid index");
  auto NextDefined =
      std::find_if(std::next(FirstIt), LastIt,
                   [](MaskIndex Elem) { return Elem.isDefined(); });

  if (NextDefined == LastIt)
    return {std::next(FirstIt), std::optional<int>{}};

  int TotalStride = *NextDefined - *FirstIt;
  int TotalWidth = std::distance(FirstIt, NextDefined);

  if (TotalStride < 0 || (TotalStride % TotalWidth != 0 && TotalStride != 0))
    return {NextDefined, std::optional<int>{}};

  return {NextDefined, TotalStride / TotalWidth};
}

// Matches "vector" region (with vstride == 0) pattern in
// [\p FirstIt, \p LastIt) indexes.
// Uses info in \p FirstElemRegion, adds defined Width, Stride and
// new NumElements to \p FirstElemRegion and returns resulting region.
//
// Arguments:
//    [\p FirstIt, \p LastIt) is the range of MaskIndex. There must not be
//    any AnotherOp indices in the range.
//    FirstIt and std::prev(LastIt) must point to a defined indices.
//    \p FirstElemRegion describes one element region with only one index
//    *FirstIt.
//    \p BoundIndex is maximum possible index of the input vector + 1
//    (BoundIndex == InputVector.length)
template <typename ForwardIter>
Region matchVectorRegionByIndexes(Region FirstElemRegion, ForwardIter FirstIt,
                                  ForwardIter LastIt, int BoundIndex) {
  IGC_ASSERT_MESSAGE(FirstIt != LastIt,
                     "the range must contain at least 1 element");
  IGC_ASSERT_MESSAGE(
      std::none_of(FirstIt, LastIt,
                   [](MaskIndex Idx) { return Idx.isAnotherOp(); }),
      "There must not be any AnotherOp indices in the range.");
  IGC_ASSERT_MESSAGE(FirstIt->isDefined(),
                     "expected FirstIt and --LastIt point to valid indices");
  IGC_ASSERT_MESSAGE(std::prev(LastIt)->isDefined(),
                     "expected FirstIt and --LastIt point to valid indices");

  if (std::distance(FirstIt, LastIt) == 1)
    return FirstElemRegion;

  std::optional<int> RefStride;
  ForwardIter NewRowIt;
  std::tie(NewRowIt, RefStride) = estimateHorizontalStride(FirstIt, LastIt);

  if (!RefStride)
    return FirstElemRegion;

  std::optional<int> Stride = RefStride;
  while (Stride == RefStride)
    std::tie(NewRowIt, Stride) = estimateHorizontalStride(NewRowIt, LastIt);

  auto TotalStride = std::distance(FirstIt, std::prev(NewRowIt)) * *RefStride;
  auto Overstep = TotalStride + FirstIt->get() - BoundIndex + 1;
  if (Overstep > 0)
    NewRowIt = std::prev(NewRowIt, llvm::divideCeil(Overstep, *RefStride));

  int Width = std::distance(FirstIt, NewRowIt);
  IGC_ASSERT_MESSAGE(Width > 0, "should be at least 1 according to algorithm");
  if (Width == 1)
    // Stride doesn't play role when the Width is 1.
    // Also it prevents from writing to big value in the region.
    RefStride = 0;
  FirstElemRegion.Stride = *RefStride;
  FirstElemRegion.Width = Width;
  FirstElemRegion.NumElements = Width;
  return FirstElemRegion;
}

// Calculates vertical stride for region by scanning mask indices in
// range [\p FirstIt, \p LastIt).
//
// Arguments:
//    [\p FirstRowRegion]  describes "vector" region (with vstride == 0),
//      which is formed by first 'FirstRowRegion.NumElements' elements
//      of the range.
//    [\p FirstIt] Points to first element of vector of indices.
//    [\p ReferenceIt] Points to some valid reference element in that vector.
//    Must be out of range of first 'FirstRowRegion.NumElements' elements.
//    First 'FirstRowRegion.NumElements' in range must be defined indices.
// Return value:
//    Value of estimated vertical stride if it is positive and integer,
//    empty value otherwise
template <typename ForwardIter>
std::optional<int> estimateVerticalStride(Region FirstRowRegion,
                                          ForwardIter FirstIt,
                                          ForwardIter ReferenceIt) {

  IGC_ASSERT_MESSAGE(std::distance(FirstIt, ReferenceIt) >=
                         static_cast<std::ptrdiff_t>(FirstRowRegion.Width),
                     "Reference element must not be part of first row");
  IGC_ASSERT_MESSAGE(
      std::all_of(FirstIt, std::next(FirstIt, FirstRowRegion.Width),
                  [](MaskIndex Elem) { return Elem.isDefined(); }),
      "First row must contain only valid indices");
  IGC_ASSERT_MESSAGE(ReferenceIt->isDefined(), "Reference index must be valid");

  int Width = FirstRowRegion.Width;

  int TotalDistance = std::distance(FirstIt, ReferenceIt);
  int VStridesToDef = TotalDistance / Width;
  int HStridesToDef = TotalDistance % Width;
  int TotalVerticalStride = *ReferenceIt - *std::next(FirstIt, HStridesToDef);
  if (TotalVerticalStride < 0 || TotalVerticalStride % VStridesToDef != 0)
    return std::optional<int>{};

  return std::optional<int>{TotalVerticalStride / VStridesToDef};
}

// Matches "matrix" region (vstride may not equal to 0) pattern in
// [\p FirstIt, \p LastIt) index.
// Uses info in \p FirstRowRegion, adds defined VStride and new NumElements to
// \p FirstRowRegion and returns resulting region.
//
// Arguments:
//    [\p FirstIt, \p LastIt) is the range of MaskIndex. Note that this
//    pass may change the contents of this vector (replace undef indices
//    with defined ones), so it can affect further usage.
//    \p LastDefinedIt points to last element in a vector with a defined
//    index
//    \p FirstRowRegion describes "vector" region (with vstride == 0),
//      which is formed by first 'FirstRowRegion.NumElements' elements
//      of the range.
//    \p BoundIndex is maximum possible index of the input vector + 1
//    (BoundIndex == InputVector.length)
template <typename ForwardIter>
Region matchMatrixRegionByIndexes(Region FirstRowRegion, ForwardIter FirstIt,
                                  ForwardIter LastIt, ForwardIter LastDefinedIt,
                                  int BoundIndex) {
  IGC_ASSERT_MESSAGE(
      FirstRowRegion.NumElements == FirstRowRegion.Width,
      "wrong argunent: vector region (with no vstride) was expected");
  IGC_ASSERT_MESSAGE(
      FirstRowRegion.VStride == 0,
      "wrong argunent: vector region (with no vstride) was expected");
  IGC_ASSERT_MESSAGE(
      FirstIt->isDefined(),
      "expected FirstIt and LastDefinedIt point to valid indices");
  IGC_ASSERT_MESSAGE(
      LastDefinedIt->isDefined(),
      "expected FirstIt and LastDefinedIt point to valid indices");
  IGC_ASSERT_MESSAGE(std::distance(FirstIt, LastIt) >=
                         static_cast<std::ptrdiff_t>(FirstRowRegion.Width),
                     "wrong argument: number of indexes must be at least equal "
                     "to region width");

  auto FirstRowEndIt = std::next(FirstIt, FirstRowRegion.Width);
  if (FirstRowEndIt == LastIt)
    return FirstRowRegion;

  auto FirstDefined = std::find_if(FirstRowEndIt, LastIt, [](MaskIndex Idx) {
    return Idx.isDefined() || Idx.isAnotherOp();
  });
  if (FirstDefined == LastIt || FirstDefined->isAnotherOp())
    return FirstRowRegion;

  int Stride = FirstRowRegion.Stride;
  int Idx = FirstIt->get();
  std::generate(std::next(FirstIt), FirstRowEndIt,
                [Idx, Stride]() mutable { return MaskIndex{Idx += Stride}; });

  std::optional<int> VStride =
      estimateVerticalStride(FirstRowRegion, FirstIt, FirstDefined);
  if (!VStride)
    return FirstRowRegion;

  int VDistance = *VStride;
  int Width = FirstRowRegion.Width;
  int NumElements = FirstRowRegion.Width;
  int HighestFirstRowElement = std::prev(FirstRowEndIt)->get();

  for (auto It = FirstRowEndIt; It != LastIt; advanceSafe(It, LastIt, Width),
            NumElements += Width, VDistance += *VStride) {
    if (It > LastDefinedIt || std::distance(It, LastIt) < Width ||
        HighestFirstRowElement + VDistance >= BoundIndex ||
        !std::equal(FirstIt, FirstRowEndIt, It,
                    [VDistance](MaskIndex Reference, MaskIndex Current) {
                      return !Current.isAnotherOp() &&
                             (Current.isUndef() ||
                              Current.get() - Reference.get() == VDistance);
                    }))
      break;
  }

  if (NumElements == Width)
    // VStride doesn't play role when the Width is equal to NumElements.
    // Also it prevents from writing to big value in the region.
    VStride = 0;
  FirstRowRegion.VStride = *VStride;
  FirstRowRegion.NumElements = NumElements;
  return FirstRowRegion;
}

// Analyzes shufflevector mask starting from \p StartIdx element of it.
// Finds the longest prefix of the cutted shufflevector mask that can be
// represented as a region of one operand of the instruction.
// Returns the operand and its region.
//
// For example:
// {0, 1, 3, 4, 25, 16 ...} -> first 4 elements form a region:
//                             <3;2,1> vstride=3, width=2, stride=1
ShuffleVectorAnalyzer::OperandRegionInfo
ShuffleVectorAnalyzer::getMaskRegionPrefix(int StartIdx) {
  IGC_ASSERT_MESSAGE(StartIdx >= 0, "Start index is out of bound");
  IGC_ASSERT_MESSAGE(StartIdx < static_cast<int>(SI->getShuffleMask().size()),
                     "Start index is out of bound");

  auto MaskVals = SI->getShuffleMask();
  auto StartIt = std::next(MaskVals.begin(), StartIdx);
  OperandRegionInfo Res = matchOneElemRegion(*SI, *StartIt);

  if (StartIdx == MaskVals.size() - 1)
    return Res;

  std::vector<MaskIndex> SubMask;
  makeSVIIndexesOperandIndexes(*SI, *Res.Op, StartIt, MaskVals.end(),
                               std::back_inserter(SubMask));

  auto FirstAnotherOpElement =
      std::find_if(SubMask.begin(), SubMask.end(),
                   [](MaskIndex Elem) { return Elem.isAnotherOp(); });
  auto PastLastDefinedElement =
      std::find_if(std::reverse_iterator(FirstAnotherOpElement),
                   std::reverse_iterator(SubMask.begin()),
                   [](MaskIndex Elem) { return Elem.isDefined(); })
          .base();

  Res.R = matchVectorRegionByIndexes(
      std::move(Res.R), SubMask.begin(), PastLastDefinedElement,
      cast<IGCLLVM::FixedVectorType>(Res.Op->getType())->getNumElements());
  Res.R = matchMatrixRegionByIndexes(
      std::move(Res.R), SubMask.begin(), SubMask.end(),
      std::prev(PastLastDefinedElement),
      cast<IGCLLVM::FixedVectorType>(Res.Op->getType())->getNumElements());
  return Res;
}

/***********************************************************************
 * ShuffleVectorAnalyzer::getAsUnslice : see if the shufflevector is an
 *    unslice where the "old value" is operand 0 and operand 1 is another
 *    shufflevector and operand 0 of that is the "new value"
 *
 * Return:  start index, or -1 if it is not an unslice
 */
int ShuffleVectorAnalyzer::getAsUnslice() {
  auto SI2 = dyn_cast<ShuffleVectorInst>(SI->getOperand(1));
  if (!SI2)
    return -1;
  Constant *MaskVec = IGCLLVM::getShuffleMaskForBitcode(SI);
  // Find prefix of undef or elements from operand 0.
  unsigned OldWidth =
      cast<IGCLLVM::FixedVectorType>(SI2->getType())->getNumElements();
  unsigned NewWidth =
      cast<IGCLLVM::FixedVectorType>(SI2->getOperand(0)->getType())
          ->getNumElements();
  unsigned Prefix = 0;
  for (;; ++Prefix) {
    if (Prefix == OldWidth - NewWidth)
      break;
    Constant *IdxC = MaskVec->getAggregateElement(Prefix);
    if (isa<UndefValue>(IdxC))
      continue;
    unsigned Idx = cast<ConstantInt>(IdxC)->getZExtValue();
    if (Idx == OldWidth)
      break; // found end of prefix
    if (Idx != Prefix)
      return -1; // not part of prefix
  }
  // Check that the whole of SI2 operand 0 follows
  for (unsigned i = 1; i != NewWidth; ++i) {
    Constant *IdxC = MaskVec->getAggregateElement(Prefix + i);
    if (isa<UndefValue>(IdxC))
      continue;
    if (cast<ConstantInt>(IdxC)->getZExtValue() != i + OldWidth)
      return -1; // not got whole of SI2 operand 0
  }
  // Check that the remainder is undef or elements from operand 0.
  for (unsigned i = Prefix + NewWidth; i != OldWidth; ++i) {
    Constant *IdxC = MaskVec->getAggregateElement(i);
    if (isa<UndefValue>(IdxC))
      continue;
    if (cast<ConstantInt>(IdxC)->getZExtValue() != i)
      return -1;
  }
  // Check that the first Prefix elements of SI2 come from its operand 1.
  Constant *MaskVec2 = IGCLLVM::getShuffleMaskForBitcode(SI2);
  for (unsigned i = 0; i != Prefix; ++i) {
    Constant *IdxC = MaskVec2->getAggregateElement(Prefix + i);
    if (isa<UndefValue>(IdxC))
      continue;
    if (cast<ConstantInt>(IdxC)->getZExtValue() != i)
      return -1;
  }
  // Success.
  return Prefix;
}

/***********************************************************************
 * extension of ShuffleVectorInst::isZeroEltSplatMask method
 */
static int nEltSplatMask(ArrayRef<int> Mask) {
  int Elt = UndefMaskElem;
  for (int i = 0, NumElts = Mask.size(); i < NumElts; ++i) {
    if (Mask[i] == UndefMaskElem)
      continue;
    if ((Elt != UndefMaskElem) && (Mask[i] != Mask[Elt]))
      return UndefMaskElem;
    if ((Mask[i] != UndefMaskElem) && (Elt == UndefMaskElem))
      Elt = i;
  }
  return Elt;
}

/***********************************************************************
 * ShuffleVectorAnalyzer::getAsSplat : if shufflevector is a splat, get the
 *      splatted input, with its vector index if the input is a vector
 */
ShuffleVectorAnalyzer::SplatInfo ShuffleVectorAnalyzer::getAsSplat() {
  Value *InVec1 = SI->getOperand(0);
  Value *InVec2 = SI->getOperand(1);

  SmallVector<int, 16> MaskAsInts;
  SI->getShuffleMask(MaskAsInts);
  int ShuffleIdx = nEltSplatMask(MaskAsInts);
  if (ShuffleIdx == UndefMaskElem)
    return SplatInfo(nullptr, 0);

  // We have position of shuffleindex as output, turn it to real index
  ShuffleIdx = MaskAsInts[ShuffleIdx];

  // The mask is a splat. Work out which element of which input vector
  // it refers to.
  int InVec1NumElements =
      cast<IGCLLVM::FixedVectorType>(InVec1->getType())->getNumElements();
  if (ShuffleIdx >= InVec1NumElements) {
    ShuffleIdx -= InVec1NumElements;
    InVec1 = InVec2;
  }
  if (auto IE = dyn_cast<InsertElementInst>(InVec1)) {
    if (InVec1NumElements == 1 || isa<UndefValue>(IE->getOperand(0)))
      return SplatInfo(IE->getOperand(1), 0);
    // Even though this is a splat, the input vector has more than one
    // element. IRBuilder::CreateVectorSplat does this. See if the input
    // vector is the result of an insertelement at the right place, and
    // if so return that. Otherwise we end up allocating
    // an unnecessarily large register.
    if (auto ConstIdx = dyn_cast<ConstantInt>(IE->getOperand(2)))
      if (ConstIdx->getSExtValue() == ShuffleIdx)
        return SplatInfo(IE->getOperand(1), 0);
  }
  return SplatInfo(InVec1, ShuffleIdx);
}

Value *ShuffleVectorAnalyzer::serialize() {
  unsigned Cost0 = getSerializeCost(0);
  unsigned Cost1 = getSerializeCost(1);

  Value *Op0 = SI->getOperand(0);
  Value *Op1 = SI->getOperand(1);
  Value *V = Op0;
  bool UseOp0AsBase = Cost0 <= Cost1;
  if (!UseOp0AsBase)
    V = Op1;

  // Expand or shink the initial value if sizes mismatch.
  unsigned NElts =
      cast<IGCLLVM::FixedVectorType>(SI->getType())->getNumElements();
  unsigned M = cast<IGCLLVM::FixedVectorType>(V->getType())->getNumElements();
  bool SkipBase = true;
  if (M != NElts) {
    if (auto C = dyn_cast<Constant>(V)) {
      SmallVector<Constant *, 16> Vals;
      for (unsigned i = 0; i < NElts; ++i) {
        Type *Ty = cast<VectorType>(C->getType())->getElementType();
        Constant *Elt =
            (i < M) ? C->getAggregateElement(i) : UndefValue::get(Ty);
        Vals.push_back(Elt);
      }
      V = ConstantVector::get(Vals);
    } else {
      // Need to insert individual elements.
      V = UndefValue::get(SI->getType());
      SkipBase = false;
    }
  }

  IRBuilder<> Builder(SI);
  for (unsigned i = 0; i < NElts; ++i) {
    // Undef index returns -1.
    int idx = SI->getMaskValue(i);
    if (idx < 0)
      continue;
    if (SkipBase) {
      if (UseOp0AsBase && idx == i)
        continue;
      if (!UseOp0AsBase && idx == i + M)
        continue;
    }

    Value *Vi = nullptr;
    if (idx < (int)M)
      Vi = Builder.CreateExtractElement(Op0, idx, "");
    else
      Vi = Builder.CreateExtractElement(Op1, idx - M, "");
    if (!isa<UndefValue>(Vi))
      V = Builder.CreateInsertElement(V, Vi, i, "");
  }

  return V;
}

unsigned ShuffleVectorAnalyzer::getSerializeCost(unsigned i) {
  unsigned Cost = 0;
  Value *Op = SI->getOperand(i);
  if (!isa<Constant>(Op) && Op->getType() != SI->getType())
    Cost += cast<IGCLLVM::FixedVectorType>(Op->getType())->getNumElements();

  unsigned NElts =
      cast<IGCLLVM::FixedVectorType>(SI->getType())->getNumElements();
  for (unsigned j = 0; j < NElts; ++j) {
    // Undef index returns -1.
    int idx = SI->getMaskValue(j);
    if (idx < 0)
      continue;
    // Count the number of elements out of place.
    unsigned M =
        cast<IGCLLVM::FixedVectorType>(Op->getType())->getNumElements();
    if ((i == 0 && idx != j) || (i == 1 && idx != j + M))
      Cost++;
  }

  return Cost;
}

IVSplitter::IVSplitter(Instruction &Inst, const unsigned *BaseOpIdx)
    : Inst(Inst) {

  ETy = Inst.getType();
  if (BaseOpIdx)
    ETy = Inst.getOperand(*BaseOpIdx)->getType();

  Len = 1;
  if (auto *EVTy = dyn_cast<IGCLLVM::FixedVectorType>(ETy)) {
    Len = EVTy->getNumElements();
    ETy = EVTy->getElementType();
  }

  VI32Ty = IGCLLVM::FixedVectorType::get(ETy->getInt32Ty(Inst.getContext()),
                                         Len * 2);
}

IVSplitter::RegionTrait IVSplitter::describeSplit(RegionType RT, size_t ElNum) {
  RegionTrait Result;
  if (RT == RegionType::LoRegion || RT == RegionType::HiRegion) {
    // take every second element;
    Result.ElStride = 2;
    Result.ElOffset = (RT == RegionType::LoRegion) ? 0 : 1;
  } else if (RT == RegionType::FirstHalf || RT == RegionType::SecondHalf) {
    // take every element, sequentially
    Result.ElStride = 1;
    Result.ElOffset = (RT == RegionType::FirstHalf) ? 0 : ElNum;
  } else {
    IGC_ASSERT_EXIT_MESSAGE(0, "incorrect region type");
  }
  return Result;
}

Constant *
IVSplitter::splitConstantVector(const SmallVectorImpl<Constant *> &KV32,
                                RegionType RT) {
  IGC_ASSERT(KV32.size() % 2 == 0);
  SmallVector<Constant *, 16> Result;
  size_t ElNum = KV32.size() / 2;
  Result.reserve(ElNum);
  auto Split = describeSplit(RT, ElNum);
  for (size_t i = 0; i < ElNum; ++i) {
    size_t Offset = Split.ElOffset + i * Split.ElStride;
    IGC_ASSERT(Offset < KV32.size());
    Result.push_back(KV32[Offset]);
  }
  return ConstantVector::get(Result);
}

Region IVSplitter::createSplitRegion(Type *SrcTy, IVSplitter::RegionType RT) {
  IGC_ASSERT(SrcTy->isVectorTy());
  IGC_ASSERT(SrcTy->getScalarType()->isIntegerTy(32));
  IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(SrcTy)->getNumElements() % 2 == 0);

  size_t Len = cast<IGCLLVM::FixedVectorType>(SrcTy)->getNumElements() / 2;

  auto Split = describeSplit(RT, Len);

  Region R(SrcTy);
  R.Width = Len;
  R.NumElements = Len;
  R.VStride = 0;
  R.Stride = Split.ElStride;
  // offset is encoded in bytes
  R.Offset = Split.ElOffset * 4;

  return R;
}

// function takes 64-bit constant value (vector or scalar) and splits it
// into an equivalent vector of 32-bit constant (as if it was Bitcast-ed)
static void convertI64ToI32(Constant &K, SmallVectorImpl<Constant *> &K32) {
  auto I64To32 = [](Constant &K) {
    // we expect only scalar types here
    IGC_ASSERT(!isa<VectorType>(K.getType()));
    IGC_ASSERT(K.getType()->isIntegerTy(64));
    auto *Ty32 = Type::getInt32Ty(K.getContext());
    if (isa<UndefValue>(K)) {
      Constant *Undef = UndefValue::get(Ty32);
      return std::make_pair(Undef, Undef);
    }
    if (isa<ConstantExpr>(K)) {
      auto *Lo = ConstantExpr::getTrunc(&K, Ty32);
      auto *Amount = ConstantInt::get(K.getType(), 32);
      auto *Shift = ConstantExpr::getLShr(&K, Amount);
      auto *Hi = ConstantExpr::getTrunc(Shift, Ty32);
      return std::make_pair(Lo, Hi);
    }
    IGC_ASSERT_EXIT(isa<ConstantInt>(K));
    auto *KI = cast<ConstantInt>(&K);
    uint64_t Val64 = KI->getZExtValue();
    const auto UI32ValueMask = std::numeric_limits<uint32_t>::max();
    Constant *VLo =
        ConstantInt::get(Ty32, static_cast<uint32_t>(Val64 & UI32ValueMask));
    Constant *VHi = ConstantInt::get(Ty32, static_cast<uint32_t>(Val64 >> 32));
    return std::make_pair(VLo, VHi);
  };

  IGC_ASSERT(K32.empty());
  if (!isa<VectorType>(K.getType())) {
    auto V32 = I64To32(K);
    K32.push_back(V32.first);
    K32.push_back(V32.second);
    return;
  }
  unsigned ElNum =
      cast<IGCLLVM::FixedVectorType>(K.getType())->getNumElements();
  K32.reserve(2 * ElNum);
  for (unsigned i = 0; i < ElNum; ++i) {
    auto V32 = I64To32(*K.getAggregateElement(i));
    K32.push_back(V32.first);
    K32.push_back(V32.second);
  }
}

std::pair<Value *, Value *>
IVSplitter::splitValue(Value &Val, RegionType RT1, const Twine &Name1,
                       RegionType RT2, const Twine &Name2, bool FoldConstants) {
  const auto &DL = Inst.getDebugLoc();
  auto BaseName = Inst.getName();

  IGC_ASSERT(Val.getType()->getScalarType()->isIntegerTy(64));

  if (FoldConstants && isa<Constant>(Val)) {
    SmallVector<Constant *, 32> KV32;
    convertI64ToI32(cast<Constant>(Val), KV32);
    Value *V1 = splitConstantVector(KV32, RT1);
    Value *V2 = splitConstantVector(KV32, RT2);
    return {V1, V2};
  }
  auto *ShreddedVal =
      new BitCastInst(&Val, VI32Ty, BaseName + ".iv32cast", &Inst);
  ShreddedVal->setDebugLoc(DL);

  auto R1 = createSplitRegion(VI32Ty, RT1);
  auto *V1 = R1.createRdRegion(ShreddedVal, BaseName + Name1, &Inst, DL);

  auto R2 = createSplitRegion(VI32Ty, RT2);
  auto *V2 = R2.createRdRegion(ShreddedVal, BaseName + Name2, &Inst, DL);
  return {V1, V2};
}

IVSplitter::LoHiSplit IVSplitter::splitOperandLoHi(unsigned SourceIdx,
                                                   bool FoldConstants) {

  IGC_ASSERT(Inst.getNumOperands() > SourceIdx);
  return splitValueLoHi(*Inst.getOperand(SourceIdx), FoldConstants);
}
IVSplitter::HalfSplit IVSplitter::splitOperandHalf(unsigned SourceIdx,
                                                   bool FoldConstants) {

  IGC_ASSERT(Inst.getNumOperands() > SourceIdx);
  return splitValueHalf(*Inst.getOperand(SourceIdx), FoldConstants);
}

IVSplitter::LoHiSplit IVSplitter::splitValueLoHi(Value &V, bool FoldConstants) {
  auto Splitted = splitValue(V, RegionType::LoRegion, ".LoSplit",
                             RegionType::HiRegion, ".HiSplit", FoldConstants);
  return {Splitted.first, Splitted.second};
}
IVSplitter::HalfSplit IVSplitter::splitValueHalf(Value &V, bool FoldConstants) {
  auto Splitted =
      splitValue(V, RegionType::FirstHalf, ".FirstHalf", RegionType::SecondHalf,
                 ".SecondHalf", FoldConstants);
  return {Splitted.first, Splitted.second};
}

Value *IVSplitter::combineSplit(Value &V1, Value &V2, RegionType RT1,
                                RegionType RT2, const Twine &Name,
                                bool Scalarize) {
  const auto &DL = Inst.getDebugLoc();

  IGC_ASSERT(V1.getType() == V2.getType());
  IGC_ASSERT(V1.getType()->isVectorTy());
  IGC_ASSERT(cast<VectorType>(V1.getType())->getElementType()->isIntegerTy(32));

  // create the write-regions
  auto R1 = createSplitRegion(VI32Ty, RT1);
  auto *UndefV = UndefValue::get(VI32Ty);
  auto *W1 = R1.createWrRegion(UndefV, &V1, Name + "partial_join", &Inst, DL);

  auto R2 = createSplitRegion(VI32Ty, RT2);
  auto *W2 = R2.createWrRegion(W1, &V2, Name + "joined", &Inst, DL);

  auto *V64Ty =
      IGCLLVM::FixedVectorType::get(ETy->getInt64Ty(Inst.getContext()), Len);
  auto *Result = new BitCastInst(W2, V64Ty, Name, &Inst);
  Result->setDebugLoc(DL);

  if (Scalarize) {
    IGC_ASSERT(
        cast<IGCLLVM::FixedVectorType>(Result->getType())->getNumElements() ==
        1);
    Result = new BitCastInst(Result, ETy->getInt64Ty(Inst.getContext()),
                             Name + "recast", &Inst);
    Result->setDebugLoc(DL);
  }
  return Result;
}
Value *IVSplitter::combineLoHiSplit(const LoHiSplit &Split, const Twine &Name,
                                    bool Scalarize) {
  IGC_ASSERT(Split.Lo);
  IGC_ASSERT(Split.Hi);

  return combineSplit(*Split.Lo, *Split.Hi, RegionType::LoRegion,
                      RegionType::HiRegion, Name, Scalarize);
}

Value *IVSplitter::combineHalfSplit(const HalfSplit &Split, const Twine &Name,
                                    bool Scalarize) {
  IGC_ASSERT(Split.Left);
  IGC_ASSERT(Split.Right);

  return combineSplit(*Split.Left, *Split.Right, RegionType::FirstHalf,
                      RegionType::SecondHalf, Name, Scalarize);
}
/***********************************************************************
 * adjustPhiNodesForBlockRemoval : adjust phi nodes when removing a block
 *
 * Enter:   Succ = the successor block to adjust phi nodes in
 *          BB = the block being removed
 *
 * This modifies each phi node in Succ as follows: the incoming for BB is
 * replaced by an incoming for each of BB's predecessors.
 */
void genx::adjustPhiNodesForBlockRemoval(BasicBlock *Succ, BasicBlock *BB) {
  for (auto i = Succ->begin(), e = Succ->end(); i != e; ++i) {
    auto Phi = dyn_cast<PHINode>(&*i);
    if (!Phi)
      break;
    // For this phi node, get the incoming for BB.
    int Idx = Phi->getBasicBlockIndex(BB);
    IGC_ASSERT_EXIT(Idx >= 0);
    Value *Incoming = Phi->getIncomingValue(Idx);
    // Iterate through BB's predecessors. For the first one, replace the
    // incoming block with the predecessor. For subsequent ones, we need
    // to add new phi incomings.
    auto pi = pred_begin(BB), pe = pred_end(BB);
    IGC_ASSERT(pi != pe);
    Phi->setIncomingBlock(Idx, *pi);
    for (++pi; pi != pe; ++pi)
      Phi->addIncoming(Incoming, *pi);
  }
}

/***********************************************************************
 * sinkAdd : sink add(s) in address calculation
 *
 * Enter:   IdxVal = the original index value
 *
 * Return:  the new calculation for the index value
 *
 * This detects the case when a variable index in a region or element access
 * is one or more constant add/subs then some mul/shl/truncs. It sinks
 * the add/subs into a single add after the mul/shl/truncs, so the add
 * stands a chance of being baled in as a constant offset in the region.
 *
 * If add sinking is successfully applied, it may leave now unused
 * instructions behind, which need tidying by a later dead code removal
 * pass.
 */
Value *genx::sinkAdd(Value *V) {
  Instruction *IdxVal = dyn_cast<Instruction>(V);
  if (!IdxVal)
    return V;
  // Collect the scale/trunc/add/sub/or instructions.
  int Offset = 0;
  SmallVector<Instruction *, 8> ScaleInsts;
  Instruction *Inst = IdxVal;
  int Scale = 1;
  bool NeedChange = false;
  for (;;) {
    if (isa<TruncInst>(Inst))
      ScaleInsts.push_back(Inst);
    else {
      if (!isa<BinaryOperator>(Inst))
        break;
      if (ConstantInt *CI = dyn_cast<ConstantInt>(Inst->getOperand(1))) {
        if (Inst->getOpcode() == Instruction::Mul) {
          Scale *= CI->getSExtValue();
          ScaleInsts.push_back(Inst);
        } else if (Inst->getOpcode() == Instruction::Shl) {
          Scale <<= CI->getSExtValue();
          ScaleInsts.push_back(Inst);
        } else if (Inst->getOpcode() == Instruction::Add) {
          Offset += CI->getSExtValue() * Scale;
          if (V != Inst)
            NeedChange = true;
        } else if (Inst->getOpcode() == Instruction::Sub) {
          Offset -= CI->getSExtValue() * Scale;
          if (IdxVal != Inst)
            NeedChange = true;
        } else if (Inst->getOpcode() == Instruction::Or) {
          if (!haveNoCommonBitsSet(Inst->getOperand(0), Inst->getOperand(1),
                                   Inst->getModule()->getDataLayout()))
            break;
          Offset += CI->getSExtValue() * Scale;
          if (V != Inst)
            NeedChange = true;
        } else
          break;
      } else
        break;
    }
    Inst = dyn_cast<Instruction>(Inst->getOperand(0));
    if (!Inst)
      return V;
  }
  if (!NeedChange)
    return V;
  // Clone the scale and trunc instructions, starting with the value that
  // was input to the add(s).
  for (SmallVectorImpl<Instruction *>::reverse_iterator i = ScaleInsts.rbegin(),
                                                        e = ScaleInsts.rend();
       i != e; ++i) {
    Instruction *Clone = (*i)->clone();
    Clone->insertBefore(IdxVal);
    Clone->setName((*i)->getName());
    Clone->setOperand(0, Inst);
    Inst = Clone;
  }
  // Create a new add instruction.
  Inst = BinaryOperator::Create(
      Instruction::Add, Inst,
      ConstantInt::get(Inst->getType(), (int64_t)Offset, true /*isSigned*/),
      Twine("addr_add"), IdxVal);
  Inst->setDebugLoc(IdxVal->getDebugLoc());
  return Inst;
}

/***********************************************************************
 * reorderBlocks : reorder blocks to increase fallthrough, and specifically
 *    to satisfy the requirements of SIMD control flow
 */
#define SUCCSZANY (true)
#define SUCCHASINST (succ->size() > 1)
#define SUCCNOINST (succ->size() <= 1)
#define SUCCANYLOOP (true)

#define PUSHSUCC(BLK, C1, C2)                                                  \
  for (succ_iterator succIter = succ_begin(BLK), succEnd = succ_end(BLK);      \
       succIter != succEnd; ++succIter) {                                      \
    llvm::BasicBlock *succ = *succIter;                                        \
    if (!visitSet.count(succ) && C1 && C2) {                                   \
      visitVec.push_back(succ);                                                \
      visitSet.insert(succ);                                                   \
      break;                                                                   \
    }                                                                          \
  }

static bool HasSimdGotoJoinInBlock(BasicBlock *BB) {
  for (BasicBlock::iterator BBI = BB->begin(), BBE = BB->end(); BBI != BBE;
       ++BBI) {
    auto IID = GenXIntrinsic::getGenXIntrinsicID(&*BBI);
    if (IID == GenXIntrinsic::genx_simdcf_goto ||
        IID == GenXIntrinsic::genx_simdcf_join)
      return true;
  }
  return false;
}

void genx::LayoutBlocks(Function &func, LoopInfo &LI) {
  std::vector<llvm::BasicBlock *> visitVec;
  std::set<llvm::BasicBlock *> visitSet;
  // Insertion Position per loop header
  std::map<llvm::BasicBlock *, llvm::BasicBlock *> InsPos;

  llvm::BasicBlock *entry = &(func.getEntryBlock());
  visitVec.push_back(entry);
  visitSet.insert(entry);
  InsPos[entry] = entry;

  while (!visitVec.empty()) {
    llvm::BasicBlock *blk = visitVec.back();
    llvm::Loop *curLoop = LI.getLoopFor(blk);
    if (curLoop) {
      auto hd = curLoop->getHeader();
      if (blk == hd && InsPos.find(hd) == InsPos.end()) {
        InsPos[blk] = blk;
      }
    }
    // push: time for DFS visit
    PUSHSUCC(blk, SUCCANYLOOP, SUCCNOINST);
    if (blk != visitVec.back())
      continue;
    // push: time for DFS visit
    PUSHSUCC(blk, SUCCANYLOOP, SUCCHASINST);
    // pop: time to move the block to the right location
    if (blk == visitVec.back()) {
      visitVec.pop_back();
      if (curLoop) {
        auto hd = curLoop->getHeader();
        if (blk != hd) {
          // move the block to the beginning of the loop
          auto insp = InsPos[hd];
          IGC_ASSERT(insp);
          if (blk != insp) {
            blk->moveBefore(insp);
            InsPos[hd] = blk;
          }
        } else {
          // move the entire loop to the beginning of
          // the parent loop
          auto LoopStart = InsPos[hd];
          IGC_ASSERT(LoopStart);
          auto PaLoop = curLoop->getParentLoop();
          auto PaHd = PaLoop ? PaLoop->getHeader() : entry;
          auto insp = InsPos[PaHd];
          if (LoopStart == hd) {
            // single block loop
            hd->moveBefore(insp);
          } else {
            // loop-header is not moved yet, so should be at the end
            // use splice
            IGCLLVM::splice(&func, insp->getIterator(), &func,
                            LoopStart->getIterator(), hd->getIterator());
            hd->moveBefore(LoopStart);
          }
          InsPos[PaHd] = hd;
        }
      } else {
        auto insp = InsPos[entry];
        if (blk != insp) {
          blk->moveBefore(insp);
          InsPos[entry] = blk;
        }
      }
    }
  }

  // fix the loop-exit pattern, put break-blocks into the loop
  for (llvm::Function::iterator blkIter = func.begin(), blkEnd = func.end();
       blkIter != blkEnd; ++blkIter) {
    llvm::BasicBlock *blk = &(*blkIter);
    llvm::Loop *curLoop = LI.getLoopFor(blk);
    bool allPredLoopExit = true;
    unsigned numPreds = 0;
    llvm::SmallPtrSet<llvm::BasicBlock *, 4> predSet;
    for (pred_iterator predIter = pred_begin(blk), predEnd = pred_end(blk);
         predIter != predEnd; ++predIter) {
      llvm::BasicBlock *pred = *predIter;
      numPreds++;
      llvm::Loop *predLoop = LI.getLoopFor(pred);
      if (curLoop == predLoop) {
        llvm::BasicBlock *predPred = pred->getSinglePredecessor();
        if (predPred) {
          llvm::Loop *predPredLoop = LI.getLoopFor(predPred);
          if (predPredLoop != curLoop &&
              (!curLoop || curLoop->contains(predPredLoop))) {
            if (!HasSimdGotoJoinInBlock(pred)) {
              predSet.insert(pred);
            } else {
              allPredLoopExit = false;
              break;
            }
          }
        }
      } else if (!curLoop || curLoop->contains(predLoop))
        continue;
      else {
        allPredLoopExit = false;
        break;
      }
    }
    if (allPredLoopExit && numPreds > 1) {
      for (SmallPtrSet<BasicBlock *, 4>::iterator predIter = predSet.begin(),
                                                  predEnd = predSet.end();
           predIter != predEnd; ++predIter) {
        llvm::BasicBlock *pred = *predIter;
        llvm::BasicBlock *predPred = pred->getSinglePredecessor();
        IGC_ASSERT(predPred);
        pred->moveAfter(predPred);
      }
    }
  }
}

void genx::LayoutBlocks(Function &func) {
  std::vector<llvm::BasicBlock *> visitVec;
  std::set<llvm::BasicBlock *> visitSet;
  // Reorder basic block to allow more fall-through
  llvm::BasicBlock *entry = &(func.getEntryBlock());
  visitVec.push_back(entry);
  visitSet.insert(entry);

  while (!visitVec.empty()) {
    llvm::BasicBlock *blk = visitVec.back();
    // push in the empty successor
    PUSHSUCC(blk, SUCCANYLOOP, SUCCNOINST);
    if (blk != visitVec.back())
      continue;
    // push in the other successor
    PUSHSUCC(blk, SUCCANYLOOP, SUCCHASINST);
    // pop
    if (blk == visitVec.back()) {
      visitVec.pop_back();
      if (blk != entry) {
        blk->moveBefore(entry);
        entry = blk;
      }
    }
  }
}

namespace {

template <class RetT, class ArgT,
          std::enable_if_t<std::is_same_v<std::remove_cv_t<RetT>, User> &&
                               std::is_same_v<std::remove_cv_t<ArgT>, Value>,
                           int> = 0>
inline llvm::SmallPtrSet<RetT *, 4>
genx_peelBitCastsGetUsers__impl(ArgT *const V) {
  // V = ...
  // A = bitcast (V)
  // B = use (bitcast (A))
  // C = use (A)
  // Provided with V argument returns set containing C and B for the particular
  // example above.
  llvm::SmallVector<RetT *, 4> BitCasts;
  llvm::SmallPtrSet<RetT *, 4> Res;
  for (const auto &UI : V->users())
    BitCasts.push_back(&*UI);
  while (!BitCasts.empty()) {
    auto *U = BitCasts.pop_back_val();
    if (isABitCast(U))
      for (const auto &UI : U->users())
        BitCasts.push_back(&*UI);
    else
      Res.insert(U);
  }
  return Res;
}

}; // namespace

llvm::SmallPtrSet<const User *, 4>
genx::peelBitCastsGetUsers(const Value *const V) {
  return ::genx_peelBitCastsGetUsers__impl<const User>(V);
}

llvm::SmallPtrSet<User *, 4> genx::peelBitCastsGetUsers(Value *const V) {
  return ::genx_peelBitCastsGetUsers__impl<User>(V);
}

// normalize g_load with bitcasts.
//
// When a single g_load is being bitcast'ed to different types, clone g_loads.
bool genx::normalizeGloads(Instruction *Inst) {
  IGC_ASSERT(isa<LoadInst>(Inst));
  auto LI = cast<LoadInst>(Inst);
  if (vc::getUnderlyingGlobalVariable(LI->getPointerOperand()) == nullptr)
    return false;

  // collect all uses connected by bitcasts.
  std::set<BitCastInst *> Visited;
  // Uses of this loads groupped by the use type.
  llvm::MapVector<Type *, std::vector<BitCastInst *>> Uses;
  // The working list.
  std::vector<BitCastInst *> Insts;

  for (auto UI : LI->users())
    if (auto BI = dyn_cast<BitCastInst>(UI))
      Insts.push_back(BI);

  while (!Insts.empty()) {
    BitCastInst *BCI = Insts.back();
    Insts.pop_back();
    if (Visited.count(BCI))
      continue;

    Uses[BCI->getType()].push_back(BCI);
    for (auto UI : BCI->users())
      if (auto BI = dyn_cast<BitCastInst>(UI))
        Insts.push_back(BI);
  }

  // There are more than two uses; clone loads that can fold bitcasts.
  if (Uses.size() <= 1)
    return false;

  // %0 = load gv
  // %1 = bitcast %0 to t1
  // %2 - bitcast %1 to t2
  //
  // ==>
  // %0 = load gv
  // %0.1 = load gv
  // %1 = bitcast %0 to t1
  // %2 - bitcast %0.1 to t2
  Instruction *LInst = LI;
  for (auto I = Uses.begin(); I != Uses.end(); ++I) {
    Type *Ty = I->first;
    if (LInst == nullptr) {
      LInst = LI->clone();
      LInst->insertAfter(LI);
    }
    Instruction *NewCI = new BitCastInst(LInst, Ty, ".clone", LInst);
    NewCI->moveAfter(LInst);
    auto &BInsts = I->second;
    for (auto BI : BInsts)
      BI->replaceAllUsesWith(NewCI);
    LInst = nullptr;
  }
  return true;
}

// fold bitcast instruction into Store by change pointer type.
Instruction *genx::foldBitCastInst(Instruction *Inst) {
  IGC_ASSERT(isa<LoadInst>(Inst) || isa<StoreInst>(Inst));
  auto LI = dyn_cast<LoadInst>(Inst);
  auto SI = dyn_cast<StoreInst>(Inst);

  Value *Ptr = LI ? LI->getPointerOperand() : SI->getPointerOperand();
  GlobalVariable *GV = vc::getUnderlyingGlobalVariable(Ptr);

  // Folding bitcast of SEV can produce getelementptr instructions
  // which must be avoided.
  if (!GV || vc::isDegenerateVectorType(*GV->getValueType()))
    return nullptr;

  if (SI) {
    Value *Val = SI->getValueOperand();
    if (auto CI = dyn_cast<BitCastInst>(Val)) {
      auto SrcTy = CI->getSrcTy();
      auto NewPtrTy = PointerType::get(SrcTy, SI->getPointerAddressSpace());
      auto NewPtr = ConstantExpr::getBitCast(GV, NewPtrTy);
      StoreInst *NewSI = new StoreInst(CI->getOperand(0), NewPtr,
                                       /*volatile*/ SI->isVolatile(), Inst);
      NewSI->takeName(SI);
      NewSI->setDebugLoc(Inst->getDebugLoc());
      Inst->eraseFromParent();
      return NewSI;
    }
  } else if (LI && LI->hasOneUse()) {
    if (auto CI = dyn_cast<BitCastInst>(LI->user_back())) {
      auto NewPtrTy =
          PointerType::get(CI->getType(), LI->getPointerAddressSpace());
      auto NewPtr = ConstantExpr::getBitCast(GV, NewPtrTy);
      auto NewLI = new LoadInst(CI->getType(), NewPtr, "",
                                /*volatile*/ LI->isVolatile(), Inst);
      NewLI->takeName(LI);
      NewLI->setDebugLoc(LI->getDebugLoc());
      CI->replaceAllUsesWith(NewLI);
      LI->replaceAllUsesWith(UndefValue::get(LI->getType()));
      LI->eraseFromParent();
      return NewLI;
    }
  }

  return nullptr;
}

bool genx::isGlobalStore(Instruction *I) {
  IGC_ASSERT(I);
  if (auto *SI = dyn_cast<StoreInst>(I))
    return isGlobalStore(SI);
  return false;
}

bool genx::isGlobalStore(StoreInst *ST) {
  IGC_ASSERT(ST);
  return vc::getUnderlyingGlobalVariable(ST->getPointerOperand()) != nullptr;
}

bool genx::isGlobalLoad(Instruction *I) {
  IGC_ASSERT(I);
  if (auto *LI = dyn_cast<LoadInst>(I))
    return isGlobalLoad(LI);
  return false;
}

bool genx::isGlobalLoad(LoadInst *LI) {
  IGC_ASSERT(LI);
  return vc::getUnderlyingGlobalVariable(LI->getPointerOperand()) != nullptr;
}

bool genx::isAVLoad(const Instruction *const I) {
  const auto *LI = dyn_cast_or_null<LoadInst>(I);
  return (LI && LI->isVolatile()) || GenXIntrinsic::isVLoad(I);
}

const Value *genx::getAVLoadSrcOrNull(const Instruction *const I,
                                      const Value *const CmpSrc) {
  if (genx::isAVLoad(I))
    if (const auto *Src = getBitCastedValue(I->getOperand(0));
        !CmpSrc || Src == CmpSrc)
      return Src;
  return nullptr;
}

Value *genx::getAVLoadSrcOrNull(Instruction *const I,
                                const Value *const CmpSrc) {
  return const_cast<Value *>(
      getAVLoadSrcOrNull(const_cast<const Instruction *>(I), CmpSrc));
}

bool genx::isAVLoad(const Instruction *I, const Value *const CmpSrc) {
  return getAVLoadSrcOrNull(I, CmpSrc);
}

bool genx::isAVStore(const Instruction *const I) {
  return (isa_and_nonnull<StoreInst>(I) && cast<StoreInst>(I)->isVolatile()) ||
         GenXIntrinsic::isVStore(I);
}

const Value *genx::getAVStoreDstOrNull(const Instruction *const I,
                                       const Value *const CmpDst) {
  if (genx::isAVStore(I)) {
    if (const auto *Dst = getBitCastedValue(I->getOperand(1));
        !CmpDst || Dst == CmpDst)
      return Dst;
  }
  return nullptr;
}

Value *genx::getAVStoreDstOrNull(Instruction *const I,
                                 const Value *const CmpDst) {
  return const_cast<Value *>(
      getAVStoreDstOrNull(const_cast<const Instruction *>(I), CmpDst));
}

bool genx::isAVStore(const Instruction *const I, const Value *const CmpDst) {
  return getAVStoreDstOrNull(I, CmpDst);
}

const Value *genx::getAGVLoadSrcOrNull(const Instruction *const I,
                                       const Value *const CmpGvSrc) {
  const auto *Src = getAVLoadSrcOrNull(I, CmpGvSrc);
  if (!Src)
    return nullptr;
  const auto *GV = vc::getUnderlyingGlobalVariable(Src);
  return GV && GV->hasAttribute(genx::FunctionMD::GenXVolatile) ? GV : nullptr;
}

Value *genx::getAGVLoadSrcOrNull(Instruction *const I,
                                 const Value *const CmpGvSrc) {
  return const_cast<Value *>(
      getAGVLoadSrcOrNull(const_cast<const Instruction *>(I), CmpGvSrc));
}

bool genx::isAGVLoad(const Instruction *const I, const Value *const CmpGvSrc) {
  return genx::getAGVLoadSrcOrNull(I, CmpGvSrc);
}

const Value *genx::getAGVStoreDstOrNull(const Instruction *const I,
                                        const Value *const CmpDst) {
  const auto *Dst = getAVStoreDstOrNull(I, CmpDst);
  if (!Dst)
    return nullptr;
  const auto *GV = vc::getUnderlyingGlobalVariable(Dst);
  return GV && GV->hasAttribute(genx::FunctionMD::GenXVolatile) ? GV : nullptr;
}

Value *genx::getAGVStoreDstOrNull(Instruction *const I,
                                  const Value *const CmpGvDst) {
  return const_cast<Value *>(
      getAGVStoreDstOrNull(const_cast<const Instruction *>(I), CmpGvDst));
}

bool genx::isAGVStore(const Instruction *const I, const Value *const CmpGvDst) {
  return genx::getAGVStoreDstOrNull(I, CmpGvDst);
}

bool genx::isLegalValueForGlobalStore(Value *V, Value *StorePtr) {
  // Value should be wrregion.
  auto *Wrr = dyn_cast<CallInst>(getBitCastedValue(V));
  if (!Wrr || !GenXIntrinsic::isWrRegion(Wrr))
    return false;

  // With old value obtained from load instruction with StorePtr.
  Value *OldVal =
      Wrr->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  auto *LI = dyn_cast<LoadInst>(getBitCastedValue(OldVal));
  return LI && (vc::getUnderlyingGlobalVariable(LI->getPointerOperand()) ==
                vc::getUnderlyingGlobalVariable(StorePtr));
}

bool genx::isGlobalStoreLegal(StoreInst *ST) {
  IGC_ASSERT(isGlobalStore(ST));
  return isLegalValueForGlobalStore(ST->getValueOperand(),
                                    ST->getPointerOperand());
}

// The following bale will produce identity moves.
// %a0 = load m
// %b0 = load m
// bale {
//   %a1 = rrd %a0, R
//   %b1 = wrr %b0, %a1, R
//   store %b1, m
// }
//
bool genx::isIdentityBale(const Bale &B) {
  if (!B.endsWithGStore())
    return false;

  StoreInst *ST = cast<StoreInst>(B.getHead()->Inst);
  if (B.size() == 1) {
    // The value to be stored should be a load from the same global.
    auto LI = dyn_cast<LoadInst>(ST->getOperand(0));
    return LI && vc::getUnderlyingGlobalVariable(LI->getOperand(0)) ==
                     vc::getUnderlyingGlobalVariable(ST->getOperand(1));
  }
  if (B.size() != 3)
    return false;

  CallInst *B1 = dyn_cast<CallInst>(ST->getValueOperand());
  GlobalVariable *GV = vc::getUnderlyingGlobalVariable(ST->getPointerOperand());
  if (!GenXIntrinsic::isWrRegion(B1) || !GV)
    return false;
  IGC_ASSERT(B1);
  auto B0 = dyn_cast<LoadInst>(B1->getArgOperand(0));
  if (!B0 || GV != vc::getUnderlyingGlobalVariable(B0->getPointerOperand()))
    return false;

  CallInst *A1 = dyn_cast<CallInst>(B1->getArgOperand(1));
  if (!GenXIntrinsic::isRdRegion(A1))
    return false;
  IGC_ASSERT(A1);
  LoadInst *A0 = dyn_cast<LoadInst>(A1->getArgOperand(0));
  if (!A0 || GV != vc::getUnderlyingGlobalVariable(A0->getPointerOperand()))
    return false;

  Region R1 = makeRegionFromBaleInfo(A1, BaleInfo());
  Region R2 = makeRegionFromBaleInfo(B1, BaleInfo());
  return R1 == R2;
}

// Check that region can be represented as raw operand.
bool genx::isValueRegionOKForRaw(Value *V, bool IsWrite,
                                 const GenXSubtarget *ST) {
  IGC_ASSERT(V);
  switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf:
    if (IsWrite)
      return false;
    if (GenXIntrinsic::isReadPredefReg(cast<Instruction>(V)->getOperand(
            GenXIntrinsic::GenXRegion::OldValueOperandNum)))
      return false;
    break;
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
    if (!IsWrite)
      return false;
    break;
  default:
    return false;
  }
  Region R = makeRegionFromBaleInfo(cast<Instruction>(V), BaleInfo());
  return isRegionOKForRaw(R, ST);
}

bool genx::isRegionOKForRaw(const genx::Region &R, const GenXSubtarget *ST) {
  unsigned GRFWidth = ST ? ST->getGRFByteSize() : 32;
  if (R.Indirect)
    return false;
  else if (R.Offset & (GRFWidth - 1)) // GRF boundary check
    return false;
  if (R.Width != R.NumElements)
    return false;
  if (R.Stride != 1)
    return false;
  return true;
}

bool genx::skipOptWithLargeBlock(FunctionGroup &FG) {
  for (auto fgi = FG.begin(), fge = FG.end(); fgi != fge; ++fgi) {
    auto F = *fgi;
    if (skipOptWithLargeBlock(*F))
      return true;
  }
  return false;
}

std::string genx::getInlineAsmCodes(const InlineAsm::ConstraintInfo &Info) {
  return Info.Codes.front();
}

bool genx::isInlineAsmMatchingInputConstraint(
    const InlineAsm::ConstraintInfo &Info) {
  return isdigit(Info.Codes.front()[0]);
}

genx::ConstraintType genx::getInlineAsmConstraintType(StringRef Codes) {
  return llvm::StringSwitch<genx::ConstraintType>(Codes)
      .Case("r", ConstraintType::Constraint_r)
      .Case("rw", ConstraintType::Constraint_rw)
      .Case("i", ConstraintType::Constraint_i)
      .Case("n", ConstraintType::Constraint_n)
      .Case("F", ConstraintType::Constraint_F)
      .Case("cr", ConstraintType::Constraint_cr)
      .Case("a", ConstraintType::Constraint_a)
      .Default(ConstraintType::Constraint_unknown);
}

unsigned
genx::getInlineAsmMatchedOperand(const InlineAsm::ConstraintInfo &Info) {
  IGC_ASSERT_MESSAGE(genx::isInlineAsmMatchingInputConstraint(Info),
                     "Matching input expected");
  int OperandValue = std::stoi(Info.Codes.front());
  IGC_ASSERT(OperandValue >= 0);
  return OperandValue;
}

std::vector<GenXInlineAsmInfo> genx::getGenXInlineAsmInfo(MDNode *MD) {
  std::vector<GenXInlineAsmInfo> Result;
  for (auto &MDOp : MD->operands()) {
    auto EntryMD = dyn_cast<MDTuple>(MDOp);
    IGC_ASSERT_MESSAGE(EntryMD, "error setting metadata for inline asm");
    IGC_ASSERT_MESSAGE(EntryMD->getNumOperands() == 3,
                       "error setting metadata for inline asm");
    ConstantAsMetadata *Op0 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(0));
    ConstantAsMetadata *Op1 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(1));
    ConstantAsMetadata *Op2 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(2));
    IGC_ASSERT_MESSAGE(Op0, "error setting metadata for inline asm");
    IGC_ASSERT_MESSAGE(Op1, "error setting metadata for inline asm");
    IGC_ASSERT_MESSAGE(Op2, "error setting metadata for inline asm");
    auto CTy = static_cast<genx::ConstraintType>(
        cast<ConstantInt>(Op0->getValue())->getZExtValue());
    Result.emplace_back(CTy, cast<ConstantInt>(Op1->getValue())->getSExtValue(),
                        cast<ConstantInt>(Op2->getValue())->getZExtValue());
  }
  return Result;
}

std::vector<GenXInlineAsmInfo> genx::getGenXInlineAsmInfo(CallInst *CI) {
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");
  MDNode *MD = CI->getMetadata(genx::MD_genx_inline_asm_info);
  // empty constraint info
  if (!MD) {
    auto *IA = cast<InlineAsm>(IGCLLVM::getCalledValue(CI));
    IGC_ASSERT_MESSAGE(IA->getConstraintString().empty(),
                       "No info only for empty constraint string");
    (void)IA;
    return std::vector<GenXInlineAsmInfo>();
  }
  return genx::getGenXInlineAsmInfo(MD);
}

bool genx::hasConstraintOfType(
    const std::vector<GenXInlineAsmInfo> &ConstraintsInfo,
    genx::ConstraintType CTy) {
  return llvm::any_of(ConstraintsInfo, [&](const GenXInlineAsmInfo &Info) {
    return Info.getConstraintType() == CTy;
  });
}

unsigned genx::getInlineAsmNumOutputs(CallInst *CI) {
  IGC_ASSERT_MESSAGE(CI->isInlineAsm(), "Inline asm expected");
  unsigned NumOutputs;
  if (CI->getType()->isVoidTy())
    NumOutputs = 0;
  else if (auto ST = dyn_cast<StructType>(CI->getType()))
    NumOutputs = ST->getNumElements();
  else
    NumOutputs = 1;
  return NumOutputs;
}

/* for <1 x Ty> returns Ty
 * for Ty returns <1 x Ty>
 * other cases are unsupported
 */
Type *genx::getCorrespondingVectorOrScalar(Type *Ty) {
  if (Ty->isVectorTy()) {
    IGC_ASSERT_MESSAGE(
        cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() == 1,
        "wrong argument: scalar or degenerate vector is expected");
    return Ty->getScalarType();
  }
  return IGCLLVM::FixedVectorType::get(Ty, 1);
}

/***********************************************************************
 * getExecSizeAllowedBits : get bitmap of allowed execution sizes
 *
 * Enter:   Inst = main instruction of bale
 *
 * Return:  bit N set if execution size 1<<N is allowed
 *
 * Most instructions have a minimum width of 1. But some instructions,
 * such as dp4 and lrp, have a minimum width of 4, and legalization cannot
 * allow such an instruction to be split to a smaller width.
 */
unsigned genx::getExecSizeAllowedBits(const Instruction *Inst,
                                      const GenXSubtarget *ST) {
  IGC_ASSERT(Inst);
  IGC_ASSERT(ST);
  switch (Inst->getOpcode()) {
  default:
    break;
  case BinaryOperator::SDiv:
  case BinaryOperator::UDiv:
  case BinaryOperator::SRem:
  case BinaryOperator::URem:
    // If integer division IS supported.
    //   Set maximum SIMD width to 16:
    //      Recent HW does not support SIMD16/SIMD32 division, however,
    //      finalizer splits such SIMD16 operations and we piggy-back
    //      on this behavior.
    // If integer division IS NOT supported.
    //   The expectation is for GenXEmulate pass to replace such operations
    //   with emulation routines (which has no restriction on SIMD width)
    return ST->hasIntDivRem32() ? 0x1f : 0x3f;
  }

  unsigned ID = vc::getAnyIntrinsicID(Inst);
  switch (ID) {
  case GenXIntrinsic::genx_ssmad:
  case GenXIntrinsic::genx_sumad:
  case GenXIntrinsic::genx_usmad:
  case GenXIntrinsic::genx_uumad:
  case GenXIntrinsic::genx_ssmad_sat:
  case GenXIntrinsic::genx_sumad_sat:
  case GenXIntrinsic::genx_usmad_sat:
  case GenXIntrinsic::genx_uumad_sat:
  case Intrinsic::fma:
    // Do not emit simd32 mad for pre-ICLLP.
    return ST->hasMadSimd32() ? 0x3f : 0x1f;
  default:
    return vc::isAnyNonTrivialIntrinsic(ID)
               ? GenXIntrinsicInfo(ID).getExecSizeAllowedBits()
               : 0x3f;
  }
}

bool genx::isSupportedFloatingPointType(const Type *Ty) {
  IGC_ASSERT(Ty);
  auto *ScalarTy = Ty->getScalarType();
  return ScalarTy->isFloatTy() || ScalarTy->isHalfTy() ||
         ScalarTy->isBFloatTy() || ScalarTy->isDoubleTy();
}

// Get type that represents OldType as vector of NewScalarType, e.g.
// <4 x i16> -> <2 x i32>, returns nullptr if it's impossible.
IGCLLVM::FixedVectorType *genx::changeVectorType(Type *OldType,
                                                 Type *NewScalarType,
                                                 const DataLayout *DL) {
  IGC_ASSERT(DL && OldType && NewScalarType);
  IGC_ASSERT(!NewScalarType->isVectorTy());
  IGC_ASSERT(isSupportedFloatingPointType(NewScalarType) ||
             NewScalarType->isIntegerTy() || NewScalarType->isPointerTy());
  auto OldTypeSize = vc::getTypeSize(OldType, DL).inBits();
  auto NewScalarTypeSize = vc::getTypeSize(NewScalarType, DL).inBits();
  if (OldTypeSize % NewScalarTypeSize)
    return nullptr;
  return IGCLLVM::FixedVectorType::get(NewScalarType,
                                       OldTypeSize / NewScalarTypeSize);
}

// Check if V is reading form predfined register.
bool genx::isPredefRegSource(const Value *V) {
  if (GenXIntrinsic::isRdRegion(V))
    return GenXIntrinsic::isReadPredefReg(cast<Instruction>(V)->getOperand(
        GenXIntrinsic::GenXRegion::OldValueOperandNum));
  return GenXIntrinsic::isReadPredefReg(V);
}

// Check if V is writing to predefined register.
bool genx::isPredefRegDestination(const Value *V) {
  if (GenXIntrinsic::isWrRegion(V))
    return std::any_of(V->user_begin(), V->user_end(), [](auto *U) {
      return GenXIntrinsic::isWritePredefReg(U);
    });
  return GenXIntrinsic::isWritePredefReg(V);
}

// info is at main template function
CastInst *genx::scalarizeOrVectorizeIfNeeded(Instruction *Inst,
                                             Instruction *InstToReplace) {
  SmallVector<Type *, 1> Types = {InstToReplace->getType()};
  return scalarizeOrVectorizeIfNeeded(Inst, Types.begin(), Types.end());
}

Function *genx::getFunctionPointerFunc(Value *V) {
  Instruction *I = nullptr;
  for (; (I = dyn_cast<CastInst>(V)); V = I->getOperand(0))
    ;
  ConstantExpr *CE = nullptr;
  for (; (CE = dyn_cast<ConstantExpr>(V)) &&
         (CE->getOpcode() == Instruction::ExtractElement || CE->isCast());
       V = CE->getOperand(0))
    ;
  if (auto *F = dyn_cast<Function>(V))
    return F;
  if (auto *CV = dyn_cast<ConstantVector>(V); CV && CV->getSplatValue())
    return getFunctionPointerFunc(CV->getSplatValue());
  return nullptr;
}

bool genx::isFuncPointerVec(Value *V) {
  bool Res = false;
  if (V->getType()->isVectorTy() && isa<ConstantExpr>(V) &&
      cast<ConstantExpr>(V)->getOpcode() == Instruction::BitCast)
    Res = isFuncPointerVec(cast<ConstantExpr>(V)->getOperand(0));
  else if (ConstantVector *Vec = dyn_cast<ConstantVector>(V))
    Res = std::all_of(Vec->op_begin(), Vec->op_end(), [](Value *V) {
      return getFunctionPointerFunc(V) != nullptr;
    });
  return Res;
}

unsigned genx::getLogAlignment(VISA_Align Align, unsigned GRFWidth) {
  switch (Align) {
  case ALIGN_BYTE:
    return Log2_32(ByteBytes);
  case ALIGN_WORD:
    return Log2_32(WordBytes);
  case ALIGN_DWORD:
    return Log2_32(DWordBytes);
  case ALIGN_QWORD:
    return Log2_32(QWordBytes);
  case ALIGN_OWORD:
    return Log2_32(OWordBytes);
  case ALIGN_GRF:
    return Log2_32(GRFWidth);
  case ALIGN_2_GRF:
    IGC_ASSERT_EXIT(GRFWidth > 0);
    return Log2_32(GRFWidth) + 1;
  default:
    report_fatal_error("Unknown alignment");
  }
}

VISA_Align genx::getVISA_Align(unsigned LogAlignment, unsigned GRFWidth) {
  if (LogAlignment == Log2_32(ByteBytes))
    return ALIGN_BYTE;
  if (LogAlignment == Log2_32(WordBytes))
    return ALIGN_WORD;
  if (LogAlignment == Log2_32(DWordBytes))
    return ALIGN_DWORD;
  if (LogAlignment == Log2_32(QWordBytes))
    return ALIGN_QWORD;
  if (LogAlignment == Log2_32(OWordBytes))
    return ALIGN_OWORD;
  if (GRFWidth > 0) {
    if (LogAlignment == Log2_32(GRFWidth))
      return ALIGN_GRF;
    if (LogAlignment == Log2_32(GRFWidth) + 1)
      return ALIGN_2_GRF;
  }
  report_fatal_error("Unknown log alignment");
}

unsigned genx::ceilLogAlignment(unsigned LogAlignment, unsigned GRFWidth) {
  if (LogAlignment <= Log2_32(ByteBytes))
    return Log2_32(ByteBytes);
  if (LogAlignment <= Log2_32(WordBytes))
    return Log2_32(WordBytes);
  if (LogAlignment <= Log2_32(DWordBytes))
    return Log2_32(DWordBytes);
  if (LogAlignment <= Log2_32(QWordBytes))
    return Log2_32(QWordBytes);
  if (LogAlignment <= Log2_32(OWordBytes))
    return Log2_32(OWordBytes);
  if (GRFWidth > 0) {
    if (LogAlignment <= Log2_32(GRFWidth))
      return Log2_32(GRFWidth);
    if (LogAlignment <= Log2_32(GRFWidth) + 1)
      return Log2_32(GRFWidth) + 1;
  }
  report_fatal_error("Unknown log alignment");
}

bool genx::isWrPredRegionLegalSetP(const CallInst &WrPredRegion) {
  IGC_ASSERT_MESSAGE(GenXIntrinsic::getGenXIntrinsicID(&WrPredRegion) ==
                         GenXIntrinsic::genx_wrpredregion,
                     "wrong argument: wrpredregion intrinsic was expected");
  auto &NewValue = *WrPredRegion.getOperand(vc::WrPredRegionOperand::NewValue);
  auto ExecSize =
      NewValue.getType()->isVectorTy()
          ? cast<IGCLLVM::FixedVectorType>(NewValue.getType())->getNumElements()
          : 1;
  auto Offset = cast<ConstantInt>(
                    WrPredRegion.getOperand(vc::WrPredRegionOperand::Offset))
                    ->getZExtValue();
  if (ExecSize > 32 || !isPowerOf2_64(ExecSize))
    return false;
  if (ExecSize == 32)
    return Offset == 0;
  return Offset == 0 || Offset == 16;
}

const CallInst *genx::checkFunctionCall(const Value *V, const Function *F) {
  if (!V || !F)
    return nullptr;
  const auto *CI = dyn_cast<CallInst>(V);
  if (CI && CI->getCalledFunction() == F)
    return CI;
  return nullptr;
}

CallInst *genx::checkFunctionCall(Value *V, const Function *F) {
  return const_cast<CallInst *>(
      genx::checkFunctionCall(static_cast<const Value *>(V), F));
}

unsigned genx::getNumGRFsPerIndirectForRegion(const genx::Region &R,
                                              const GenXSubtarget *ST,
                                              bool Allow2D) {
  IGC_ASSERT_MESSAGE(R.Indirect, "Indirect region expected");
  IGC_ASSERT(ST);
  if (ST->hasIndirectGRFCrossing() &&
      (R.ElementBytes != genx::ByteBytes || ST->hasIndirectByteGRFCrossing()) &&
      // SKL+. See if we can allow GRF crossing.
      (Allow2D || !R.is2D())) {
    return 2;
  }
  return 1;
}

std::size_t genx::getStructElementPaddedSize(unsigned ElemIdx,
                                             unsigned NumOperands,
                                             const StructLayout &Layout) {
  IGC_ASSERT_MESSAGE(ElemIdx < NumOperands,
                     "wrong argument: invalid index into a struct");
  if (ElemIdx == NumOperands - 1)
    return Layout.getSizeInBytes() - Layout.getElementOffset(ElemIdx);
  return Layout.getElementOffset(ElemIdx + 1) -
         Layout.getElementOffset(ElemIdx);
}

// splitStructPhi : split a phi node with struct type by splitting into
//                  struct elements
bool genx::splitStructPhi(PHINode *Phi) {
  StructType *Ty = cast<StructType>(Phi->getType());
  // Find where we need to insert the combine instructions.
  Instruction *CombineInsertBefore = Phi->getParent()->getFirstNonPHI();
  // Now split the phi.
  Value *Combined = UndefValue::get(Ty);
  // For each struct element...
  for (unsigned Idx = 0, e = Ty->getNumElements(); Idx != e; ++Idx) {
    Type *ElTy = Ty->getTypeAtIndex(Idx);
    // Create the new phi node.
    PHINode *NewPhi =
        PHINode::Create(ElTy, Phi->getNumIncomingValues(),
                        Phi->getName() + ".element" + Twine(Idx), Phi);
    NewPhi->setDebugLoc(Phi->getDebugLoc());
    // Combine the new phi.
    Instruction *Combine = InsertValueInst::Create(
        Combined, NewPhi, Idx, NewPhi->getName(), CombineInsertBefore);
    Combine->setDebugLoc(Phi->getDebugLoc());
    Combined = Combine;
    // For each incoming...
    for (unsigned In = 0, InEnd = Phi->getNumIncomingValues(); In != InEnd;
         ++In) {
      // Create an extractelement to get the individual element value.
      // This needs to go before the terminator of the incoming block.
      BasicBlock *IncomingBB = Phi->getIncomingBlock(In);
      Value *Incoming = Phi->getIncomingValue(In);
      Instruction *Extract = ExtractValueInst::Create(
          Incoming, Idx, Phi->getName() + ".element" + Twine(Idx),
          IncomingBB->getTerminator());
      Extract->setDebugLoc(Phi->getDebugLoc());
      // Add as an incoming of the new phi node.
      NewPhi->addIncoming(Extract, IncomingBB);
    }
  }
  Phi->replaceAllUsesWith(Combined);
  Phi->eraseFromParent();
  return true;
}

bool genx::splitStructPhis(Function *F) {
  bool Modified = false;
  for (Function::iterator fi = F->begin(), fe = F->end(); fi != fe; ++fi) {
    BasicBlock *BB = &*fi;
    for (BasicBlock::iterator bi = BB->begin();;) {
      PHINode *Phi = dyn_cast<PHINode>(&*bi);
      if (!Phi)
        break;
      ++bi; // increment here as splitStructPhi removes old phi node
      if (isa<StructType>(Phi->getType()))
        Modified |= splitStructPhi(Phi);
    }
  }
  return Modified;
}

bool genx::isRdRWithOldValueVLoadSrc(Value *V) {
  if (!GenXIntrinsic::isRdRegion(V))
    return false;
  auto *RdR = cast<CallInst>(V);
  auto *I = dyn_cast<Instruction>(
      RdR->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
  return I && isAVLoad(I);
};

bool genx::isWrRWithOldValueVLoadSrc(Value *V) {
  if (!GenXIntrinsic::isWrRegion(V))
    return false;
  auto *WrR = cast<CallInst>(V);
  auto *I = dyn_cast<Instruction>(
      WrR->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
  return I && isAVLoad(I);
};

const Instruction *genx::getAVLoadKillOrNull(
    const Instruction *FromVLoad, const Instruction *ToPosI,
    const bool ChangeSearchDirectionBasedOnDominance,
    bool PosProvedReachableFromVLoad, const DominatorTree *const DT,
    const SmallPtrSet<const BasicBlock *, 2> *const ExcludeBlocksOnCfgTraversal,
    const llvm::SmallVector<const Instruction *, 8> *const KillCallSites) {

  const auto *VLoadSrc = genx::getAVLoadSrcOrNull(FromVLoad);

  IGC_ASSERT(VLoadSrc);
  IGC_ASSERT(FromVLoad->getFunction() == ToPosI->getFunction());

  auto isAKill = [VLoadSrc, KillCallSites](const Instruction &I) {
    if (isAVStore(&I, VLoadSrc))
      return true;

    const auto *Call = dyn_cast<CallInst>(&I);
    if (!Call)
      return false;

    const bool IsAnIntrinsic =
        vc::isAnyNonTrivialIntrinsic(vc::getAnyIntrinsicID(Call));

    if (LLVM_LIKELY(!KillCallSites))
      return !IsAnIntrinsic;

    const bool KillCallSiteMatch =
        llvm::find(*KillCallSites, Call) != KillCallSites->end();
    const auto *CalledFunction = Call->getCalledFunction();
    const bool IsAnIndirectCall = !CalledFunction;
    const bool IsDeclaration =
        CalledFunction && CalledFunction->isDeclaration();
    return KillCallSiteMatch || IsAnIndirectCall ||
           (!IsAnIntrinsic && IsDeclaration);
  };

  if (DT && ChangeSearchDirectionBasedOnDominance &&
      DT->dominates(ToPosI, FromVLoad))
    std::swap(ToPosI, FromVLoad);

  const auto *VLoadBB = FromVLoad->getParent();
  const auto *PosBB = ToPosI->getParent();

  if (VLoadBB == PosBB) {
    if (auto KII = std::find_if(FromVLoad->getIterator(), ToPosI->getIterator(),
                                isAKill);
        KII != ToPosI->getIterator())
      return &*KII;
    return nullptr;
  }

  llvm::SmallPtrSet<const BasicBlock *, 16> Visited = {VLoadBB};
  llvm::SmallVector<const BasicBlock *, 16> BBs;

  if (!PosProvedReachableFromVLoad) {
    PosProvedReachableFromVLoad =
        llvm::find(FromVLoad->users(), ToPosI) != FromVLoad->users().end();

    if (!PosProvedReachableFromVLoad && DT)
      PosProvedReachableFromVLoad = DT->dominates(FromVLoad, ToPosI);

    if (!PosProvedReachableFromVLoad) {
      std::copy(succ_begin(VLoadBB), succ_end(VLoadBB),
                std::back_inserter(BBs));

      while (!PosProvedReachableFromVLoad && !BBs.empty()) {
        const auto *BB = BBs.pop_back_val();
        Visited.insert(BB);
        if (BB == PosBB)
          PosProvedReachableFromVLoad = true;
        else
          std::copy_if(
              succ_begin(BB), succ_end(BB), std::back_inserter(BBs),
              [&](const BasicBlock *BB) { return !Visited.count(BB); });
      }

      if (!PosProvedReachableFromVLoad)
        return nullptr;

      BBs.clear();
    }
  }

  if (const auto KII =
          std::find_if(PosBB->begin(), ToPosI->getIterator(), isAKill);
      KII != ToPosI->getIterator())
    return &*KII;

  if (const auto KII =
          std::find_if(FromVLoad->getIterator(), VLoadBB->end(), isAKill);
      KII != VLoadBB->end())
    return &*KII;

  Visited = {VLoadBB};
  if (ExcludeBlocksOnCfgTraversal)
    Visited.insert(ExcludeBlocksOnCfgTraversal->begin(),
                   ExcludeBlocksOnCfgTraversal->end());
  std::copy_if(pred_begin(PosBB), pred_end(PosBB), std::back_inserter(BBs),
               [&](const BasicBlock *BB) { return !Visited.count(BB); });

  while (!BBs.empty()) {
    const auto *BB = BBs.pop_back_val();

    if (const auto KII = std::find_if(BB->begin(), BB->end(), isAKill);
        KII != BB->end())
      return &*KII;

    Visited.insert(BB);
    std::copy_if(pred_begin(BB), pred_end(BB), std::back_inserter(BBs),
                 [&](const BasicBlock *BB) { return !Visited.count(BB); });
  }

  return nullptr;
}

namespace {
template <
    class T,
    std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, Instruction>, int> = 0>
inline llvm::SmallPtrSet<T *, 1> genx_getSrcVLoads__impl(T *I) {
  IGC_ASSERT(I);
  I = dyn_cast<T>(genx::getBitCastedValue(I));
  llvm::SmallPtrSet<T *, 1> Res;
  if (!I)
    return Res;
  for (const auto &Opnd : I->operands())
    if (auto *OpndSrc =
            dyn_cast<Instruction>(genx::getBitCastedValue(Opnd.get())))
      if (genx::isAVLoad(OpndSrc))
        Res.insert(OpndSrc);
  return Res;
};
}; // namespace

llvm::SmallPtrSet<Instruction *, 1> genx::getSrcVLoads(Instruction *I) {
  return ::genx_getSrcVLoads__impl(I);
};

llvm::SmallPtrSet<const Instruction *, 1>
genx::getSrcVLoads(const Instruction *I) {
  return ::genx_getSrcVLoads__impl(I);
};

Instruction *genx::getSrcVLoadOrNull(Instruction *const I) {
  const auto Res = getSrcVLoads(I);
  return Res.size() ? *Res.begin() : nullptr;
}

const Instruction *genx::getSrcVLoadOrNull(const Instruction *const I) {
  const auto Res = getSrcVLoads(I);
  return Res.size() ? *Res.begin() : nullptr;
}

bool genx::hasVLoadSource(const Instruction *const I) {
  return getSrcVLoadOrNull(I);
}

bool genx::isSafeToSink_CheckAVLoadKill(const Bale &B,
                                        const Instruction *const To,
                                        const DominatorTree *const DT) {
  IGC_ASSERT(!DT || DT->dominates(B.getHead()->Inst, To));
  for (auto *LI : B.getVLoadSources()) {
    if (getAVLoadKillOrNull(LI, To))
      return false;
  }
  return true;
}

bool genx::isSafeToSink_CheckAVLoadKill(const Instruction *const I,
                                        const Instruction *const To,
                                        const GenXBaling *const Baling,
                                        const DominatorTree *const DT) {
  IGC_ASSERT(I && To && Baling);
  Bale Bale;
  Baling->buildBale(
      const_cast<Instruction *>(
          I), // The baling API is mostly used in non-const context, so it has
              // no const-ok overloads.
      &Bale); // TBD!: recheck IncludeAddr=false here, we may want to take
              // address calculation into consideration when sinking, since
              // we'll most likely move the bale along with address calculation
              // (e.g. GenXUnbaling).
  return isSafeToSink_CheckAVLoadKill(Bale, To, DT);
}

bool genx::isSafeToUse_CheckAVLoadKillOrForbiddenUser(
    const Instruction *const UseSrc, const Instruction *const UseTarget,
    const DominatorTree *const DT) {
  IGC_ASSERT(UseSrc != UseTarget);
  IGC_ASSERT(DT->dominates(UseSrc, UseTarget));
  if (isAVLoad(UseSrc)) {
    if (!isAGVLoad(UseSrc)) // Non genx_volatile-related genx.vloads are allowed
                            // to have any users.
      return true;
    return !isAGVLoadForbiddenUser(UseTarget) &&
           !getAVLoadKillOrNull(UseSrc, UseTarget, false, false);
  }
  return true;
}

bool genx::isSafeToMove_CheckAVLoadKill(const Instruction *const I,
                                        const Instruction *const To,
                                        const DominatorTree *const DT) {
  IGC_ASSERT(I != To);
  if (LLVM_UNLIKELY(genx::isAVLoad(I)))
    return !getAVLoadKillOrNull(I, To, true, false, DT);
  for (auto *LI : getSrcVLoads(I)) {
    const bool LIDomTo = !DT || DT->dominates(LI, To);
    if (!LIDomTo || getAVLoadKillOrNull(LI, To))
      return false;
  }
  return true;
}

bool genx::isSafeToReplace_CheckAVLoadKillOrForbiddenUser(
    const Instruction *const OldI, const Instruction *const NewI,
    const DominatorTree *const DT) {
  IGC_ASSERT_MESSAGE(OldI->getParent() && NewI->getParent(),
                     "both instructions must be placed in IR.");
  return isSafeToMove_CheckAVLoadKill(NewI, OldI, DT) &&
         (!genx::isAVLoad(NewI) ||
          llvm::all_of(OldI->users(), [NewI, DT, IsAGVLoad = isAGVLoad(NewI)](
                                          const User *const U) {
            const auto *const UserInst = dyn_cast<Instruction>(U);
            return UserInst && (!IsAGVLoad || !isAGVLoadForbiddenUser(U)) &&
                   !getAVLoadKillOrNull(NewI, UserInst);
          }));
}

bool genx::legalizeGVLoadForbiddenUsers(Instruction *GVLoad) {
  IGC_ASSERT(isAGVLoad(GVLoad));
  bool Changed = false;
  Instruction *Rdr = nullptr;
  for (auto UserI = GVLoad->user_begin(), E = GVLoad->user_end(); UserI != E;) {
    auto *GVLoadUser = *UserI++;
    if (!genx::isAGVLoadForbiddenUser(GVLoadUser))
      continue;
    if (!Rdr) {
      const auto *GVLoadMaybeVT =
          dyn_cast<IGCLLVM::FixedVectorType>(GVLoad->getType());
      const unsigned ElsCount =
          GVLoadMaybeVT ? GVLoadMaybeVT->getNumElements() : 1;
      genx::Region R(GVLoad);
      R.getSubregion(0, ElsCount);
      Rdr =
          R.createRdRegion(GVLoad, GVLoad->getName() + "._gvload_legalized_rdr",
                           GVLoad->getNextNode(), GVLoad->getDebugLoc(), false);
    }
    IGC_ASSERT(Rdr);
#ifndef NDEBUG
    vc::diagnose(GVLoad->getContext(), "genx::legalizeGVLoadForbiddenUsers: ",
                 "illegal genx.vload user found, fixing it by inserting "
                 "rdregion in between.",
                 DS_Warning, vc::WarningName::Generic, GVLoad);
#endif
    IGCLLVM::replaceUsesWithIf(GVLoad, Rdr, [Rdr, GVLoadUser](Use &U) {
      return U.getUser() != Rdr && U.getUser() == GVLoadUser;
    });
    Changed |= true;
  }
  return Changed;
}

bool genx::vloadsReadSameValue(const Instruction *L1, const Instruction *L2,
                               const DominatorTree *const DT) {
  IGC_ASSERT_MESSAGE(
      genx::isAVLoad(L1) && genx::isAVLoad(L2),
      "L1 and L2 are expected to be genx.vload or a load volatile");
  IGC_ASSERT_MESSAGE(L1 && L2 && DT, "L1, L2 and DT must not be null.");
  if (genx::getAVLoadSrcOrNull(L2, genx::getAVLoadSrcOrNull(L1))) {
    if (DT->dominates(L2, L1))
      std::swap(L2, L1);
    if (DT->dominates(L1, L2))
      return !genx::getAVLoadKillOrNull(L1, L2, false, true);
  }
  return false;
}

bool genx::isBitwiseIdentical(const Value *V1, const Value *V2,
                              const DominatorTree *const DT) {
  IGC_ASSERT_MESSAGE(V1 && V2, "values must not be null");

  V1 = genx::getBitCastedValue(V1);
  V2 = genx::getBitCastedValue(V2);
  if (V1 == V2)
    return true;

  if (DT) {
    auto *L1 = dyn_cast<Instruction>(V1);
    auto *L2 = dyn_cast<Instruction>(V2);
    if (genx::isAVLoad(L1) && genx::isAVLoad(L2))
      return genx::vloadsReadSameValue(L1, L2, DT);
  }

  return false;
}
