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
// Implementation of methods for Region class
//
//===----------------------------------------------------------------------===//

#include "GenXRegion.h"
#include "GenXAlignmentInfo.h"
#include "GenXBaling.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "vc/GenXOpts/GenXAnalysis.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include <unordered_map>

using namespace llvm;
using namespace genx;

/***********************************************************************
 * getWithOffset : get a Region given a rdregion/wrregion, baling in
 *      constant add of offset
 *
 * This constructs the Region with a variable index that is a constant add
 * baled in (i.e. Region::Indirect and Region::Offset both set to the
 * operands of the add). It is for use when baling information is not
 * available, but the caller wants the constant offset separated out like
 * that.
 */
Region Region::getWithOffset(Instruction *Inst, bool WantParentWidth)
{
  unsigned OperandNum = 0;
  switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf:
      OperandNum = GenXIntrinsic::GenXRegion::RdIndexOperandNum;
      break;
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
    case GenXIntrinsic::genx_wrconstregion:
      OperandNum = GenXIntrinsic::GenXRegion::WrIndexOperandNum;
      break;
    default:
      llvm_unreachable("not rdregion or wrregion");
      break;
  }
  BaleInfo BI;
  if (GenXBaling::isBalableIndexAdd(Inst->getOperand(OperandNum)))
    BI.setOperandBaled(OperandNum);
  return Region(Inst, BI, WantParentWidth);
}

/***********************************************************************
 * Region constructor from a rd/wr region and its BaleInfo
 * This also works with rdpredregion and wrpredregion, with Offset in
 * bits rather than bytes, and with ElementBytes set to 1.
 */
Region::Region(Instruction *Inst, const BaleInfo &BI, bool WantParentWidth)
    : CMRegion()
{
  // Determine where to get the subregion value from and which arg index
  // the region parameters start at.
  unsigned ArgIdx = 0;
  Value *Subregion = 0;
  assert(isa<CallInst>(Inst));
  auto CallI = cast<CallInst>(Inst);
  assert(CallI->getCalledFunction());
  switch (GenXIntrinsic::getGenXIntrinsicID(CallI->getCalledFunction())) {
    case GenXIntrinsic::genx_rdpredregion:
      NumElements = Inst->getType()->getVectorNumElements();
      Width = NumElements;
      Offset = cast<ConstantInt>(Inst->getOperand(1))->getZExtValue();
      ElementBytes = 1;
      return;
    case GenXIntrinsic::genx_wrpredregion:
      NumElements = Inst->getOperand(1)->getType()->getVectorNumElements();
      Width = NumElements;
      Offset = cast<ConstantInt>(Inst->getOperand(2))->getZExtValue();
      ElementBytes = 1;
      return;
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf:
      ArgIdx = 1;
      // The size/type of the region is given by the return value:
      Subregion = Inst;
      break;
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
    case GenXIntrinsic::genx_wrconstregion:
      ArgIdx = 2;
      // The size/type of the region is given by the "subregion value to
      // write" operand:
      Subregion = Inst->getOperand(1);
      // For wrregion, while we're here, also get the mask. We set mask to NULL
      // if the mask operand is constant 1 (i.e. not predicated).
      Mask = Inst->getOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum);
      if (auto C = dyn_cast<Constant>(Mask))
        if (C->isAllOnesValue())
          Mask = 0;
      break;
    default:
      assert(0);
  }
  // Get the region parameters.
  assert(Subregion);
  ElementTy = Subregion->getType();
  if (VectorType *VT = dyn_cast<VectorType>(ElementTy)) {
    ElementTy = VT->getElementType();
    NumElements = VT->getNumElements();
  }
  const DataLayout &DL = Inst->getModule()->getDataLayout();
  assert(DL.getTypeSizeInBits(ElementTy) % genx::ByteBits  == 0);
  ElementBytes = DL.getTypeSizeInBits(ElementTy) / genx::ByteBits;
  VStride = cast<ConstantInt>(Inst->getOperand(ArgIdx))->getSExtValue();
  Width = cast<ConstantInt>(Inst->getOperand(ArgIdx + 1))->getSExtValue();
  Stride = cast<ConstantInt>(Inst->getOperand(ArgIdx + 2))->getSExtValue();
  ArgIdx += 3;
  // Get the start index.
  Value *V = Inst->getOperand(ArgIdx);
  assert(V->getType()->getScalarType()->isIntegerTy(16) &&
         "region index must be i16 or vXi16 type");
#if _DEBUG
  if (VectorType *VT = dyn_cast<VectorType>(V->getType()))
    assert(VT->getNumElements() * Width == NumElements &&
           "vector region index size mismatch");
#endif
  if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
    Offset = CI->getSExtValue(); // Constant index.
  else {
    Indirect = V; // Index is variable; assume no baled in add.
    if (BI.isOperandBaled(ArgIdx)) {
      Instruction *Operator = cast<Instruction>(V);
      // The index is variable and has something baled in. We want to process
      // a baled in add or add_addr, and ignore a baled in rdregion.
      if(!GenXIntrinsic::isRdRegion(Operator)) {
        // The index is variable and has a baled in or/add/sub/add_addr.
        assert((Operator->getOpcode() == Instruction::Add   ||
                Operator->getOpcode() == Instruction::Sub   ||
                Operator->getOpcode() == Instruction::Or    ||
                GenXIntrinsic::getGenXIntrinsicID(Operator) == GenXIntrinsic::genx_add_addr)
                && "error: your offset seems to be calculated not through 'add' or 'or' ");
        Constant *C = cast<Constant>(Operator->getOperand(1));
        ConstantInt *CI = dyn_cast<ConstantInt>(C);
        if (!CI)
          CI = cast<ConstantInt>(C->getSplatValue());

        // check for or could be changed to add
        if (Operator->getOpcode() == Instruction::Or &&
          !haveNoCommonBitsSet(Operator->getOperand(0), Operator->getOperand(1),
          Operator->getModule()->getDataLayout()))
        {
          assert(false && "or should be changed to add without any errors");
        }


        Offset = CI->getSExtValue();

        if (Operator->getOpcode() == Instruction::Sub)
          Offset = -Offset;

        Indirect = Operator->getOperand(0);
      }
    }
    // For a variable index, get the parent width arg.
    ConstantInt *PW = dyn_cast<ConstantInt>(Inst->getOperand(ArgIdx + 1));
    if (PW)
      ParentWidth = PW->getZExtValue();
  }
  // We do some trivial legalization here. The legalization pass does not
  // make these changes; instead we do them here so they are not permanently
  // written back into the IR but are made on the fly each time some other
  // pass uses this code to get the region info.
  if (NumElements == 1) {
    Width = Stride = 1;
    VStride = 0;
  } else {
    if (NumElements <= Width) {
      Width = NumElements;
      VStride = 0;
    } else if ((unsigned)VStride == Width * Stride) {
      // VStride == Width * Stride, so we can canonicalize to a 1D region,
      // but only if not indirect or not asked to preserve parentwidth,
      // and never if multi-indirect.
      if (!Indirect
          || (!isa<VectorType>(Indirect->getType()) && !WantParentWidth)) {
        Width = NumElements;
        VStride = 0;
        ParentWidth = 0;
      }
    } else if (Width == 1) {
      // We can turn a 2D width 1 region into a 1D region, but if it is
      // indirect it invalidates ParentWidth. So only do it if not asked
      // to keep ParentWidth. Also we cannot do it if it is multi-indirect.
      if (!Indirect
          || (!isa<VectorType>(Indirect->getType()) && !WantParentWidth)) {
        Width = NumElements;
        Stride = VStride;
        VStride = 0;
        ParentWidth = 0;
      }
    }
    if (Stride == 0 && Width == NumElements) {
      // Canonical scalar region.
      Width = 1;
      VStride = 0;
    }
  }
}

/***********************************************************************
 * Region::getLegalSize : get the max legal size of a region
 *
 * Enter:   Idx = start index into the subregion
 *          Allow2D = whether to allow 2D region
 *          InputNumElements = number of elements in whole input vector (so
 *                we can tell if it is small enough that it cannot possibly
 *                cross a GRF boundary)
 *          ST = GenXSubtarget (so we can get gen specific crossing rules)
 *          AI = 0 else AlignmentInfo (to determine alignment of indirect index)
 */
unsigned Region::getLegalSize(unsigned Idx, bool Allow2D,
    unsigned InputNumElements, const GenXSubtarget *ST, AlignmentInfo *AI)
{
  Alignment Align;
  if (Indirect) {
    Align = Alignment::getUnknown();
    if (AI)
      Align = AI->get(Indirect);
  }
  return getLegalSize(Idx, Allow2D, InputNumElements, ST, Align);
}

/***********************************************************************
 * Region::getLegalSize : get the max legal size of a region
 *
 * Enter:   Idx = start index into the subregion
 *          Allow2D = whether to allow 2D region
 *          InputNumElements = number of elements in whole input vector (so
 *                we can tell if it is small enough that it cannot possibly
 *                cross a GRF boundary)
 *          ST = GenXSubtarget (so we can get gen specific crossing rules)
 *          Align = alignment of indirect index if any
 *
 * The setting of Indirect is used as follows:
 *
 * 0: not indirect
 * anything of scalar type: single indirect
 * anything of vector type: multi indirect
 */
unsigned Region::getLegalSize(unsigned Idx, bool Allow2D,
    unsigned InputNumElements, const GenXSubtarget *ST, Alignment Align)
{
  // Determine the max valid width.
  unsigned ValidWidth = 1;
  unsigned GRFWidth = ST ? ST->getGRFWidth() : 32;
  int MaxStride = 4;
  unsigned LogGRFWidth = genx::log2(GRFWidth);
  if ((!Stride || exactLog2(Stride) >= 0) && (Allow2D || Stride <= MaxStride)) {
    // The stride is legal, so we can potentially do more than one element at a
    // time.
    // Disallow 2D if the stride is too large for a real Gen region. For a
    // source operand (Allow2D is true), we allow a 1D region with stride too
    // large, because the vISA writer turns it into a 2D region with width 1.
    bool StrideValid = (Stride <= MaxStride);

    if (Indirect && isa<VectorType>(Indirect->getType())) {
      // Multi indirect.
      if (!Allow2D) {
        // Multi indirect not allowed in wrregion.
        if (!Stride)
          ValidWidth = 1 << genx::log2(Width);
      } else if (Width == 1 || !Stride) {
        // Multi indirect with width 1 or stride 0.
        // Return the max power of two number of elements that:
        // 1. fit in 2 GRFs; and
        // 2. fit in the whole region; and
        // 3. fit in a row if the width is not legal
        // 4. no more than 8 elements in multi indirect (because there
        //    are only 8 elements in an address register).
        unsigned LogWidth = genx::log2(Width);
        if (1U << LogWidth == Width)
          LogWidth = genx::log2(NumElements); // legal width
        unsigned LogElementBytes = genx::log2(ElementBytes);
        if (LogWidth + LogElementBytes > (LogGRFWidth + 1))
          LogWidth = LogGRFWidth + 1 - LogElementBytes;
        ValidWidth = 1 << LogWidth;
        if (ValidWidth > 8)
          ValidWidth = 8;
      }
      // Other multi indirect can only do one element.
    } else {
      // Calculate number of elements up to the boundary imposed by GRF
      // crossing rules.
      unsigned ElementsPerGRF = GRFWidth / ElementBytes;
      unsigned OffsetElements = Offset / ElementBytes;
      unsigned ElementsToBoundary = 1;
      unsigned RealIdx = Idx / Width * VStride + Idx % Width * Stride;
      if (!Indirect) {
        // For a direct operand, just use the constant offset of the
        // region and the index so far to calculate how far into a GRF this
        // subregion starts, and set the boundary at the next-but-one GRF
        // boundary.
        unsigned NumGRF = 2;
        ElementsToBoundary = (NumGRF * ElementsPerGRF) -
                             ((RealIdx + OffsetElements) % ElementsPerGRF);
      } else if (InputNumElements <= ElementsPerGRF) {
        // Indirect region but the whole vector is no bigger than a GRF, so
        // there is no limit imposed by GRF crossing.
        ElementsToBoundary = ElementsPerGRF;
      } else {
        // For an indirect region, calculate the min and max index (including
        // offset) from the region parameters, and add on the current start
        // index to both.
        // For <= BDW:
        //   1. If the min and max then are in the same GRF, then the distance
        //      from max to the next GRF boundary is the allowed size.
        // For >= SKL:
        //   1. If the min and max then are in the same GRF, then the distance
        //      from max to the next-but-one GRF boundary is the allowed size.
        //   2. If the max is in the next GRF after min, then the distance
        //      from max to the next GRF boundary is the allowed size.
        // However vISA imposes the restriction that, in a source indirect
        // region, a row cannot cross a GRF, unless the region is contiguous.
        // Pending a proper fix, we have a temporary fix here that we disallow
        // GRF crossing completely unless the original region is a destination
        // operand or is a 1D source operand (so GenXVisaFuncWriter can turn it
        // into Nx1 instead of 1xN).  We use Allow2D as a proxy for "is source
        // operand".
        unsigned GRFsPerIndirect = 1;
        assert(ST);
        if (ST->hasIndirectGRFCrossing() &&
          // SKL+. See if we can allow GRF crossing.
            (Allow2D || !is2D())) {
            GRFsPerIndirect = 2;
        }
        unsigned Last = (NumElements / Width - 1) * VStride + (Width - 1) * Stride;
        unsigned Max = InputNumElements - Last - 1 + RealIdx;
        unsigned Min = RealIdx;
        unsigned MinMaxGRFDiff = (Max & -ElementsPerGRF) - (Min & -ElementsPerGRF);
        if (!MinMaxGRFDiff) // min and max in same GRF
          ElementsToBoundary = ElementsPerGRF * GRFsPerIndirect
              - (Max & (ElementsPerGRF - 1));
        else if (MinMaxGRFDiff == 1 && GRFsPerIndirect > 1)
          ElementsToBoundary = ElementsPerGRF - (Max & (ElementsPerGRF - 1));
        // We may be able to refine an indirect region legal width further...
        if (exactLog2(ParentWidth) >= 0
            && ParentWidth <= ElementsPerGRF) {
          // ParentWidth tells us that a row of our region cannot cross a GRF
          // boundary. Say that the boundary is at the next multiple of
          // ParentWidth.
          ElementsToBoundary = std::max(ParentWidth - RealIdx % ParentWidth,
                ElementsToBoundary);
        } else if (!isa<VectorType>(Indirect->getType())) {
          // Use the alignment+offset of the single indirect index, with alignment
          // limited to one GRF.
          if (!Align.isUnknown()) {
            unsigned LogAlign = Align.getLogAlign();
            unsigned ExtraBits = Align.getExtraBits();
            ExtraBits += (Offset + RealIdx * ElementBytes);
            ExtraBits &= ((1 << LogAlign) - 1);
            if (LogAlign >= LogGRFWidth && !ExtraBits) {
              // Start is GRF aligned, so legal width is 1 GRF for <=BDW or
              // 2 GRFs for >=SKL.
              ElementsToBoundary = ElementsPerGRF * GRFsPerIndirect;
            } else if (LogAlign > (unsigned)genx::log2(ElementBytes) ||
                       (LogAlign == (unsigned)genx::log2(ElementBytes) &&
                        ExtraBits == 0)) {
              LogAlign = std::min(LogGRFWidth, LogAlign) - genx::log2(ElementBytes);
              ExtraBits = (ExtraBits & (GRFWidth-1)) >> genx::log2(ElementBytes);
              // We have some alignment, so we can say that the next GRF boundary
              // is (at least) that many elements away, minus the offset from that
              // alignment.
              // For SKL+, we can cross one GRF boundary, so add on one GRF's
              // worth.
              unsigned ElementsToBoundaryFromAlign = (1U << LogAlign) - ExtraBits;
              ElementsToBoundaryFromAlign += (GRFsPerIndirect - 1) * ElementsPerGRF;
              ElementsToBoundary = std::max(ElementsToBoundaryFromAlign,
                  ElementsToBoundary);
            }
          }
        }
      }

      // Now calculate what subregion we can fit in before the boundary
      // calculated above.
      if (Allow2D && StrideValid) {
        if ((!VStride || exactLog2(VStride) >= 0) && exactLog2(Width) >= 0
          && Width <= 16 && !(Idx % Width)
            && ElementsToBoundary >= (Width - 1) * Stride + 1) {
          // The vstride and width are legal, and we're at the start of a
          // row, and ElementsToBoundary is big enough for at least one
          // whole row, so we can potentially do more than one whole row at a
          // time. See how many we can fit, without including the "slack"
          // at the end of the last row.
          unsigned NumRows = 0;
          if (VStride == 0) // Avoid divide by 0
            NumRows = (NumElements - Idx) / Width;
          else {
            unsigned LastElementOfRow = (Width - 1) * Stride;
            unsigned Slack = VStride - (LastElementOfRow + 1);
            NumRows = (ElementsToBoundary + Slack) / VStride;
            if (NumRows) {
              if (NumRows * Width + Idx > NumElements)
                NumRows = (NumElements - Idx) / Width;
            }
          }
          ValidWidth = (1 << genx::log2(NumRows)) * Width;
        }
        if (ValidWidth == 1 && Idx % Width) {
          // That failed. See if we can legally get to the end of the row then
          // the same number of elements again at the start of the next row.
          unsigned ToEndOfRow = Width - Idx % Width;
          if (exactLog2(ToEndOfRow) >= 0 && ToEndOfRow <= 16) {
            unsigned NewVStride = VStride + (ToEndOfRow - Width) * Stride;
            if (exactLog2(NewVStride) >= 0
                && NewVStride + (ToEndOfRow - 1) * Stride < ElementsToBoundary) {
              // Yes, we can do the end of one row and the same size start of
              // the next row.
              ValidWidth = 2 * ToEndOfRow;
            }
          }
        }
      }
      if (ValidWidth == 1) {
        // That failed. See how many elements we can get, no further than the
        // next end of row.
        ValidWidth = Width - Idx % Width;
        if (ValidWidth * Stride - (Stride - 1) > ElementsToBoundary)
          ValidWidth = (ElementsToBoundary + Stride - 1) / Stride;
        ValidWidth = 1 << genx::log2(ValidWidth);
      }
      // If the RStride is 0 (which is seen in splat operations) then the
      // above logic tends to determine that all of the elements can fit,
      // irrespective of vector size and type. This is usually incorrect
      // in the wider context, so clamp it here to whatever fits in 2GRF if
      // necessary
      if (ValidWidth > (2 * ElementsPerGRF))
        ValidWidth = 2 * ElementsPerGRF;

    }
  }
  return ValidWidth;
}

/***********************************************************************
 * RdWrRegionSequence::buildFromStartWr:  detect a split (legalized)
 *    sequence rdregion-wrregion from the start, and populate the
 *    RdWrRegionSequence object with its details
 *
 * This fails if there is any predication. It succeeds with a sequence length
 * of one (i.e. a single rdregion-wrregion pair).
 *
 * On success, if the WaitingFor field matches one of the wrregions in the
 * sequence, then WaitingFor is reset to 0. This is used by buildFromWr to
 * check that the sequence includes the wrregion originally passed to it.
 *
 * On failure, EndWr is left as is, which means that isNull() continues to
 * be true.
 */
bool RdWrRegionSequence::buildFromStartWr(Instruction *ArgStartWr,
    GenXBaling *Baling)
{
  StartWr = ArgStartWr;
  auto Wr = StartWr;
  assert(GenXIntrinsic::isWrRegion(Wr));
  Region TotalWrR(Wr, Baling->getBaleInfo(Wr));
  WrR = TotalWrR;
  if (TotalWrR.Mask)
    return false;
  OldVal = Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  auto RdVal = Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
  if (auto Rd = dyn_cast<Instruction>(RdVal)) {
    // Handle the case that the start wrregion has a rdregion, so we look for
    // a sequence of rd-wr pairs.
    if (!GenXIntrinsic::isRdRegion(Rd))
      return false;
    Region TotalRdR(Rd, Baling->getBaleInfo(Rd));
    RdR = TotalRdR;
    Input = Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    EndWr = Wr;
    if (Wr == WaitingFor)
      WaitingFor = nullptr;
    bool SeenWaitingFor = false;
    for (;;) {
      if (!Wr->hasOneUse() || Wr->use_begin()->getOperandNo()
          != GenXIntrinsic::GenXRegion::OldValueOperandNum)
        break;
      Wr = cast<Instruction>(Wr->use_begin()->getUser());
      if (!GenXIntrinsic::isWrRegion(Wr))
        break;
      Value *In = Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      if (!GenXIntrinsic::isRdRegion(In))
        break;
      auto Rd = cast<Instruction>(In);
      if (Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum) != Input)
        break;
      // Append to the regions. Give up if either fails.
      if (!TotalRdR.append(Region(Rd, Baling->getBaleInfo(Rd)))
          || !TotalWrR.append(Region(Wr, Baling->getBaleInfo(Wr))))
        break;
      SeenWaitingFor |= Wr == WaitingFor;
      // If both regions are now legal (have a whole number of rows), then
      // save the current position.
      if (TotalRdR.isWholeNumRows() && TotalWrR.isWholeNumRows()) {
        RdR = TotalRdR;
        WrR = TotalWrR;
        EndWr = Wr;
        if (SeenWaitingFor)
          WaitingFor = nullptr;
      }
    }
    return true;
  }
  if (!isa<UndefValue>(Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum)))
    return false;
  auto TotalC = dyn_cast<Constant>(RdVal);
  if (!TotalC)
    return false;
  // Handle the case that the start wrregion has a constant "new value" operand
  // and an undef "old value" operand.
  // We look for a sequence of wrregions where the "new value" operands are all
  // constant and we derive the overall constant.
  Region TotalRdR(TotalC);
  RdR = TotalRdR;
  Input = TotalC;
  EndWr = Wr;
  if (Wr == WaitingFor)
    WaitingFor = nullptr;
  bool SeenWaitingFor = false;
  for (;;) {
    if (!Wr->hasOneUse() || Wr->use_begin()->getOperandNo()
        != GenXIntrinsic::GenXRegion::OldValueOperandNum)
      break;
    Wr = cast<Instruction>(Wr->use_begin()->getUser());
    if (!GenXIntrinsic::isWrRegion(Wr))
      break;
    auto In = dyn_cast<Constant>(Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
    if (!In)
      break;
    // Append to the regions. Give up if either fails.
    Region InR(In);
    InR.Offset = TotalRdR.NumElements * TotalRdR.ElementBytes;
    if (!TotalRdR.append(InR)
        || !TotalWrR.append(Region(Wr, Baling->getBaleInfo(Wr))))
      break;
    SeenWaitingFor |= Wr == WaitingFor;
    // Append the constant.
    TotalC = concatConstants(TotalC, In);
    // If both regions are now legal (have a whole number of rows), then save
    // the current position.
    if (TotalRdR.isWholeNumRows() && TotalWrR.isWholeNumRows()) {
      RdR = TotalRdR;
      WrR = TotalWrR;
      EndWr = Wr;
      Input = TotalC;
      if (SeenWaitingFor)
        WaitingFor = nullptr;
    }
  }
  return true;
}

/***********************************************************************
 * RdWrRegionSequence::buildFromWr:  detect a split (legalized) rdregion-wrregion
 *    sequence starting from any wrregion within it, and populate the
 *    RdWrRegionSequence object with its details
 *
 * This fails if there is any predication. It succeeds with a sequence length
 * of one (i.e. a single rdregion-wrregion pair).
 *
 * On failure, EndWr is left as is, which means that isNull() continues to
 * be true.
 */
bool RdWrRegionSequence::buildFromWr(Instruction *Wr, GenXBaling *Baling)
{
  // Remember that our sequence needs to contain Wr.
  WaitingFor = Wr;
  // Scan back to what looks like the start of the sequence.
  assert(GenXIntrinsic::isWrRegion(Wr));
  StartWr = Wr;
  auto RdVal = Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
  auto Rd = dyn_cast<Instruction>(RdVal);
  bool ConstSequence = false;
  if (!Rd) {
    if (!isa<Constant>(RdVal))
      return 0;
    ConstSequence = true;
  } else
    Input = Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  for (;;) {
    Wr = dyn_cast<Instruction>(
        Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
    if (!GenXIntrinsic::isWrRegion(Wr))
      break;
    assert(Wr);
    if (!Wr->hasOneUse())
      break;
    RdVal = Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
    if (ConstSequence) {
      if (!isa<Constant>(RdVal))
        break;
    } else {
      Rd = dyn_cast<Instruction>(
          Wr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
      if (!Rd)
        break;
      if (Input != Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum))
        break;
    }
    StartWr = Wr;
  }
  // Try detecting a split rdregion-wrregion starting at StartWr.
  for (;;) {
    if (!buildFromStartWr(StartWr, Baling)) {
      EndWr = nullptr;
      return false;
    }
    if (!WaitingFor)
      return true; // success
    // The detected sequence did not include the wrregion this function
    // started with. Retry with the following sequence.
    StartWr = cast<Instruction>(EndWr->use_begin()->getUser());
    if (GenXIntrinsic::isWrRegion(StartWr))
      return false;
  }
}

/***********************************************************************
 * RdWrRegionSequence::buildFromRd:  detect a split (legalized) rdregion-wrregion
 *    sequence starting from any rdregion within it, and populate the
 *    RdWrRegionSequence object with its details
 *
 * This fails if there is any predication. It succeeds with a sequence length
 * of one (i.e. a single rdregion-wrregion pair).
 */
bool RdWrRegionSequence::buildFromRd(Instruction *Rd, GenXBaling *Baling)
{
  assert(GenXIntrinsic::isRdRegion(Rd));
  if (!Rd->hasOneUse())
    return false;
  if (Rd->use_begin()->getOperandNo() != GenXIntrinsic::GenXRegion::NewValueOperandNum)
    return false;
  auto Wr = cast<Instruction>(Rd->use_begin()->getUser());
  if (!GenXIntrinsic::isWrRegion(Wr))
    return false;
  return buildFromWr(Wr, Baling);
}

/***********************************************************************
 * RdWrRegionSequence::size : get number of rdregion-wrregion pairs in the
 *    sequence
 */
unsigned RdWrRegionSequence::size() const
{
  unsigned Size = 1;
  Instruction *Wr = EndWr;
  for ( ; Wr != StartWr; ++Size)
    Wr = cast<Instruction>(
        Wr->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum));
  return Size;
}

/***********************************************************************
 * RdWrRegionSequence::isOnlyUseOfInput : check whether the sequence is the
 *    only use of its input
 */
bool RdWrRegionSequence::isOnlyUseOfInput() const
{
  unsigned Count = 0;
  for (auto ui = Input->use_begin(), ue = Input->use_end();
      ui != ue; ++ui)
    ++Count;
  return Count == size();
}

/***********************************************************************
 * RdWrRegionSequence::getRdIndex : get the index of the legalized rdregion
 */
Value *RdWrRegionSequence::getRdIndex() const
{
  if (isa<Constant>(Input))
    return ConstantInt::get(Type::getInt16Ty(StartWr->getContext()), 0);
  auto Rd = cast<Instruction>(
      StartWr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
  assert(GenXIntrinsic::isRdRegion(Rd));
  return Rd->getOperand(GenXIntrinsic::GenXRegion::RdIndexOperandNum);
}

/***********************************************************************
 * RdWrRegionSequence::getWrIndex : get the index of the legalized wrregion
 */
Value *RdWrRegionSequence::getWrIndex() const
{
  return StartWr->getOperand(GenXIntrinsic::GenXRegion::WrIndexOperandNum);
}

/***********************************************************************
 * RdWrRegionSequence::getInputUse : get some use of Input in the sequence
 *
 * This only works if the RdWrRegionSequence is a sequence of rd-wr pairs,
 * rather than a sequence of wrregions with constant input. In the latter
 * case, this returns 0.
 */
Use *RdWrRegionSequence::getInputUse() const
{
  auto Rd = dyn_cast<Instruction>(
      StartWr->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum));
  if (!GenXIntrinsic::isRdRegion(Rd))
    return nullptr;
  assert(Rd && Rd->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum) == Input);
  return &Rd->getOperandUse(GenXIntrinsic::GenXRegion::OldValueOperandNum);
}

/***********************************************************************
 * RdWrRegionSequence::print : debug dump/print
 */
void RdWrRegionSequence::print(raw_ostream &OS) const
{
  if (isNull())
    OS << "null";
  else {
    OS << "sequence";
    if (OldVal)
      dbgs() << " OldVal=" << OldVal->getName();
    dbgs() << " Input=" << Input->getName()
      << " StartWr=" << StartWr->getName()
      << " EndWr=" << EndWr->getName()
      << " RdR=" << RdR
      << " WrR=" << WrR;
  }
}

static Value *simplifyRegionWrite(Instruction *Inst) {
  assert(GenXIntrinsic::isWrRegion(Inst));
  Value *NewVal = Inst->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);

  // Replace C with A
  // C = wrregion(A, undef, R)
  if (isa<UndefValue>(NewVal))
    return Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);

  // When A and undef have the same type, replace C with A
  // B = rdregion(A, R)
  // C = wrregion(undef, B, R)
  //
  // or replace C by A
  //
  // B = rdregion(A, R)
  // C = wrregion(A, B, R)
  //
  if (GenXIntrinsic::isRdRegion(NewVal)) {
    Instruction *B = cast<Instruction>(NewVal);
    Region InnerR(B, BaleInfo());
    Region OuterR(Inst, BaleInfo());
    if (OuterR != InnerR)
      return nullptr;

    auto OldValB = B->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    auto OldValC = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
    if ((isa<UndefValue>(OldValC) &&
         OldValB->getType() == OldValC->getType()) ||
        OldValB == OldValC)
      return OldValB;
  }

  return nullptr;
}

static Value *simplifyRegionRead(Instruction *Inst) {
  assert(GenXIntrinsic::isRdRegion(Inst));
  Value *Input = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  if (isa<UndefValue>(Input))
    return UndefValue::get(Inst->getType());
  else if (auto C = dyn_cast<Constant>(Input)) {
    if (auto Splat = C->getSplatValue()) {
      Type *Ty = Inst->getType();
      if (Ty->isVectorTy())
        Splat = ConstantVector::getSplat(Ty->getVectorNumElements(), Splat);
      return Splat;
    }
  } else if (GenXIntrinsic::isWrRegion(Input) && Input->hasOneUse()) {
    // W = wrr(A, B, R)
    // C = rdr(W, R)
    // =>
    // replace C by B
    Instruction *WI = cast<Instruction>(Input);
    Region R1(WI, BaleInfo());
    Region R2(Inst, BaleInfo());
    if (R1 == R2) {
      Value *B = WI->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      if (B->getType() == Inst->getType())
        return B;
    }
  }
  return nullptr;
}

// Simplify a region read or write.
Value *llvm::genx::simplifyRegionInst(Instruction *Inst, const DataLayout *DL) {
  if (Inst->use_empty())
    return nullptr;

  if (Constant *C = ConstantFoldGenX(Inst, *DL))
    return C;

  unsigned ID = GenXIntrinsic::getGenXIntrinsicID(Inst);
  switch (ID) {
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_wrregioni:
    return simplifyRegionWrite(Inst);
  case GenXIntrinsic::genx_rdregionf:
  case GenXIntrinsic::genx_rdregioni:
    return simplifyRegionRead(Inst);
  default:
    break;
  }
  return nullptr;
}

bool llvm::genx::simplifyRegionInsts(Function *F, const DataLayout *DL) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;
      if (auto V = simplifyRegionInst(Inst, DL)) {
        Inst->replaceAllUsesWith(V);
        Inst->eraseFromParent();
        Changed = true;
      }
    }
  }
  return Changed;
}

// Cleanup loads.
// %load1 = load *m
// %load2 = load *m
// no store to m
// use(load1, load2)
//
bool llvm::genx::cleanupLoads(Function *F) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    // The dominating loads (may have different types) for each variable.
    std::unordered_map<GlobalVariable *, std::vector<LoadInst *>> DomLoads;
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;
      if (auto SI = dyn_cast<StoreInst>(Inst)) {
        auto GV = getUnderlyingGlobalVariable(SI->getPointerOperand());
        if (!GV)
          continue;
        // Kill all live loads on this variable.
        DomLoads[GV].clear();
      } else if (auto LI = dyn_cast<LoadInst>(Inst)) {
        auto GV = getUnderlyingGlobalVariable(LI->getPointerOperand());
        if (!GV)
          continue;
        auto &Loads = DomLoads[GV];
        LoadInst *DomLI = nullptr;
        for (auto LI1 : Loads) {
          if (LI1->getType() == LI->getType()) {
            DomLI = LI1;
            break;
          }
        }
        if (DomLI == nullptr)
          Loads.push_back(LI);
        else {
          LI->replaceAllUsesWith(DomLI);
          LI->eraseFromParent();
          Changed = true;
        }
      }
    }
  }
  return Changed;
}

bool
llvm::genx::IsLinearVectorConstantInts(Value* v, int64_t& start, int64_t& stride) {
    auto cv = dyn_cast<ConstantDataVector>(v);
    if (!cv)
        return false;
    // Flatten the vector out into the elements array
    llvm::SmallVector<llvm::Constant*, 16> elements;
    auto vectorLength = cv->getType()->getVectorNumElements();
    for (unsigned i = 0; i < vectorLength; ++i)
        elements.push_back(cv->getElementAsConstant(i));

    llvm::ConstantInt* ci = llvm::dyn_cast<llvm::ConstantInt>(elements[0]);
    if (ci == NULL)
        return false; // Not a vector of integers

    int64_t val0 = ci->getSExtValue();
    if (vectorLength == 1) {
        start = val0;
        stride = 0;
        return true;
    }
    ci = llvm::dyn_cast<llvm::ConstantInt>(elements[1]);
    if (ci == NULL)
        return false; // Not a vector of integers
    int64_t prevVal = ci->getSExtValue();
    int64_t diff = prevVal - val0;

    // For each element in the array, see if it is both a ConstantInt and
    // if the difference between it and the value of the previous element
    // is stride.  If not, fail.
    for (int i = 2; i < (int)vectorLength; ++i) {
        ci = llvm::dyn_cast<llvm::ConstantInt>(elements[i]);
        if (ci == NULL)
            return false;

        int64_t nextVal = ci->getSExtValue();
        if (prevVal + diff != nextVal)
            return false;

        prevVal = nextVal;
    }
    start = val0;
    stride = diff;
    return true;
}
