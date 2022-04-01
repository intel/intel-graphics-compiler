/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Implementation of methods for Region class
//
//===----------------------------------------------------------------------===//

#include "GenXAlignmentInfo.h"
#include "GenXBaling.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"

#include "vc/GenXOpts/GenXAnalysis.h"
#include "vc/Utils/GenX/GlobalVariable.h"
#include "vc/Utils/General/IRBuilder.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/TargetFolder.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include <unordered_map>

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

using namespace llvm;
using namespace genx;

namespace {

/***********************************************************************
 * Local function for testing one assertion statement.
 * It returns true if all is ok.
 * If region index is a vector it should has given number of elements.
 */
bool testRegionIndexForSizeMismatch(const llvm::Value *const V,
  const unsigned& Width, const unsigned& NumElements) {

  bool Result = true;

  auto *const VT = dyn_cast<IGCLLVM::FixedVectorType>(V->getType());

  if (VT) {
    const unsigned VectorElements = (VT->getNumElements() * Width);
    Result = (VectorElements == NumElements);
    IGC_ASSERT_MESSAGE(Result, "vector region index size mismatch");
  }

  return Result;
}

/***********************************************************************
 * Local function for testing one assertion statement.
 * It returns true if all is ok.
 * Instruction 'or' should be changed to 'add' without any errors.
 */
bool testOperator(const llvm::Instruction *const Operator) {

  IGC_ASSERT(Operator);
  IGC_ASSERT(Operator->getModule());

  const unsigned Opcode = Operator->getOpcode();
  const bool IsAdd = (Instruction::Add == Opcode);
  const bool IsSub = (Instruction::Sub == Opcode);
  const bool IsOr = (Instruction::Or == Opcode);
  const bool IsAddAddr =
    (GenXIntrinsic::genx_add_addr == GenXIntrinsic::getGenXIntrinsicID(Operator));

  bool Result = (IsAdd || IsSub || IsOr || IsAddAddr);
  IGC_ASSERT_MESSAGE(Result,
    "your offset seems to be calculated not through ADD or OR");

  // check if instruction 'or' could be changed to 'add'
  if (Result && IsOr) {
    const llvm::Value *const Op0 = Operator->getOperand(0);
    const llvm::Value *const Op1 = Operator->getOperand(1);
    const DataLayout &DL = Operator->getModule()->getDataLayout();

    Result = llvm::haveNoCommonBitsSet(Op0, Op1, DL);
    IGC_ASSERT_MESSAGE(Result, "OR should be changed to ADD with no errors");
  }

  return Result;
}

} // namespace

static cl::opt<bool>
    AdjustValidWidthForTarget("adj-width-for-target", cl::Hidden,
                              cl::init(false),
                              cl::desc("Adjust valid width on the CM side"));

/***********************************************************************
 * makeRegionWithOffset: get a Region given a rdregion/wrregion, baling in
 * constant add of offset
 *
 * This constructs the Region with a variable index that is a constant add
 * baled in (i.e. Region::Indirect and Region::Offset both set to the
 * operands of the add). It is for use when baling information is not
 * available, but the caller wants the constant offset separated out like
 * that.
 */
Region genx::makeRegionWithOffset(const Instruction *Inst,
                                  bool WantParentWidth) {
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
    IGC_ASSERT_EXIT_MESSAGE(0, "not rdregion or wrregion");
    break;
  }
  BaleInfo BI;
  if (GenXBaling::isBalableIndexAdd(Inst->getOperand(OperandNum)))
    BI.setOperandBaled(OperandNum);
  return makeRegionFromBaleInfo(Inst, BI, WantParentWidth);
}

/***********************************************************************
 * Region constructor from a rd/wr region and its BaleInfo
 * This also works with rdpredregion and wrpredregion, with Offset in
 * bits rather than bytes, and with ElementBytes set to 1.
 */
Region genx::makeRegionFromBaleInfo(const Instruction *Inst, const BaleInfo &BI,
                                    bool WantParentWidth) {
  Region Result;
  // Determine where to get the subregion value from and which arg index
  // the region parameters start at.
  unsigned ArgIdx = 0;
  const Value *Subregion = nullptr;
  IGC_ASSERT(isa<CallInst>(Inst));
  auto CallI = cast<CallInst>(Inst);
  IGC_ASSERT(CallI->getCalledFunction());
  switch (GenXIntrinsic::getGenXIntrinsicID(CallI->getCalledFunction())) {
    case GenXIntrinsic::genx_rdpredregion:
      Result.NumElements =
          cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
      Result.Width = Result.NumElements;
      Result.Offset = cast<ConstantInt>(Inst->getOperand(1))->getZExtValue();
      Result.ElementBytes = 1;
      return Result;
    case GenXIntrinsic::genx_wrpredregion:
      Result.NumElements =
          cast<IGCLLVM::FixedVectorType>(Inst->getOperand(1)->getType())
              ->getNumElements();
      Result.Width = Result.NumElements;
      Result.Offset = cast<ConstantInt>(Inst->getOperand(2))->getZExtValue();
      Result.ElementBytes = 1;
      return Result;
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
      Result.Mask =
          Inst->getOperand(GenXIntrinsic::GenXRegion::PredicateOperandNum);
      if (auto C = dyn_cast<Constant>(Result.Mask))
        if (C->isAllOnesValue())
          Result.Mask = 0;
      break;
    default:
      IGC_ASSERT(0);
  }
  // Get the region parameters.
  IGC_ASSERT(Subregion);
  Result.ElementTy = Subregion->getType();
  if (auto *VT = dyn_cast<IGCLLVM::FixedVectorType>(Result.ElementTy)) {
    Result.ElementTy = VT->getElementType();
    Result.NumElements = VT->getNumElements();
  }
  const DataLayout &DL = Inst->getModule()->getDataLayout();
  static_assert(genx::ByteBits);
  IGC_ASSERT(DL.getTypeSizeInBits(Result.ElementTy) % genx::ByteBits == 0);
  Result.ElementBytes = DL.getTypeSizeInBits(Result.ElementTy) / genx::ByteBits;
  Result.VStride = cast<ConstantInt>(Inst->getOperand(ArgIdx))->getSExtValue();
  Result.Width =
      cast<ConstantInt>(Inst->getOperand(ArgIdx + 1))->getSExtValue();
  Result.Stride =
      cast<ConstantInt>(Inst->getOperand(ArgIdx + 2))->getSExtValue();
  ArgIdx += 3;
  // Get the start index.
  Value *V = Inst->getOperand(ArgIdx);
  IGC_ASSERT_MESSAGE(V->getType()->getScalarType()->isIntegerTy(16),
    "region index must be i16 or vXi16 type");
  IGC_ASSERT(
      testRegionIndexForSizeMismatch(V, Result.Width, Result.NumElements));

  if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
    Result.Offset = CI->getSExtValue(); // Constant index.
  else {
    Result.Indirect = V; // Index is variable; assume no baled in add.
    if (BI.isOperandBaled(ArgIdx)) {
      Instruction *Operator = cast<Instruction>(V);
      // The index is variable and has something baled in. We want to process
      // a baled in add or add_addr, and ignore a baled in rdregion.
      if(!GenXIntrinsic::isRdRegion(Operator)) {
        // The index is variable and has a baled in or/add/sub/add_addr.
        // offset is calculated through 'add' or 'or'
        IGC_ASSERT(testOperator(Operator));

        Constant *C = cast<Constant>(Operator->getOperand(1));
        ConstantInt *CI = dyn_cast<ConstantInt>(C);
        if (!CI)
          CI = cast<ConstantInt>(C->getSplatValue());

        Result.Offset = CI->getSExtValue();

        if (Operator->getOpcode() == Instruction::Sub)
          Result.Offset = -Result.Offset;

        Result.Indirect = Operator->getOperand(0);
      }
    }
    // For a variable index, get the parent width arg.
    ConstantInt *PW = dyn_cast<ConstantInt>(Inst->getOperand(ArgIdx + 1));
    if (PW)
      Result.ParentWidth = PW->getZExtValue();
  }
  // We do some trivial legalization here. The legalization pass does not
  // make these changes; instead we do them here so they are not permanently
  // written back into the IR but are made on the fly each time some other
  // pass uses this code to get the region info.
  if (Result.NumElements == 1) {
    Result.Width = Result.Stride = 1;
    Result.VStride = 0;
  } else {
    if (Result.NumElements <= Result.Width) {
      Result.Width = Result.NumElements;
      Result.VStride = 0;
    } else if ((unsigned)Result.VStride == Result.Width * Result.Stride) {
      // VStride == Width * Stride, so we can canonicalize to a 1D region,
      // but only if not indirect or not asked to preserve parentwidth,
      // and never if multi-indirect.
      if (!Result.Indirect ||
          (!isa<VectorType>(Result.Indirect->getType()) && !WantParentWidth)) {
        Result.Width = Result.NumElements;
        Result.VStride = 0;
        Result.ParentWidth = 0;
      }
    } else if (Result.Width == 1) {
      // We can turn a 2D width 1 region into a 1D region, but if it is
      // indirect it invalidates ParentWidth. So only do it if not asked
      // to keep ParentWidth. Also we cannot do it if it is multi-indirect.
      if (!Result.Indirect ||
          (!isa<VectorType>(Result.Indirect->getType()) && !WantParentWidth)) {
        Result.Width = Result.NumElements;
        Result.Stride = Result.VStride;
        Result.VStride = 0;
        Result.ParentWidth = 0;
      }
    }
    if (Result.Stride == 0 && Result.Width == Result.NumElements) {
      // Canonical scalar region.
      Result.Width = 1;
      Result.VStride = 0;
    }
  }
  return Result;
}

/***********************************************************************
 * Region::getLegalRegionSizeForTarget: get the max legal size of a region
 *
 * Enter:   Idx = start index into the subregion
 *          Allow2D = whether to allow 2D region
 *          InputNumElements = number of elements in whole input vector (so
 *                we can tell if it is small enough that it cannot possibly
 *                cross a GRF boundary)
 *          ST = GenXSubtarget (so we can get gen specific crossing rules)
 *          AI = 0 else AlignmentInfo (to determine alignment of indirect index)
 */
unsigned genx::getLegalRegionSizeForTarget(const GenXSubtarget &ST,
                                           const Region &R, unsigned Idx,
                                           bool Allow2D,
                                           unsigned InputNumElements,
                                           AlignmentInfo *AI) {
  Alignment Align;
  if (R.Indirect) {
    Align = Alignment::getUnknown();
    if (AI)
      Align = AI->get(R.Indirect);
  }
  return getLegalRegionSizeForTarget(ST, R, Idx, Allow2D, InputNumElements,
                                     Align);
}

/***********************************************************************
 * Region::getLegalRegionSizeForTarget : get the max legal size of a region
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
unsigned genx::getLegalRegionSizeForTarget(const GenXSubtarget &ST,
                                           const Region &R, unsigned Idx,
                                           bool Allow2D,
                                           unsigned InputNumElements,
                                           Alignment Align) {
  // Determine the max valid width.
  unsigned ValidWidth = 1;
  unsigned GRFByteSize = ST.getGRFByteSize();
  int MaxStride = 4;
  unsigned LogGRFWidth = genx::log2(GRFByteSize);
  if ((!R.Stride || exactLog2(R.Stride) >= 0) &&
      (Allow2D || R.Stride <= MaxStride)) {
    // The stride is legal, so we can potentially do more than one element at a
    // time.
    // Disallow 2D if the stride is too large for a real Gen region. For a
    // source operand (Allow2D is true), we allow a 1D region with stride too
    // large, because the vISA writer turns it into a 2D region with width 1.
    bool StrideValid = (R.Stride <= MaxStride);

    if (R.Indirect && isa<VectorType>(R.Indirect->getType())) {
      // Multi indirect.
      if (!Allow2D) {
        // Multi indirect not allowed in wrregion.
        if (!R.Stride)
          ValidWidth = 1 << genx::log2(R.Width);
      } else if (R.Width == 1 || !R.Stride) {
        // Multi indirect with width 1 or stride 0.
        // Return the max power of two number of elements that:
        // 1. fit in 2 GRFs; and
        // 2. fit in the whole region; and
        // 3. fit in a row if the width is not legal
        // 4. no more than 8 elements in multi indirect (because there
        //    are only 8 elements in an address register).
        unsigned LogWidth = genx::log2(Width);
        if (1U << LogWidth == Width)
          LogWidth = genx::log2(R.NumElements); // legal width
        unsigned LogElementBytes = genx::log2(R.ElementBytes);
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
      unsigned ElementsPerGRF = GRFByteSize / R.ElementBytes;
      unsigned ElementsToBoundary = 1;
      unsigned RealIdx = Idx / R.Width * R.VStride + Idx % R.Width * R.Stride;
      if (!R.Indirect) {
        unsigned OffsetElements = R.getOffsetInElements();
        // For a direct operand, just use the constant offset of the
        // region and the index so far to calculate how far into a GRF this
        // subregion starts, and set the boundary at the next-but-one GRF
        // boundary.
        unsigned NumGRF = 2;
        // For PVC it's legal to read only from one GFR in byte source
        if (ST.isPVC() && R.ElementBytes == genx::ByteBytes)
          NumGRF = 1;
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
        // operand or is a 1D source operand (so GenXCisaBuilder can turn it
        // into Nx1 instead of 1xN).  We use Allow2D as a proxy for "is source
        // operand".
        unsigned GRFsPerIndirect =
            genx::getNumGRFsPerIndirectForRegion(R, &ST, Allow2D);
        unsigned Last = (R.NumElements / R.Width - 1) * R.VStride +
                        (R.Width - 1) * R.Stride;
        unsigned Max = InputNumElements - Last - 1 + RealIdx;
        unsigned Min = RealIdx;
        unsigned MinMaxGRFDiff = (Max & -ElementsPerGRF) - (Min & -ElementsPerGRF);
        if (!MinMaxGRFDiff) // min and max in same GRF
          ElementsToBoundary = ElementsPerGRF * GRFsPerIndirect
              - (Max & (ElementsPerGRF - 1));
        else if (MinMaxGRFDiff == 1 && GRFsPerIndirect > 1)
          ElementsToBoundary = ElementsPerGRF - (Max & (ElementsPerGRF - 1));
        // We may be able to refine an indirect region legal width further...
        if (exactLog2(R.ParentWidth) >= 0 &&
            R.ParentWidth <= GRFsPerIndirect * ElementsPerGRF) {
          // ParentWidth tells us that a row of our region cannot cross a
          // possible number of elements addressed by indirect region. Say that
          // the boundary is at the next multiple of ParentWidth.
          ElementsToBoundary = std::max(R.ParentWidth - RealIdx % R.ParentWidth,
                                        ElementsToBoundary);
        } else if (!isa<VectorType>(R.Indirect->getType())) {
          // Use the alignment+offset of the single indirect index, with alignment
          // limited to one GRF.
          if (!Align.isUnknown()) {
            unsigned LogAlign = Align.getLogAlign();
            unsigned ExtraBits = Align.getExtraBits();
            ExtraBits += (R.Offset + RealIdx * R.ElementBytes);
            ExtraBits &= ((1 << LogAlign) - 1);
            if (LogAlign >= LogGRFWidth && !ExtraBits) {
              // Start is GRF aligned, so legal width is 1 GRF for <=BDW or
              // 2 GRFs for >=SKL.
              ElementsToBoundary = ElementsPerGRF * GRFsPerIndirect;
            } else if (LogAlign > (unsigned)genx::log2(R.ElementBytes) ||
                       (LogAlign == (unsigned)genx::log2(R.ElementBytes) &&
                        ExtraBits == 0)) {
              LogAlign =
                  std::min(LogGRFWidth, LogAlign) - genx::log2(R.ElementBytes);
              ExtraBits =
                  (ExtraBits & (GRFByteSize - 1)) >> genx::log2(R.ElementBytes);
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
        if ((!R.VStride || exactLog2(R.VStride) >= 0) &&
            exactLog2(R.Width) >= 0 && R.Width <= 16 && !(Idx % R.Width) &&
            ElementsToBoundary >= (R.Width - 1) * R.Stride + 1) {
          // The vstride and width are legal, and we're at the start of a
          // row, and ElementsToBoundary is big enough for at least one
          // whole row, so we can potentially do more than one whole row at a
          // time. See how many we can fit, without including the "slack"
          // at the end of the last row.
          unsigned NumRows = 0;
          if (R.VStride == 0) // Avoid divide by 0
            NumRows = (R.NumElements - Idx) / R.Width;
          else {
            unsigned LastElementOfRow = (R.Width - 1) * R.Stride;
            unsigned Slack = R.VStride - (LastElementOfRow + 1);
            NumRows = (ElementsToBoundary + Slack) / R.VStride;
            if (NumRows) {
              if (NumRows * R.Width + Idx > R.NumElements)
                NumRows = (R.NumElements - Idx) / R.Width;
            }
          }
          ValidWidth = (1 << genx::log2(NumRows)) * R.Width;
        }
        if (ValidWidth == 1 && Idx % R.Width) {
          // That failed. See if we can legally get to the end of the row then
          // the same number of elements again at the start of the next row.
          unsigned ToEndOfRow = R.Width - Idx % R.Width;
          if (exactLog2(ToEndOfRow) >= 0 && ToEndOfRow <= 16) {
            unsigned NewVStride = R.VStride + (ToEndOfRow - R.Width) * R.Stride;
            if (exactLog2(NewVStride) >= 0 &&
                NewVStride + (ToEndOfRow - 1) * R.Stride < ElementsToBoundary) {
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
        ValidWidth = R.Width - Idx % R.Width;
        if (ValidWidth * R.Stride - (R.Stride - 1) > ElementsToBoundary)
          ValidWidth = (ElementsToBoundary + R.Stride - 1) / R.Stride;
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
  if (AdjustValidWidthForTarget)
    if (ST.isPVC() && R.is2D()) {
      while ((ValidWidth * R.ElementBytes) >= 64)
        ValidWidth /= 2;
    }

  // Some targets do not have multi indirect byte regioning and in general case
  // transformation from multi indirect region to indirect is possible for
  // regions with width = 1.
  if (R.isMultiIndirect() && R.ElementBytes == genx::ByteBytes &&
      !ST.hasMultiIndirectByteRegioning())
    ValidWidth = 1;
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
                                          GenXBaling *Baling) {
  StartWr = ArgStartWr;
  auto Wr = StartWr;
  IGC_ASSERT(GenXIntrinsic::isWrRegion(Wr));
  IGC_ASSERT(Baling);
  Region TotalWrR = genx::makeRegionFromBaleInfo(Wr, Baling->getBaleInfo(Wr));
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
    Region TotalRdR = makeRegionFromBaleInfo(Rd, Baling->getBaleInfo(Rd));
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
      if (!TotalRdR.append(
              makeRegionFromBaleInfo(Rd, Baling->getBaleInfo(Rd))) ||
          !TotalWrR.append(makeRegionFromBaleInfo(Wr, Baling->getBaleInfo(Wr))))
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
    if (!TotalRdR.append(InR) ||
        !TotalWrR.append(makeRegionFromBaleInfo(Wr, Baling->getBaleInfo(Wr))))
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
  IGC_ASSERT(GenXIntrinsic::isWrRegion(Wr));
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
    IGC_ASSERT(Wr);
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
  IGC_ASSERT(GenXIntrinsic::isRdRegion(Rd));
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
  IGC_ASSERT(GenXIntrinsic::isRdRegion(Rd));
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
  const unsigned OpIdx = GenXIntrinsic::GenXRegion::OldValueOperandNum;
  IGC_ASSERT(Rd);
  IGC_ASSERT(Rd->getOperand(OpIdx) == Input);
  return &Rd->getOperandUse(OpIdx);
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

static Instruction* simplifyConstIndirectRegion(Instruction* Inst) {
  // if a region has a constant-vector as its indirect offsets,
  // try to recognize the pattern, and replace it with
  // a direct region with v-stride, h-stride, h-width
  Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
  if (R.Indirect == nullptr)
    return Inst;

  auto cv = dyn_cast<ConstantDataVector>(R.Indirect);
  if (!cv)
    return Inst;
  // Flatten the vector out into the elements array
  llvm::SmallVector<llvm::Constant*, 16> elements;
  auto vectorLength =
      cast<IGCLLVM::FixedVectorType>(cv->getType())->getNumElements();
  for (unsigned i = 0; i < vectorLength; ++i)
    elements.push_back(cv->getElementAsConstant(i));

  llvm::ConstantInt* ci = llvm::dyn_cast<llvm::ConstantInt>(elements[0]);
  if (ci == NULL)
    return Inst; // Not a vector of integers

  int VStride = 1;
  unsigned Width = 1;
  int Stride = 1;
  int Offset = (-1);

  int64_t val0 = ci->getSExtValue();
  if (vectorLength == 1) {
    R.Indirect = nullptr;
    R.Offset = val0 + R.Offset;
    R.Stride = 0;
    R.Width = 1;
    R.VStride = 0;
    if (GenXIntrinsic::isRdRegion(Inst))
      return R.createRdRegion(Inst->getOperand(0),
        Inst->getName(), Inst, Inst->getDebugLoc());
    else if (GenXIntrinsic::isWrRegion(Inst))
      return R.createWrRegion(Inst->getOperand(0), Inst->getOperand(1),
                              Inst->getName(), Inst, Inst->getDebugLoc());
    return Inst;
  }
  ci = llvm::dyn_cast<llvm::ConstantInt>(elements[1]);
  if (ci == NULL)
    return Inst; // Not a vector of integers

  int64_t prevVal = ci->getSExtValue();
  int64_t diff = prevVal - val0;
  if (diff < 0)
    return Inst; // cannot have negative stride
  int i0 = 0;
  Offset = val0 + R.Offset;
  // check if this is a 1d simple-stride region
  for (int i = 2; i < (int)vectorLength; ++i) {
    ci = llvm::dyn_cast<llvm::ConstantInt>(elements[i]);
    if (ci == NULL)
      return Inst;

    int64_t nextVal = ci->getSExtValue();
    if (prevVal + diff != nextVal) {
      if (Width == 1) {
        Width = i - i0;
        VStride = nextVal - val0;
        val0 = nextVal;
        i0 = i;
      }
      else if (nextVal != val0 + VStride || i != i0 + Width)
        return Inst;
      else {
        val0 = nextVal;
        i0 = i;
      }
      if (VStride < 0)
        return Inst; // cannot have negative stride
    }
    prevVal = nextVal;
  }
  Stride = diff*8 / R.ElementTy->getPrimitiveSizeInBits();
  VStride = VStride*8 / R.ElementTy->getPrimitiveSizeInBits();
  // rewrite the region inst
  R.Indirect = nullptr;
  R.Offset = Offset;
  R.Stride = Stride;
  R.Width = (Width == 1) ? R.NumElements : Width;
  R.VStride = VStride;
  if (GenXIntrinsic::isRdRegion(Inst))
    return R.createRdRegion(Inst->getOperand(0),
      Inst->getName(), Inst, Inst->getDebugLoc());
  else if (GenXIntrinsic::isWrRegion(Inst))
    return R.createWrRegion(Inst->getOperand(0), Inst->getOperand(1),
                            Inst->getName(), Inst, Inst->getDebugLoc());
  return Inst;
}

static Optional<std::pair<IGCLLVM::FixedVectorType *, Region>>
convertRegionInstType(Instruction *Inst, Type *NewScalarTy,
                      const DataLayout &DL, const GenXSubtarget &ST) {
  using namespace GenXIntrinsic::GenXRegion;
  IGC_ASSERT(GenXIntrinsic::isRdRegion(Inst) ||
             GenXIntrinsic::isWrRegion(Inst));
  auto *OldVal = Inst->getOperand(OldValueOperandNum);
  // Do not change register category to predicate.
  if (NewScalarTy->isIntegerTy(1))
    return None;
  auto *NewVecTy = genx::changeVectorType(OldVal->getType(), NewScalarTy, &DL);
  if (!NewVecTy)
    return None;
  Region R = makeRegionFromBaleInfo(Inst, BaleInfo());
  if (!R.changeElementType(NewScalarTy, &DL))
    return None;
  // Transformation is not profitable for 2D regions or if it will require
  // legalization.
  if (R.is2D() || R.NumElements > llvm::PowerOf2Floor(
                                      genx::getExecSizeAllowedBits(Inst, &ST)))
    return None;
  return std::make_pair(NewVecTy, R);
}

// fold bitcast with wrregion:
//                                  ==>  %oldval.cast = bitcast(%oldval)
// %2 = bitcast(%1)                      %3 = wrregion(%oldval.cast, %1, ...)
// %3 = wrregion(%oldval, %2, ...)       %2 = bitcast(%3)
// so it can be baled later.
static Value *simplifyBitCastWithRegionWrite(Instruction *WrR,
                                             const DataLayout &DL,
                                             const GenXSubtarget &ST) {
  using namespace GenXIntrinsic::GenXRegion;
  IGC_ASSERT(GenXIntrinsic::isWrRegion(WrR));
  auto *OldVal = WrR->getOperand(OldValueOperandNum);
  if (GenXIntrinsic::isReadWritePredefReg(OldVal))
    return nullptr;
  Value *NewVal = WrR->getOperand(NewValueOperandNum);
  auto *BCI = dyn_cast<BitCastInst>(NewVal);
  if (!BCI)
    return nullptr;
  if (WrR->hasOneUse() && GenXIntrinsic::isWritePredefReg(WrR->user_back()))
    return nullptr;
  auto *NewScalarTy = BCI->getSrcTy()->getScalarType();
  auto ConvertRes = convertRegionInstType(WrR, NewScalarTy, DL, ST);
  if (!ConvertRes)
    return nullptr;
  auto [NewVecTy, R] = *ConvertRes;
  IRBuilder<TargetFolder> IRB(WrR->getParent(), BasicBlock::iterator(WrR),
                              TargetFolder(DL));
  IGC_ASSERT(vc::isBitCastAllowed(*OldVal, *NewVecTy));
  auto *OldValCast =
      IRB.CreateBitCast(OldVal, NewVecTy, OldVal->getName() + ".cast");
  auto *NewWrR = R.createWrRegion(OldValCast, BCI->getOperand(0),
                                  WrR->getName(), WrR, WrR->getDebugLoc());
  auto *NewBCI = IRB.CreateBitCast(NewWrR, WrR->getType(), BCI->getName());
  return NewBCI;
}

static Value *simplifyRegionWrite(Instruction *WrR, const DataLayout *DL) {
  using namespace GenXIntrinsic::GenXRegion;
  IGC_ASSERT(GenXIntrinsic::isWrRegion(WrR));
  Value *NewVal = WrR->getOperand(NewValueOperandNum);

  // Replace C with B if R - whole region
  // C = wrregion(A, B, R)
  if (auto R = makeRegionFromBaleInfo(WrR, BaleInfo());
      R.isWhole(WrR->getType(), DL) && !R.Mask &&
      vc::isBitCastAllowed(*NewVal, *WrR->getType()) &&
      !isPredefRegDestination(WrR) && !isPredefRegSource(NewVal))
    return IRBuilder<>(WrR).CreateBitCast(NewVal, WrR->getType(), WrR->getName());
  // Replace C with A
  // C = wrregion(A, undef, R)
  if (isa<UndefValue>(NewVal))
    return WrR->getOperand(OldValueOperandNum);

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
    Instruction *RdR = cast<Instruction>(NewVal);
    Region InnerR = makeRegionFromBaleInfo(RdR, BaleInfo());
    Region OuterR = makeRegionFromBaleInfo(WrR, BaleInfo());
    if (OuterR != InnerR)
      return nullptr;

    auto OldValRdR = RdR->getOperand(OldValueOperandNum);
    if (GenXIntrinsic::isReadPredefReg(OldValRdR))
      return nullptr;
    auto OldValWrR = WrR->getOperand(OldValueOperandNum);
    if ((isa<UndefValue>(OldValWrR) &&
         OldValRdR->getType() == OldValWrR->getType()) ||
        OldValRdR == OldValWrR)
      return OldValRdR;
  }
  return nullptr;
}

// fold bitcast with rdregion:
// %2 = rdregion(%1, ...)  ==>  %3 = bitcast(%1)
// %3 = bitcast(%2)             %2 = rdregion(%3, ...)
// so it can be baled later.
static Value *simplifyBitCastFromRegionRead(BitCastInst *BCI,
                                            const DataLayout &DL,
                                            const GenXSubtarget &ST) {
  using namespace GenXIntrinsic::GenXRegion;
  // TODO: fix this, as rdregion can return scalar
  // for cases
  //%2 =  <1 x i32> rdregion
  //%3 = bitcast <1 x i32> %2 to i32
  if (!BCI->getType()->isVectorTy())
    return nullptr;
  Instruction *RdR = dyn_cast<Instruction>(BCI->getOperand(0));
  if (!RdR || !GenXIntrinsic::isRdRegion(RdR) || !RdR->hasOneUse())
    return nullptr;
  auto *OldVal = RdR->getOperand(OldValueOperandNum);
  if (GenXIntrinsic::isReadPredefReg(OldVal))
    return nullptr;
  auto *NewScalarTy = BCI->getDestTy()->getScalarType();
  auto ConvertRes = convertRegionInstType(RdR, NewScalarTy, DL, ST);
  if (!ConvertRes)
    return nullptr;
  auto [NewVecTy, R] = *ConvertRes;
  IGC_ASSERT(vc::isBitCastAllowed(*OldVal, *NewVecTy));
  IRBuilder<TargetFolder>(BCI->getParent(), BasicBlock::iterator(BCI),
                          TargetFolder(DL));
  auto *NewBCI =
      IRBuilder<TargetFolder>(BCI->getParent(), BasicBlock::iterator(BCI),
                              TargetFolder(DL))
          .CreateBitCast(OldVal, NewVecTy, BCI->getName());
  auto *NewRdR =
      R.createRdRegion(NewBCI, RdR->getName(), BCI, RdR->getDebugLoc());
  return NewRdR;
}

static Value *simplifyRegionRead(Instruction *Inst, const DataLayout *DL) {
  IGC_ASSERT(GenXIntrinsic::isRdRegion(Inst));
  Value *Input = Inst->getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
  if (makeRegionFromBaleInfo(Inst, BaleInfo()).isWhole(Input->getType(), DL) &&
      vc::isBitCastAllowed(*Input, *Inst->getType()) &&
      !genx::isPredefRegSource(Inst))
    return IRBuilder<>(Inst).CreateBitCast(Input, Inst->getType(),
                                           Inst->getName());
  if (isa<UndefValue>(Input))
    return UndefValue::get(Inst->getType());
  else if (auto C = dyn_cast<Constant>(Input)) {
    if (auto Splat = C->getSplatValue()) {
      if (auto *Ty = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType()))
        Splat = ConstantVector::getSplat(
            IGCLLVM::getElementCount(Ty->getNumElements()), Splat);
      return Splat;
    }
  } else if (GenXIntrinsic::isWrRegion(Input) && Input->hasOneUse()) {
    // W = wrr(A, B, R)
    // C = rdr(W, R)
    // =>
    // replace C by B
    Instruction *WI = cast<Instruction>(Input);
    Region R1 = makeRegionFromBaleInfo(WI, BaleInfo());
    Region R2 = makeRegionFromBaleInfo(Inst, BaleInfo());
    if (R1 == R2) {
      Value *B = WI->getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      if (B->getType() == Inst->getType())
        return B;
    }
  }
  return nullptr;
}

// Simplify a region read or write.
Value *llvm::genx::simplifyRegionInst(Instruction *Inst, const DataLayout *DL,
                                      const GenXSubtarget *ST) {
  if (Inst->use_empty())
    return nullptr;

  unsigned ID = GenXIntrinsic::getGenXIntrinsicID(Inst);
  switch (ID) {
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_rdregionf:
  case GenXIntrinsic::genx_rdregioni: {
    auto replace = simplifyConstIndirectRegion(Inst);
    if (replace != Inst) {
      Inst->replaceAllUsesWith(replace);
      Inst = replace;
    }
    break;
  }
  default:
    break;
  }
  if (Constant *C = ConstantFoldGenX(Inst, *DL))
    return C;

  if (auto *BCI = dyn_cast<BitCastInst>(Inst); BCI && DL && ST)
    return simplifyBitCastFromRegionRead(BCI, *DL, *ST);
  ID = GenXIntrinsic::getGenXIntrinsicID(Inst);
  switch (ID) {
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_wrregioni:
    if (auto *Res = simplifyRegionWrite(Inst, DL))
      return Res;
    if (DL && ST)
      return simplifyBitCastWithRegionWrite(Inst, *DL, *ST);
    break;
  case GenXIntrinsic::genx_rdregionf:
  case GenXIntrinsic::genx_rdregioni:
    return simplifyRegionRead(Inst, DL);
  default:
    break;
  }
  return nullptr;
}

bool llvm::genx::simplifyRegionInsts(Function *F, const DataLayout *DL,
                                     const GenXSubtarget *ST) {
  bool Changed = false;
  for (auto &BB : F->getBasicBlockList()) {
    for (auto I = BB.begin(); I != BB.end();) {
      Instruction *Inst = &*I++;
      if (auto V = simplifyRegionInst(Inst, DL, ST)) {
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
        auto *GV = vc::getUnderlyingGlobalVariable(SI->getPointerOperand());
        if (!GV)
          continue;
        // Kill all live loads on this variable.
        DomLoads[GV].clear();
      } else if (auto LI = dyn_cast<LoadInst>(Inst)) {
        auto *GV = vc::getUnderlyingGlobalVariable(LI->getPointerOperand());
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
    auto vectorLength =
        cast<IGCLLVM::FixedVectorType>(cv->getType())->getNumElements();
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
