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
// Utility functions for the GenX backend.
//
//===----------------------------------------------------------------------===//
#include "GenXUtil.h"
#include "FunctionGroup.h"
#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXRegion.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"

#include <iterator>

using namespace llvm;
using namespace genx;

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
    Instruction *InsertBefore, Module *M)
{
  if (!M)
    M = InsertBefore->getParent()->getParent()->getParent();
  Function *Decl = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_convert,
      In->getType());
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
    Instruction *InsertBefore, Module *M)
{
  if (!M)
    M = InsertBefore->getParent()->getParent()->getParent();
  auto OffsetVal = ConstantInt::get(In->getType()->getScalarType(), Offset);
  Function *Decl = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_convert_addr,
      In->getType());
  Value *Args[] = { In, OffsetVal };
  return CallInst::Create(Decl, Args, Name, InsertBefore);
}

/***********************************************************************
 * createAddAddr : create a genx_add_addr intrinsic call
 *
 * InsertBefore can be 0 so the new instruction is not inserted anywhere,
 * but in that case M must be non-0 and set to the Module.
 */
CallInst *genx::createAddAddr(Value *Lhs, Value *Rhs, const Twine &Name,
    Instruction *InsertBefore, Module *M)
{
  if (!M)
    M = InsertBefore->getParent()->getParent()->getParent();
  Value *Args[] = {Lhs, Rhs};
  Type *Tys[] = {Rhs->getType(), Lhs->getType()};
  Function *Decl = GenXIntrinsic::getGenXDeclaration(M, GenXIntrinsic::genx_add_addr, Tys);
  return CallInst::Create(Decl, Args, Name, InsertBefore);
}

/***********************************************************************
 * createUnifiedRet : create a dummy instruction that produces dummy
 * unified return value.
 *
 * %Name.unifiedret = call Ty @llvm.ssa_copy(Ty undef)
 */
CallInst *genx::createUnifiedRet(Type *Ty, const Twine &Name, Module *M) {
  assert(Ty && M && "wrong arguments");
  auto G = Intrinsic::getDeclaration(M, Intrinsic::ssa_copy, Ty);
  return CallInst::Create(G, UndefValue::get(Ty), Name + ".unifiedret",
                          static_cast<Instruction *>(nullptr));
}

/***********************************************************************
 * getPredicateConstantAsInt : get an i1 or vXi1 constant's value as a single integer
 */
unsigned genx::getPredicateConstantAsInt(Constant *C)
{
  if (auto CI = dyn_cast<ConstantInt>(C))
    return CI->getZExtValue(); // scalar
  unsigned Bits = 0;
  unsigned NumElements = cast<VectorType>(C->getType())->getNumElements();
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
Constant *genx::getConstantSubvector(Constant *V,
    unsigned StartIdx, unsigned Size)
{
  Type *ElTy = cast<VectorType>(V->getType())->getElementType();
  Type *RegionTy = VectorType::get(ElTy, Size);
  if (isa<UndefValue>(V))
    V = UndefValue::get(RegionTy);
  else if (isa<ConstantAggregateZero>(V))
    V = ConstantAggregateZero::get(RegionTy);
  else {
    SmallVector<Constant *, 32> Val;
    for (unsigned i = 0; i != Size; ++i)
      Val.push_back(V->getAggregateElement(i + StartIdx));
    V = ConstantVector::get(Val);
  }
  return V;
}

/***********************************************************************
 * concatConstants : concatenate two possibly vector constants, giving a
 *      vector constant
 */
Constant *genx::concatConstants(Constant *C1, Constant *C2)
{
  assert(C1->getType()->getScalarType() == C2->getType()->getScalarType());
  Constant *CC[] = { C1, C2 };
  SmallVector<Constant *, 8> Vec;
  bool AllUndef = true;
  for (unsigned Idx = 0; Idx != 2; ++Idx) {
    Constant *C = CC[Idx];
    if (auto VT = dyn_cast<VectorType>(C->getType())) {
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
 * findClosestCommonDominator : find closest common dominator of some instructions
 *
 * Enter:   DT = dominator tree
 *          Insts = the instructions
 *
 * Return:  The one instruction that dominates all the others, if any.
 *          Otherwise the terminator of the closest common dominating basic
 *          block.
 */
Instruction *genx::findClosestCommonDominator(DominatorTree *DT,
    ArrayRef<Instruction *> Insts)
{
  assert(!Insts.empty());
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
 * of the two address operand if any, or -1 if not.
 */
int genx::getTwoAddressOperandNum(CallInst *CI)
{
  auto IntrinsicID = GenXIntrinsic::getAnyIntrinsicID(CI);
  if (IntrinsicID == GenXIntrinsic::not_any_intrinsic)
    return -1; // not intrinsic
  if (GenXIntrinsic::isWrRegion(IntrinsicID) ||
      IntrinsicID == GenXIntrinsic::genx_wrpredregion ||
      IntrinsicID == GenXIntrinsic::genx_wrpredpredregion)
    return 0; // wr(pred(pred))region has operand 0 as two address operand
  if (CI->getType()->isVoidTy())
    return -1; // no return value
  GenXIntrinsicInfo II(IntrinsicID);
  unsigned Num = CI->getNumArgOperands();
  if (!Num)
    return -1; // no args
  --Num; // Num = last arg number, could be two address operand
  if (isa<UndefValue>(CI->getOperand(Num)))
    return -1; // operand is undef, must be RAW_NULLALLOWED
  if (II.getArgInfo(Num).getCategory() != GenXIntrinsicInfo::TWOADDR)
    return -1; // not two addr operand
  if (CI->use_empty() && II.getRetInfo().rawNullAllowed())
    return -1; // unused result will be V0
  return Num; // it is two addr
}

/***********************************************************************
 * isNot : test whether an instruction is a "not" instruction (an xor with
 *    constant all ones)
 */
bool genx::isNot(Instruction *Inst)
{
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
bool genx::isPredNot(Instruction *Inst)
{
  if (Inst->getOpcode() == Instruction::Xor)
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1)))
      if (C->isAllOnesValue() && C->getType()->getScalarType()->isIntegerTy(1))
        return true;
  return false;
}

/***********************************************************************
 * isIntNot : test whether an instruction is a "not" instruction (an xor
 *    with constant all ones) with non-predicate type
 */
bool genx::isIntNot(Instruction *Inst)
{
  if (Inst->getOpcode() == Instruction::Xor)
    if (auto C = dyn_cast<Constant>(Inst->getOperand(1)))
      if (C->isAllOnesValue() && !C->getType()->getScalarType()->isIntegerTy(1))
        return true;
  return false;
}

/***********************************************************************
 * ShuffleVectorAnalyzer::getAsSlice : see if the shufflevector is a slice on
 *    operand 0, and if so return the start index, or -1 if it is not a slice
 */
int ShuffleVectorAnalyzer::getAsSlice()
{
  unsigned WholeWidth = SI->getOperand(0)->getType()->getVectorNumElements();
  Constant *Selector = cast<Constant>(SI->getOperand(2));
  unsigned Width = SI->getType()->getVectorNumElements();
  unsigned StartIdx = cast<ConstantInt>(
      Selector->getAggregateElement((unsigned)0))->getZExtValue();
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
  if (MaxIndex >= SI->getOperand(0)->getType()->getVectorNumElements())
    return false;

  // Find first non-one difference.
  auto SliceEnd =
      std::adjacent_find(Begin, End,
                         [](int Prev, int Next) { return Next - Prev != 1; });
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

// Based on the value of a shufflevector mask element defines in which of
// 2 operands it points. The operand is returned.
static Value *getOperandByMaskValue(const ShuffleVectorInst &SI,
                                    int MaskValue) {
  assert(MaskValue >= 0 && "invalid index");
  int FirstOpSize = SI.getOperand(0)->getType()->getVectorNumElements();
  if (MaskValue < FirstOpSize)
    return SI.getOperand(0);
  else {
    int SecondOpSize = SI.getOperand(1)->getType()->getVectorNumElements();
    assert(MaskValue < FirstOpSize + SecondOpSize && "invalid index");
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
    auto FirstOpSize = SI.getOperand(0)->getType()->getVectorNumElements();
    Init.R.Offset = (MaskVal - FirstOpSize) * Init.R.ElementBytes;
  }
  return Init;
}

// Takes shufflevector mask indexes from [\p FirstIt, \p LastIt),
// converts them to the indexes of \p Operand of \p SI instruction
// and writes them to \p OutIt.
// Invalid indexes become negative numbers.
template <typename ForwardIter, typename OutputIter>
void makeSVIIndexesOperandIndexes(const ShuffleVectorInst &SI,
                                  const Value &Operand, ForwardIter FirstIt,
                                  ForwardIter LastIt, OutputIter OutIt) {
  int FirstOpSize = SI.getOperand(0)->getType()->getVectorNumElements();
  if (&Operand == SI.getOperand(0)) {
    std::transform(FirstIt, LastIt, OutIt, [FirstOpSize](int MaskVal) {
      if (MaskVal >= FirstOpSize)
        return -1;
      return MaskVal;
    });
    return;
  }
  assert(&Operand == SI.getOperand(1) &&
         "wrong argument: a shufflevector operand was expected");
  std::transform(FirstIt, LastIt, OutIt,
                 [FirstOpSize](int MaskVal) { return MaskVal - FirstOpSize; });
}

// Matches "vector" region (with vstride == 0) pattern in
// [\p FirstIt, \p LastIt) indexes.
// Uses info in \p FirstElemRegion, adds defined Width, Stride and
// new NumElements to \p FirstElemRegion and returns resulting region.
//
// Arguments:
//    [\p FirstIt, \p LastIt) is the range of indexes into some vector.
//    Negative index means invalid index.
//    \p FirstElemRegion describes one element region with only one index
//    *FirstIt.
template <typename ForwardIter>
Region matchVectorRegionByIndexes(Region FirstElemRegion, ForwardIter FirstIt,
                                  ForwardIter LastIt) {
  assert(FirstIt != LastIt && "the range must contain at least 1 element");

  if (std::distance(FirstIt, LastIt) == 1)
    return FirstElemRegion;
  int Stride = *std::next(FirstIt) - *FirstIt;
  if (Stride < 0)
    return FirstElemRegion;
  auto NewRowIt =
      std::adjacent_find(FirstIt, LastIt, [Stride](int First, int Second) {
        return Second < 0 || Second - First != Stride;
      });
  if (NewRowIt != LastIt) {
    ++NewRowIt;
  }
  int Width = std::distance(FirstIt, NewRowIt);
  assert(Width > 0 && "should be at least 1 according to algorithm");
  if (Width == 1)
    // Stride doesn't play role when the Width is 1.
    // Also it prevents from writing to big value in the region.
    Stride = 0;
  FirstElemRegion.Stride = Stride;
  FirstElemRegion.Width = Width;
  FirstElemRegion.NumElements = Width;
  return FirstElemRegion;
}

// Matches "matrix" region (vstride may not equal to 0) pattern in
// [\p FirstIt, \p LastIt) index.
// Uses info in \p FirstRowRegion, adds defined VStride and new NumElements to
// \p FirstRowRegion and returns resulting region.
//
// Arguments:
//    [\p FirstIt, \p LastIt) is the range of indexes into some vector.
//    Negative index means invalid index.
//    \p FirstRowRegion describes "vector" region (with vstride == 0),
//      which is formed by first 'FirstRowRegion.NumElements' elements
//      of the range.
template <typename ForwardIter>
Region matchMatrixRegionByIndexes(Region FirstRowRegion, ForwardIter FirstIt,
                                  ForwardIter LastIt) {
  assert(FirstRowRegion.NumElements == FirstRowRegion.Width &&
         FirstRowRegion.VStride == 0 &&
         "wrong argunent: vector region (with no vstride) was expected");

//  TODO: rewrite this assert to remove VS build error
//  assert(std::distance(FirstIt, LastIt) >= FirstRowRegion.Width &&
//         "wrong argument: number of indexes must be at least equal to region "
//         "width");

  auto FirstRowEndIt = std::next(FirstIt, FirstRowRegion.Width);
  if (FirstRowEndIt == LastIt)
    return FirstRowRegion;
  int VStride = *FirstRowEndIt - *FirstIt;
  if (VStride < 0)
    return FirstRowRegion;

  int Width = FirstRowRegion.Width;
  int VDistance = VStride;
  int NumElements = Width;
  for (auto It = FirstRowEndIt; It != LastIt; advanceSafe(It, LastIt, Width),
            NumElements += Width, VDistance += VStride) {
    if (std::distance(It, LastIt) < Width ||
        !std::equal(FirstIt, FirstRowEndIt, It,
                    [VDistance](int Reference, int Current) {
                      return Current - Reference == VDistance && Current >= 0;
                    }))
      break;
  }
  if (NumElements == Width)
    // VStride doesn't play role when the Width is equal to NumElements.
    // Also it prevents from writing to big value in the region.
    VStride = 0;
  FirstRowRegion.VStride = VStride;
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
  assert(StartIdx >= 0 && 
         StartIdx < static_cast<int>(SI->getShuffleMask().size()) &&
         "Start index is out of bound");

  auto MaskVals = SI->getShuffleMask();
  auto StartIt = std::next(MaskVals.begin(), StartIdx);
  OperandRegionInfo Res = matchOneElemRegion(*SI, *StartIt);

  if (StartIdx == MaskVals.size() - 1)
    return Res;

  makeSVIIndexesOperandIndexes(*SI, *Res.Op, StartIt, MaskVals.end(), StartIt);

  Res.R = matchVectorRegionByIndexes(std::move(Res.R), StartIt, MaskVals.end());
  Res.R = matchMatrixRegionByIndexes(std::move(Res.R), StartIt, MaskVals.end());
  return Res;
}

/***********************************************************************
 * ShuffleVectorAnalyzer::getAsUnslice : see if the shufflevector is an
 *    unslice where the "old value" is operand 0 and operand 1 is another
 *    shufflevector and operand 0 of that is the "new value"
 *
 * Return:  start index, or -1 if it is not an unslice
 */
int ShuffleVectorAnalyzer::getAsUnslice()
{
  auto SI2 = dyn_cast<ShuffleVectorInst>(SI->getOperand(1));
  if (!SI2)
    return -1;
  Constant *MaskVec = cast<Constant>(SI->getOperand(2));
  // Find prefix of undef or elements from operand 0.
  unsigned OldWidth = SI2->getType()->getVectorNumElements(); 
  unsigned NewWidth = SI2->getOperand(0)->getType()->getVectorNumElements(); 
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
  Constant *MaskVec2 = cast<Constant>(SI2->getOperand(2));
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
 * ShuffleVectorAnalyzer::getAsSplat : if shufflevector is a splat, get the
 *      splatted input, with its vector index if the input is a vector
 */
ShuffleVectorAnalyzer::SplatInfo ShuffleVectorAnalyzer::getAsSplat()
{
  Value *InVec1 = SI->getOperand(0);
  Value *InVec2 = SI->getOperand(1);
  Constant *MaskVec = cast<Constant>(SI->getOperand(2));
  ConstantInt *IdxVal = dyn_cast_or_null<ConstantInt>(MaskVec->getSplatValue());
  if (!IdxVal)
    return SplatInfo(0, 0);
  // The mask is a splat. Work out which element of which input vector
  // it refers to.
  unsigned ShuffleIdx = IdxVal->getSExtValue();
  unsigned InVec1NumElements = InVec1->getType()->getVectorNumElements();
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
  unsigned NElts = SI->getType()->getVectorNumElements();
  unsigned M = V->getType()->getVectorNumElements();
  bool SkipBase = true;
  if (M != NElts) {
    if (auto C = dyn_cast<Constant>(V)) {
      SmallVector<Constant *, 16> Vals;
      for (unsigned i = 0; i < NElts; ++i) {
        Type *Ty = C->getType()->getVectorElementType();
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
    Cost += Op->getType()->getVectorNumElements();

  unsigned NElts = SI->getType()->getVectorNumElements();
  for (unsigned j = 0; j < NElts; ++j) {
    // Undef index returns -1.
    int idx = SI->getMaskValue(j);
    if (idx < 0)
      continue;
    // Count the number of elements out of place.
    unsigned M = Op->getType()->getVectorNumElements();
    if ((i == 0 && idx != j) || (i == 1 && idx != j + M))
      Cost++;
  }

  return Cost;
}


IVSplitter::IVSplitter(Instruction &Inst, unsigned* BaseOpIdx) : Inst(Inst) {

  ETy = Inst.getType();
  if (BaseOpIdx)
    ETy = Inst.getOperand(*BaseOpIdx)->getType();

  Len = 1;
  if (ETy->isVectorTy()) {
    Len = ETy->getVectorNumElements();
    ETy = ETy->getVectorElementType();
  }

  VI32Ty = VectorType::get(ETy->getInt32Ty(Inst.getContext()), Len * 2);
}

Region IVSplitter::createSplitRegion(Type *Ty, IVSplitter::RegionType RT) {
  Region R(Ty);
  R.Width = Len;
  R.NumElements = Len;
  R.VStride = 0;

  if (RT == RegionType::LoRegion || RT == RegionType::HiRegion) {
    // take every second element;
    R.Stride = 2;
    // offset is encoded in bytes
    R.Offset = (RT == RegionType::LoRegion) ? 0 : 4;
  }
  else if (RT == RegionType::FirstHalf || RT == RegionType::SecondHalf) {
    // take every element
    R.Stride = 1;
    // offset is encoded in bytes
    R.Offset = (RT == RegionType::FirstHalf) ? 0 : 4 * Len;
  }
  else {
    llvm_unreachable("incorrect region type");
  }
  return R;
}

std::pair<Value*, Value*> IVSplitter::splitValue(Value& Val, RegionType RT1,
                                                 const Twine& Name1,
                                                 RegionType RT2,
                                                 const Twine& Name2) {
  const auto &DL = Inst.getDebugLoc();
  auto BaseName = Inst.getName();

  assert(Val.getType()->getScalarType()->isIntegerTy(64));
  auto *ShreddedVal = new BitCastInst(&Val, VI32Ty, BaseName + ".iv32cast", &Inst);
  ShreddedVal->setDebugLoc(DL);

  auto R1 = createSplitRegion(VI32Ty, RT1);
  auto *V1 = R1.createRdRegion(ShreddedVal, BaseName + Name1, &Inst, DL);

  auto R2 = createSplitRegion(VI32Ty, RT2);
  auto *V2 = R2.createRdRegion(ShreddedVal, BaseName + Name2, &Inst, DL);
  return { V1, V2 };
}

IVSplitter::LoHiSplit IVSplitter::splitOperandLoHi(unsigned SourceIdx) {

  assert(Inst.getNumOperands() > SourceIdx);
  auto Splitted =  splitValue(*Inst.getOperand(SourceIdx),
                              RegionType::LoRegion, ".LoSplit",
                              RegionType::HiRegion, ".HiSplit");

  return {Splitted.first, Splitted.second};
}
IVSplitter::HalfSplit IVSplitter::splitOperandHalf(unsigned SourceIdx) {

  assert(Inst.getNumOperands() > SourceIdx);
  auto Splitted =  splitValue(*Inst.getOperand(SourceIdx),
                              RegionType::FirstHalf, ".FirstHalf",
                              RegionType::SecondHalf, ".SecondHalf");

  return {Splitted.first, Splitted.second};
}
Value* IVSplitter::combineSplit(Value &V1, Value &V2, RegionType RT1,
                                RegionType RT2, const Twine& Name,
                                bool Scalarize) {
  const auto &DL = Inst.getDebugLoc();

  assert(V1.getType() == V2.getType() && V1.getType()->isVectorTy() &&
         V1.getType()->getVectorElementType()->isIntegerTy(32));

  // create the write-regions
  auto R1 = createSplitRegion(VI32Ty, RT1);
  auto *UndefV = UndefValue::get(VI32Ty);
  auto *W1 = R1.createWrRegion(UndefV, &V1, Name + ".partial_join", &Inst, DL);

  auto R2 = createSplitRegion(VI32Ty, RT2);
  auto *W2 = R2.createWrRegion(W1, &V2, Name + ".joined", &Inst, DL);

  auto *V64Ty = VectorType::get(ETy->getInt64Ty(Inst.getContext()), Len);
  auto *Result = new BitCastInst(W2, V64Ty, Name, &Inst);
  Result->setDebugLoc(DL);

  if (Scalarize) {
    assert(Result->getType()->getVectorNumElements() == 1);
    Result = new BitCastInst(Result, ETy->getInt64Ty(Inst.getContext()),
                             Name + ".recast", &Inst);
  }
  return Result;

}
Value *IVSplitter::combineLoHiSplit(const LoHiSplit &Split, const Twine &Name,
                                    bool Scalarize) {

  assert(Split.Lo && Split.Hi);

  return combineSplit(*Split.Lo, *Split.Hi, RegionType::LoRegion,
                      RegionType::HiRegion, Name, Scalarize);
}

Value *IVSplitter::combineHalfSplit(const HalfSplit &Split, const Twine &Name,
                                    bool Scalarize) {
  assert(Split.Left && Split.Right);

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
void genx::adjustPhiNodesForBlockRemoval(BasicBlock *Succ, BasicBlock *BB)
{
  for (auto i = Succ->begin(), e = Succ->end(); i != e; ++i) {
    auto Phi = dyn_cast<PHINode>(&*i);
    if (!Phi)
      break;
    // For this phi node, get the incoming for BB.
    int Idx = Phi->getBasicBlockIndex(BB);
    assert(Idx >= 0);
    Value *Incoming = Phi->getIncomingValue(Idx);
    // Iterate through BB's predecessors. For the first one, replace the
    // incoming block with the predecessor. For subsequent ones, we need
    // to add new phi incomings.
    auto pi = pred_begin(BB), pe = pred_end(BB);
    assert(pi != pe);
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
        } else if(Inst->getOpcode() == Instruction::Or) {
          if (!haveNoCommonBitsSet(Inst->getOperand(0),
                                  Inst->getOperand(1),
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
#define SUCCSZANY     (true)
#define SUCCHASINST   (succ->size() > 1)
#define SUCCNOINST    (succ->size() <= 1)
#define SUCCANYLOOP   (true)

#define PUSHSUCC(BLK, C1, C2) \
        for(succ_iterator succIter = succ_begin(BLK), succEnd = succ_end(BLK); \
          succIter!=succEnd; ++succIter) {                                   \
          llvm::BasicBlock *succ = *succIter;                                \
          if (!visitSet.count(succ) && C1 && C2) {                           \
            visitVec.push_back(succ);                                        \
            visitSet.insert(succ);                                           \
            break;                                                           \
          }                                                                  \
        }

static bool HasSimdGotoJoinInBlock(BasicBlock *BB)
{
  for (BasicBlock::iterator BBI = BB->begin(),
                            BBE = BB->end();
       BBI != BBE; ++BBI) {
    auto IID = GenXIntrinsic::getGenXIntrinsicID(&*BBI);
    if (IID == GenXIntrinsic::genx_simdcf_goto ||
        IID == GenXIntrinsic::genx_simdcf_join)
      return true;
  }
  return false;
}

void genx::LayoutBlocks(Function &func, LoopInfo &LI)
{
  std::vector<llvm::BasicBlock*> visitVec;
  std::set<llvm::BasicBlock*> visitSet;
  // Insertion Position per loop header
  std::map<llvm::BasicBlock*, llvm::BasicBlock*> InsPos;

  llvm::BasicBlock* entry = &(func.getEntryBlock());
  visitVec.push_back(entry);
  visitSet.insert(entry);
  InsPos[entry] = entry;

  while (!visitVec.empty()) {
    llvm::BasicBlock* blk = visitVec.back();
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
          assert(insp);
          if (blk != insp) {
            blk->moveBefore(insp);
            InsPos[hd] = blk;
          }
        }
        else {
          // move the entire loop to the beginning of
          // the parent loop
          auto LoopStart = InsPos[hd];
          assert(LoopStart);
          auto PaLoop = curLoop->getParentLoop();
          auto PaHd = PaLoop ? PaLoop->getHeader() : entry;
          auto insp = InsPos[PaHd];
          if (LoopStart == hd) {
            // single block loop
            hd->moveBefore(insp);
          }
          else {
            // loop-header is not moved yet, so should be at the end
            // use splice
            llvm::Function::BasicBlockListType& BBList = func.getBasicBlockList();
            BBList.splice(insp->getIterator(), BBList, LoopStart->getIterator(),
              hd->getIterator());
            hd->moveBefore(LoopStart);
          }
          InsPos[PaHd] = hd;
        }
      }
      else {
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
        assert(predPred);
        pred->moveAfter(predPred);
      }
    }
  }
}

void genx::LayoutBlocks(Function &func)
{
  std::vector<llvm::BasicBlock*> visitVec;
  std::set<llvm::BasicBlock*> visitSet;
  // Reorder basic block to allow more fall-through 
  llvm::BasicBlock* entry = &(func.getEntryBlock());
  visitVec.push_back(entry);
  visitSet.insert(entry);

  while (!visitVec.empty()) {
    llvm::BasicBlock* blk = visitVec.back();
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

// normalize g_load with bitcasts.
//
// When a single g_load is being bitcast'ed to different types, clone g_loads.
bool genx::normalizeGloads(Instruction *Inst) {
  assert(isa<LoadInst>(Inst));
  auto LI = cast<LoadInst>(Inst);
  if (getUnderlyingGlobalVariable(LI->getPointerOperand()) == nullptr)
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
  assert(isa<LoadInst>(Inst) || isa<StoreInst>(Inst));
  auto LI = dyn_cast<LoadInst>(Inst);
  auto SI = dyn_cast<StoreInst>(Inst);

  Value *Ptr = LI ? LI->getPointerOperand() : SI->getPointerOperand();
  GlobalVariable *GV = getUnderlyingGlobalVariable(Ptr);
  if (!GV)
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
      auto NewPtrTy = PointerType::get(CI->getType(), LI->getPointerAddressSpace());
      auto NewPtr = ConstantExpr::getBitCast(GV, NewPtrTy);
      auto NewLI = new LoadInst(NewPtr, "",
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

const GlobalVariable *genx::getUnderlyingGlobalVariable(const Value *V) {
  while (auto CE = dyn_cast_or_null<ConstantExpr>(V)) {
    if (CE->getOpcode() == CastInst::BitCast)
      V = CE->getOperand(0);
    else
      break;
  }
  return dyn_cast_or_null<GlobalVariable>(V);
}

GlobalVariable *genx::getUnderlyingGlobalVariable(Value *V) {
  return const_cast<GlobalVariable *>(
      getUnderlyingGlobalVariable(const_cast<const Value *>(V)));
}

bool genx::isGlobalStore(StoreInst *ST) {
  assert(ST);
  return getUnderlyingGlobalVariable(ST->getPointerOperand()) != nullptr;
}

bool genx::isGlobalLoad(LoadInst *LI) {
  assert(LI);
  return getUnderlyingGlobalVariable(LI->getPointerOperand()) != nullptr;
}

bool genx::isLegalValueForGlobalStore(Value *V, Value *StorePtr) {
  // Value should be wrregion.
  auto *Wrr = dyn_cast<CallInst>(V);
  if (!Wrr || !GenXIntrinsic::isWrRegion(Wrr))
    return false;

  // With old value obtained from load instruction with StorePtr.
  Value *OldVal =
      Wrr->getArgOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  auto *LI = dyn_cast<LoadInst>(OldVal);
  return LI && (getUnderlyingGlobalVariable(LI->getPointerOperand()) ==
                getUnderlyingGlobalVariable(StorePtr));
}

bool genx::isGlobalStoreLegal(StoreInst *ST) {
  assert(isGlobalStore(ST));
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
    return LI && getUnderlyingGlobalVariable(LI->getOperand(0)) ==
                     getUnderlyingGlobalVariable(ST->getOperand(1));
  }
  if (B.size() != 3)
    return false;

  CallInst *B1 = dyn_cast<CallInst>(ST->getValueOperand());
  GlobalVariable *GV = getUnderlyingGlobalVariable(ST->getPointerOperand());
  if (!GenXIntrinsic::isWrRegion(B1) || !GV)
    return false;
  assert(B1);
  auto B0 = dyn_cast<LoadInst>(B1->getArgOperand(0));
  if (!B0 || GV != getUnderlyingGlobalVariable(B0->getPointerOperand()))
    return false;

  CallInst *A1 = dyn_cast<CallInst>(B1->getArgOperand(1));
  if (!GenXIntrinsic::isRdRegion(A1))
    return false;
  assert(A1);
  LoadInst *A0 = dyn_cast<LoadInst>(A1->getArgOperand(0));
  if (!A0 || GV != getUnderlyingGlobalVariable(A0->getPointerOperand()))
    return false;

  Region R1(A1, BaleInfo());
  Region R2(B1, BaleInfo());
  return R1 == R2;
}

// Check that region can be represented as raw operand.
bool genx::isValueRegionOKForRaw(Value *V, bool IsWrite,
                                 const GenXSubtarget *ST) {
  assert(V);
  switch (GenXIntrinsic::getGenXIntrinsicID(V)) {
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf:
    if (IsWrite)
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
  Region R(cast<Instruction>(V), BaleInfo());
  return isRegionOKForRaw(R, ST);
}

bool genx::isRegionOKForRaw(const genx::Region &R, const GenXSubtarget *ST) {
  unsigned GRFWidth = ST ? ST->getGRFWidth() : 32;
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
  assert(genx::isInlineAsmMatchingInputConstraint(Info) &&
         "Matching input expected");
  int OperandValue = std::stoi(Info.Codes.front());
  assert(OperandValue >= 0);
  return OperandValue;
}

std::vector<GenXInlineAsmInfo> genx::getGenXInlineAsmInfo(MDNode *MD) {
  std::vector<GenXInlineAsmInfo> Result;
  for (auto &MDOp : MD->operands()) {
    auto EntryMD = dyn_cast<MDTuple>(MDOp);
    assert(EntryMD && EntryMD->getNumOperands() == 3 &&
           "error setting metadata for inline asm");
    ConstantAsMetadata *Op0 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(0));
    ConstantAsMetadata *Op1 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(1));
    ConstantAsMetadata *Op2 =
        dyn_cast<ConstantAsMetadata>(EntryMD->getOperand(2));
    assert(Op0 && Op1 && Op2 && "error setting metadata for inline asm");
    auto CTy = static_cast<genx::ConstraintType>(
        cast<ConstantInt>(Op0->getValue())->getZExtValue());
    Result.emplace_back(CTy, cast<ConstantInt>(Op1->getValue())->getSExtValue(),
                        cast<ConstantInt>(Op2->getValue())->getZExtValue());
  }
  return Result;
}

std::vector<GenXInlineAsmInfo> genx::getGenXInlineAsmInfo(CallInst *CI) {
  assert(CI->isInlineAsm() && "Inline asm expected");
  MDNode *MD = CI->getMetadata(genx::MD_genx_inline_asm_info);
  // empty constraint info
  if (!MD) {
    auto *IA = cast<InlineAsm>(CI->getCalledValue());
    assert(IA->getConstraintString().empty() &&
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
  assert(CI->isInlineAsm() && "Inline asm expected");
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
    assert(Ty->getVectorNumElements() == 1 &&
      "wrong argument: scalar or degenerate vector is expected");
    return Ty->getScalarType();
  }
  return VectorType::get(Ty, 1);
}

// info is at main template function
CastInst *genx::scalarizeOrVectorizeIfNeeded(Instruction *Inst, Type *RefType) {
  return scalarizeOrVectorizeIfNeeded(Inst, &RefType, std::next(&RefType));
}

// info is at main template function
CastInst *genx::scalarizeOrVectorizeIfNeeded(Instruction *Inst,
  Instruction *InstToReplace) {
  return scalarizeOrVectorizeIfNeeded(Inst, &InstToReplace, std::next(&InstToReplace));
}

const Type &genx::fixDegenerateVectorType(const Type &Ty) {
  if (!isa<VectorType>(Ty))
    return Ty;
  auto &VecTy = cast<VectorType>(Ty);
  if (VecTy.getNumElements() != 1)
    return Ty;
  return *VecTy.getElementType();
}

Type &genx::fixDegenerateVectorType(Type &Ty) {
  return const_cast<Type &>(
      fixDegenerateVectorType(static_cast<const Type &>(Ty)));
}

Value *genx::getFunctionPointer(Value *V) {
  Instruction *I = nullptr;
  while (I = dyn_cast<Instruction>(V)) {
    if (isa<SelectInst>(I))
      V = I->getOperand(1);
    else if (isa<BitCastInst>(I) || isa<PtrToIntInst>(I))
      V = I->getOperand(0);
    else
      break;
  }
  ConstantExpr *CE = nullptr;
  while ((CE = dyn_cast<ConstantExpr>(V)) &&
         (CE->getOpcode() == Instruction::ExtractElement ||
          CE->isCast()))
    V = CE->getOperand(0);
  if (isa<Constant>(V) && V->getType()->isPointerTy() &&
      V->getType()->getPointerElementType()->isFunctionTy()) {
    return V;
  }
  return nullptr;
}

bool genx::isFuncPointerVec(Value *V, SetVector<Function *> *Funcs) {
  bool Res = true;
  if (V->getType()->isVectorTy() && isa<ConstantExpr>(V) &&
      cast<ConstantExpr>(V)->getOpcode() == Instruction::BitCast) {
    Res = getFunctionPointer(cast<ConstantExpr>(V)->getOperand(0));
  } else if (ConstantVector *Vec = dyn_cast<ConstantVector>(V)) {
    for (auto it = Vec->op_begin(), ie = Vec->op_end(); it != ie; it++) {
      auto *F = getFunctionPointer(*it);
      if (F && Funcs) {
        Funcs->insert(cast<Function>(F));
      } else if (!F) {
        Res = false;
        break;
      }
    }
  } else
    Res = false;
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
    return Log2_32(GRFWidth) + 1;
  default:
    report_fatal_error("Unknown alignment");
  }
}

VISA_Align genx::getVISA_Align(unsigned LogAlignment, unsigned GRFWidth) {
  if (LogAlignment == Log2_32(ByteBytes))
    return ALIGN_BYTE;
  else if (LogAlignment == Log2_32(WordBytes))
    return ALIGN_WORD;
  else if (LogAlignment == Log2_32(DWordBytes))
    return ALIGN_DWORD;
  else if (LogAlignment == Log2_32(QWordBytes))
    return ALIGN_QWORD;
  else if (LogAlignment == Log2_32(OWordBytes))
    return ALIGN_OWORD;
  else if (LogAlignment == Log2_32(GRFWidth))
    return ALIGN_GRF;
  else if (LogAlignment == Log2_32(GRFWidth) + 1)
    return ALIGN_2_GRF;
  else
    report_fatal_error("Unknown log alignment");
}

unsigned genx::CeilAlignment(unsigned LogAlignment, unsigned GRFWidth) {
  if (LogAlignment <= Log2_32(ByteBytes))
    return Log2_32(ByteBytes);
  else if (LogAlignment <= Log2_32(WordBytes))
    return Log2_32(WordBytes);
  else if (LogAlignment <= Log2_32(DWordBytes))
    return Log2_32(DWordBytes);
  else if (LogAlignment <= Log2_32(QWordBytes))
    return Log2_32(QWordBytes);
  else if (LogAlignment <= Log2_32(OWordBytes))
    return Log2_32(OWordBytes);
  else if (LogAlignment <= Log2_32(GRFWidth))
    return Log2_32(GRFWidth);
  else if (LogAlignment <= Log2_32(GRFWidth) + 1)
    return Log2_32(GRFWidth) + 1;
  else
    report_fatal_error("Unknown log alignment");
}
