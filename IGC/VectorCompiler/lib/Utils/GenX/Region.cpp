/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Implementation of methods for CMRegion class
//
//===----------------------------------------------------------------------===//

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

#include "vc/Utils/General/Types.h"
#include "vc/Utils/GenX/Region.h"
#include "vc/Utils/GenX/TypeSize.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace vc;

// Find the datalayout if possible.
static const DataLayout *getDL(const Value *V) {
  IGC_ASSERT(V);
  if (auto *Inst = dyn_cast_or_null<Instruction>(V))
    return &Inst->getModule()->getDataLayout();
  if (auto *Arg = dyn_cast_or_null<Argument>(V))
    return &Arg->getParent()->getParent()->getDataLayout();
  return nullptr;
}

/***********************************************************************
 * Region constructor from a type
 */
CMRegion::CMRegion(Type *Ty, const DataLayout *DL)
    : ElementBytes(0), ElementTy(0), NumElements(1), VStride(0), Width(1),
      Stride(1), Offset(0), Indirect(0), IndirectIdx(0), IndirectAddrOffset(0),
      Mask(0), ParentWidth(0)
{
  IGC_ASSERT_MESSAGE(!Ty->isAggregateType(),
    "cannot create region based on an aggregate type");
  ElementTy = Ty;
  if (IGCLLVM::FixedVectorType *VT =
          dyn_cast<IGCLLVM::FixedVectorType>(ElementTy)) {
    ElementTy = VT->getElementType();
    NumElements = VT->getNumElements();
    Width = NumElements;
  }
  if (DL) {
    unsigned BitSize = DL->getTypeSizeInBits(ElementTy);
    ElementBytes = alignTo<8>(BitSize) / 8;
  } else {
    unsigned BitSize = ElementTy->getPrimitiveSizeInBits();
    ElementBytes = alignTo<8>(BitSize) / 8;
    IGC_ASSERT_MESSAGE(ElementBytes,
      "Cannot compute element size without data layout");
  }
}

/***********************************************************************
 * Region constructor from a value
 */
CMRegion::CMRegion(const Value *V, const DataLayout *DL)
    : CMRegion(V->getType(), DL ? DL : getDL(V)) {}

/***********************************************************************
 * Region constructor from a rd/wr region and its BaleInfo
 * This also works with rdpredregion and wrpredregion, with Offset in
 * bits rather than bytes, and with ElementBytes set to 1.
 */
CMRegion::CMRegion(const Instruction *Inst, bool WantParentWidth)
    : ElementBytes(0), ElementTy(0), NumElements(1), VStride(1), Width(1),
      Stride(1), Offset(0), Indirect(0), IndirectIdx(0), IndirectAddrOffset(0),
      Mask(0), ParentWidth(0) {
  // Determine where to get the subregion value from and which arg index
  // the region parameters start at.
  unsigned ArgIdx = 0;
  const Value *Subregion = 0;
  IGC_ASSERT(isa<CallInst>(Inst));
  switch (GenXIntrinsic::getGenXIntrinsicID(Inst)) {
    case GenXIntrinsic::genx_rdpredregion:
      NumElements =
          cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
      Width = NumElements;
      Offset = cast<ConstantInt>(Inst->getOperand(1))->getZExtValue();
      ElementBytes = 1;
      return;
    case GenXIntrinsic::genx_wrpredregion:
      NumElements =
          cast<IGCLLVM::FixedVectorType>(Inst->getOperand(1)->getType())
              ->getNumElements();
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
      IGC_ASSERT(0);
      break;
  }
  // Get the region parameters.
  IGC_ASSERT_EXIT(Subregion);
  ElementTy = Subregion->getType();
  if (IGCLLVM::FixedVectorType *VT =
          dyn_cast<IGCLLVM::FixedVectorType>(ElementTy)) {
    ElementTy = VT->getElementType();
    NumElements = VT->getNumElements();
  }
  ElementBytes = ElementTy->getPrimitiveSizeInBits() / 8;
  if (ElementTy->getPrimitiveSizeInBits())
    ElementBytes = ElementBytes ? ElementBytes : 1;
  VStride = cast<ConstantInt>(Inst->getOperand(ArgIdx))->getSExtValue();
  Width = cast<ConstantInt>(Inst->getOperand(ArgIdx + 1))->getSExtValue();
  Stride = cast<ConstantInt>(Inst->getOperand(ArgIdx + 2))->getSExtValue();
  ArgIdx += 3;
  // Get the start index.
  Value *V = Inst->getOperand(ArgIdx);
  IGC_ASSERT_MESSAGE(V->getType()->getScalarType()->isIntegerTy(16),
    "region index must be i16 or vXi16 type");

  if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
    Offset = CI->getSExtValue(); // Constant index.
  else {
    Indirect = V; // Index is variable; assume no baled in add.
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
 * Region constructor from bitmap of which elements to set
 *
 * Enter:   Bits = bitmap of which elements to set
 *          ElementBytes = bytes per element
 *
 * It is assumed that Bits represents a legal 1D region.
 */
CMRegion::CMRegion(unsigned Bits, unsigned ElementBytes)
    : ElementBytes(ElementBytes), ElementTy(0), NumElements(1), VStride(1),
      Width(1), Stride(1), Offset(0), Indirect(0), IndirectIdx(0),
      IndirectAddrOffset(0), Mask(0), ParentWidth(0)
{
  IGC_ASSERT(Bits);
  Offset = llvm::countTrailingZeros(Bits);

  // It might seem problematic that countTrailingZeros may return 32 and that we could do shift by 32 here.
  // But countTrailingZeros would return 32 in scenario where "Bits" variable is 0
  // And IGC_ASSERT(Bits) covers this case... in debug mode, at least.
  if (Offset < std::numeric_limits<unsigned>::digits) {
    Bits >>= Offset;
    Offset *= ElementBytes;

    if (Bits != 1) {
      Stride = llvm::countTrailingZeros(Bits & ~1);
      NumElements = Width = countPopulation(Bits);
    }
  }
}

/***********************************************************************
 * CMRegion::getSubregion : modify Region struct for a subregion
 *
 * Enter:   StartIdx = start index of subregion (in elements)
 *          Size = size of subregion (in elements)
 *
 * This does not modify the Mask; the caller needs to do that separately.
 */
void CMRegion::getSubregion(unsigned StartIdx, unsigned Size)
{
  if (Indirect && isa<VectorType>(Indirect->getType())) {
    // Vector indirect (multi indirect). Set IndirectIdx to the index of
    // the start element in the vector indirect.
    IndirectIdx = StartIdx / Width;
    StartIdx %= Width;
  }
  int AddOffset = StartIdx / Width * VStride;
  AddOffset += StartIdx % Width * Stride;
  AddOffset *= ElementBytes;
  Offset += AddOffset;
  if (!(StartIdx % Width) && !(Size % Width)) {
    // StartIdx is at the start of a row and Size is a whole number of
    // rows.
  } else if (StartIdx % Width + Size > Width) {
    // The subregion goes over a row boundary. This can only happen if there
    // is only one row split and it is exactly in the middle.
    VStride += (Size / 2 - Width) * Stride;
    Width = Size / 2;
  } else {
    // Within a single row.
    Width = Size;
    VStride = Size * Stride;
  }
  NumElements = Size;
}

/***********************************************************************
 * CMRegion::createRdRegion : create rdregion intrinsic from "this" Region
 *
 * Enter:   Input = vector value to extract subregion from
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give the new instruction
 *          AllowScalar = true to return scalar if region is size 1
 *
 * Return:  newly created instruction
 */
Instruction *CMRegion::createRdRegion(Value *Input, const Twine &Name,
    Instruction *InsertBefore, const DebugLoc &DL, bool AllowScalar)
{
  IGC_ASSERT_MESSAGE(ElementBytes, "not expecting i1 element type");

  Value *StartIdx = getStartIdx(Name, InsertBefore, DL);
  IntegerType *I32Ty = Type::getInt32Ty(Input->getContext());
  Value *ParentWidthArg = UndefValue::get(I32Ty);
  if (Indirect)
    ParentWidthArg = ConstantInt::get(I32Ty, ParentWidth);
  Value *Args[] = {   // Args to new rdregion:
      Input, // input to original rdregion
      ConstantInt::get(I32Ty, VStride), // vstride
      ConstantInt::get(I32Ty, Width), // width
      ConstantInt::get(I32Ty, Stride), // stride
      StartIdx, // start index (in bytes)
      ParentWidthArg // parent width (if variable start index)
  };
  IGC_ASSERT_MESSAGE(
      ElementTy ==
          cast<IGCLLVM::FixedVectorType>(Args[0]->getType())->getElementType(),
      "Region and rdregion input operand types mismatch");
  Type *RegionTy = getRegionType(!AllowScalar);
  Module *M = InsertBefore->getModule();
  auto IID = ElementTy->isFloatingPointTy() ? GenXIntrinsic::genx_rdregionf
                                            : GenXIntrinsic::genx_rdregioni;
  Function *Decl = getGenXRegionDeclaration(M, IID, RegionTy, Args);
  Instruction *NewInst = CallInst::Create(Decl, Args, Name, InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * CMRegion::createWrRegion : create wrregion instruction for subregion
 * CMRegion::createWrConstRegion : create wrconstregion instruction for subregion
 *
 * Enter:   OldVal = vector value to insert subregion into (can be undef)
 *          Input = subregion value to insert (can be scalar, as long as
 *                  region size is 1)
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give any new instruction
 *
 * Return:  The new wrregion instruction. However, if it would have had a
 *          predication mask of all 0s, it is omitted and OldVal is returned
 *          instead.
 */
Instruction *CMRegion::createWrRegion(Value *OldVal, Value *Input,
                                      const Twine &Name,
                                      Instruction *InsertBefore,
                                      const DebugLoc &DL) {
  return createWrCommonRegion(OldVal->getType()->isFPOrFPVectorTy()
        ? GenXIntrinsic::genx_wrregionf : GenXIntrinsic::genx_wrregioni,
      OldVal, Input,
      Name, InsertBefore, DL);
}

Instruction *CMRegion::createWrConstRegion(Value *OldVal, Value *Input,
                                           const Twine &Name,
                                           Instruction *InsertBefore,
                                           const DebugLoc &DL) {
  IGC_ASSERT(!Indirect);
  IGC_ASSERT(!Mask);
  IGC_ASSERT(isa<Constant>(Input));
  return createWrCommonRegion(GenXIntrinsic::genx_wrconstregion, OldVal, Input,
      Name, InsertBefore, DL);
}

Instruction *CMRegion::createWrCommonRegion(GenXIntrinsic::ID IID,
                                            Value *OldVal, Value *Input,
                                            const Twine &Name,
                                            Instruction *InsertBefore,
                                            const DebugLoc &DL) {
  IGC_ASSERT_MESSAGE(ElementBytes, "not expecting i1 element type");
  if (isa<VectorType>(Input->getType()))
    IGC_ASSERT_MESSAGE(
        NumElements ==
            cast<IGCLLVM::FixedVectorType>(Input->getType())->getNumElements(),
        "input value and region are inconsistent");
  else
    IGC_ASSERT_MESSAGE(NumElements == 1, "input value and region are inconsistent");
  IGC_ASSERT_MESSAGE(OldVal->getType()->getScalarType() == Input->getType()->getScalarType(),
    "scalar type mismatch");
  Value *StartIdx = getStartIdx(Name, InsertBefore, DL);
  IntegerType *I32Ty = Type::getInt32Ty(Input->getContext());
  Value *ParentWidthArg = UndefValue::get(I32Ty);
  if (Indirect)
    ParentWidthArg = ConstantInt::get(I32Ty, ParentWidth);
  // Get the mask value. If R.Mask is 0, then the wrregion is unpredicated
  // and we just use constant 1.
  Value *MaskArg = Mask;
  if (!MaskArg)
    MaskArg = ConstantInt::get(Type::getInt1Ty(Input->getContext()), 1);
  // Build the wrregion.
  Value *Args[] = {   // Args to new wrregion:
      OldVal, // original vector
      Input, // value to write into subregion
      ConstantInt::get(I32Ty, VStride), // vstride
      ConstantInt::get(I32Ty, Width), // width
      ConstantInt::get(I32Ty, Stride), // stride
      StartIdx, // start index (in bytes)
      ParentWidthArg, // parent width (if variable start index)
      MaskArg // mask
  };
  Module *M = InsertBefore->getModule();
  Function *Decl = getGenXRegionDeclaration(M, IID, nullptr, Args);
  Instruction *NewInst = CallInst::Create(Decl, Args, Name, InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * CMRegion::createRdPredRegion : create rdpredregion instruction
 * CMRegion::createRdPredRegionOrConst : create rdpredregion instruction, or
 *      simplify to constant
 *
 * Enter:   Input = vector value to extract subregion from
 *          Index = start index of subregion
 *          Size = size of subregion
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give any new instruction
 *
 * Return:  The new rdpredregion instruction
 *
 * Unlike createRdRegion, this is a static method in Region, because you pass
 * the region parameters (the start index and size) directly into this method.
 */
Instruction *CMRegion::createRdPredRegion(Value *Input, unsigned Index,
    unsigned Size, const Twine &Name, Instruction *InsertBefore,
    const DebugLoc &DL)
{
  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  Value *Args[] = { // Args to new rdpredregion call:
    Input, // input predicate
    ConstantInt::get(I32Ty, Index) // start offset
  };
  auto RetTy =
      IGCLLVM::FixedVectorType::get(Args[0]->getType()->getScalarType(), Size);
  Module *M = InsertBefore->getModule();
  Function *Decl = getGenXRegionDeclaration(M, GenXIntrinsic::genx_rdpredregion,
      RetTy, Args);
  Instruction *NewInst = CallInst::Create(Decl, Args, Name, InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * CMRegion::createRdVectorSplat: create vector splat using rdregion
 *
 * Enter:   NumElements = number of elements in the resulting vector
 *          Input = Value from which splat is built
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give any new instruction
 *
 * Return:  The new rdregion representing vector splat
 */
Value *CMRegion::createRdVectorSplat(const DataLayout &DL, unsigned NumElements,
                                     Value *SplattedValue, const Twine &Name,
                                     Instruction *InsertPt,
                                     const DebugLoc &DbgLoc) {
  auto *V1Cast = CastInst::Create(
      Instruction::BitCast, SplattedValue,
      IGCLLVM::FixedVectorType::get(SplattedValue->getType(), 1),
      SplattedValue->getName() + ".v1cast", InsertPt);
  V1Cast->setDebugLoc(DbgLoc);

  CMRegion R(V1Cast->getType(), &DL);
  R.NumElements = NumElements;
  R.Width = 1;
  R.VStride = 0;
  R.Stride = 0;
  return R.createRdRegion(V1Cast, SplattedValue->getName() + ".splat", InsertPt,
                          DbgLoc, false /* AllowScalar */);
}

/***********************************************************************
* GetConstantSubvector : get a contiguous region from a vector constant
*/
static Constant *GetConstantSubvector(Constant *V,
  unsigned StartIdx, unsigned Size)
{
  Type *ElTy = cast<IGCLLVM::FixedVectorType>(V->getType())->getElementType();
  Type *RegionTy = IGCLLVM::FixedVectorType::get(ElTy, Size);
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

Value *CMRegion::createRdPredRegionOrConst(Value *Input, unsigned Index,
    unsigned Size, const Twine &Name, Instruction *InsertBefore,
    const DebugLoc &DL)
{
  if (auto C = dyn_cast<Constant>(Input))
    return GetConstantSubvector(C, Index, Size);
  return createRdPredRegion(Input, Index, Size, Name, InsertBefore, DL);
}

/***********************************************************************
 * CMRegion::createWrPredRegion : create wrpredregion instruction
 *
 * Enter:   OldVal = vector value to insert subregion into (can be undef)
 *          Input = subregion value to insert
 *          Index = start index of subregion
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give any new instruction
 *
 * Return:  The new wrpredregion instruction
 *
 * Unlike createWrRegion, this is a static method in Region, because you pass
 * the only region parameter (the start index) directly into this method.
 */
Instruction *CMRegion::createWrPredRegion(Value *OldVal, Value *Input,
    unsigned Index, const Twine &Name, Instruction *InsertBefore,
    const DebugLoc &DL)
{
  IntegerType *I32Ty = Type::getInt32Ty(Input->getContext());
  Value *Args[] = {   // Args to new wrpredregion:
      OldVal, // original vector
      Input, // value to write into subregion
      ConstantInt::get(I32Ty, Index), // start index
  };
  Module *M = InsertBefore->getModule();
  Function *Decl = getGenXRegionDeclaration(M, GenXIntrinsic::genx_wrpredregion,
      nullptr, Args);
  Instruction *NewInst = CallInst::Create(Decl, Args, Name, InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * CMRegion::createWrPredPredRegion : create wrpredpredregion instruction
 *
 * Enter:   OldVal = vector value to insert subregion into (can be undef)
 *          Input = subregion value to insert
 *          Index = start index of subregion
 *          Pred = predicate for the write region
 *          Name = name for new instruction
 *          InsertBefore = insert new inst before this point
 *          DL = DebugLoc to give any new instruction
 *
 * Return:  The new wrpredpredregion instruction
 *
 * Unlike createWrRegion, this is a static method in Region, because you pass
 * the only region parameter (the start index) directly into this method.
 */
Instruction *CMRegion::createWrPredPredRegion(Value *OldVal, Value *Input,
    unsigned Index, Value *Pred, const Twine &Name, Instruction *InsertBefore,
    const DebugLoc &DL)
{
  Type *Tys[] = { OldVal->getType(), Input->getType() };
  Function *CalledFunc = GenXIntrinsic::getGenXDeclaration(
      InsertBefore->getModule(), GenXIntrinsic::genx_wrpredpredregion, Tys);
  Value *Args[] = { OldVal, Input,
      ConstantInt::get(Type::getInt32Ty(InsertBefore->getContext()), Index),
      Pred };
  auto NewInst = CallInst::Create(CalledFunc, Args, "", InsertBefore);
  NewInst->setDebugLoc(DL);
  return NewInst;
}

/***********************************************************************
 * setRegionCalledFunc : for an existing rdregion/wrregion call, modify
 *      its called function to match its operand types
 *
 * This is used in GenXLegalization after modifying a wrregion operand
 * such that its type changes. The called function then needs to change
 * because it is decorated with overloaded types.
 */
void CMRegion::setRegionCalledFunc(Instruction *Inst)
{
  auto CI = cast<CallInst>(Inst);
  SmallVector<Value *, 8> Opnds;
  for (unsigned i = 0, e = IGCLLVM::getNumArgOperands(CI); i != e; ++i)
    Opnds.push_back(CI->getOperand(i));
  Function *Decl = getGenXRegionDeclaration(
      Inst->getModule(), GenXIntrinsic::getGenXIntrinsicID(Inst),
      Inst->getType(), Opnds);
  CI->setOperand(IGCLLVM::getNumArgOperands(CI), Decl);
}

/***********************************************************************
 * getRegionDeclaration : get the function declaration for a region intrinsic
 *
 * Enter:   M = Module
 *          IID = intrinsic ID
 *          RetTy = return type (can be 0 if return type not overloaded)
 *          Args = array of operands so we can determine overloaded types
 *
 * Return:  the Function
 */
Function *CMRegion::getGenXRegionDeclaration(Module *M,
    GenXIntrinsic::ID IID, Type *RetTy, ArrayRef<Value *> Args)
{
  switch (IID) {
    case GenXIntrinsic::genx_rdregioni:
    case GenXIntrinsic::genx_rdregionf: {
      Type *Tys[] = { RetTy, Args[0]->getType(), Args[4]->getType() };
      return GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
    }
    case GenXIntrinsic::genx_wrregioni:
    case GenXIntrinsic::genx_wrregionf:
    case GenXIntrinsic::genx_wrconstregion: {
      Type *Tys[] = { Args[0]->getType(), Args[1]->getType(),
          Args[5]->getType(), Args[7]->getType() };
      return GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
    }
    case GenXIntrinsic::genx_rdpredregion: {
      Type *Tys[] = { RetTy, Args[0]->getType() };
      return GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
    }
    case GenXIntrinsic::genx_wrpredregion: {
      Type *Tys[] = { Args[0]->getType(), Args[1]->getType() };
      return GenXIntrinsic::getGenXDeclaration(M, IID, Tys);
    }
    default:
      IGC_ASSERT_EXIT_MESSAGE(0, "unrecognized region intrinsic ID");
  }
  return nullptr;
}

/***********************************************************************
 * getStartIdx : get the LLVM IR Value for the start index of a region
 *
 * This is common code used by both createRdRegion and createWrRegion.
 */
Value *CMRegion::getStartIdx(const Twine &Name, Instruction *InsertBefore,
    const DebugLoc &DL)
{
  IntegerType *I16Ty = Type::getInt16Ty(InsertBefore->getContext());
  if (!Indirect)
    return ConstantInt::get(I16Ty, Offset);
  // Deal with indirect (variable index) region.
  if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(Indirect->getType())) {
    if (VT->getNumElements() != NumElements) {
      // We have a vector indirect and we need to take a subregion of it.
      CMRegion IdxRegion(Indirect);
      IdxRegion.getSubregion(IndirectIdx, NumElements / Width);
      Indirect = IdxRegion.createRdRegion(Indirect,
          Name + ".multiindirect_idx_subregion", InsertBefore, DL);
      IndirectIdx = 0;
    }
  }
  Value *Index = Indirect;
  if (Offset) {
    Constant *OffsetVal = ConstantInt::get(I16Ty, Offset);
    if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(Indirect->getType()))
      OffsetVal = ConstantVector::getSplat(
          IGCLLVM::getElementCount(VT->getNumElements()), OffsetVal);
    auto BO = BinaryOperator::Create(Instruction::Add, Index, OffsetVal,
        Name + ".indirect_idx_add", InsertBefore);
    BO->setDebugLoc(DL);
    Index = BO;
  }
  return Index;
}

/***********************************************************************
 * isSimilar : compare two regions to see if they have the same region
 *      parameters other than start offset, also allowing element type to
 *      be different
 */
bool CMRegion::isSimilar(const CMRegion &R2) const
{
  if (ElementBytes == R2.ElementBytes)
    return isStrictlySimilar(R2);
  // Change the element type to match, so we can compare the regions.
  CMRegion R = R2;
  // FIXME: we need DataLayout here
  if (!R.changeElementType(ElementTy, nullptr))
    return false;
  return isStrictlySimilar(R);
}

SmallVector<unsigned, 8> CMRegion::getAccessIndices() const {
  IGC_ASSERT(!Indirect);
  SmallVector<unsigned, 8> Res;
  Res.reserve(NumElements);
  IGC_ASSERT(Offset % ElementBytes == 0);
  unsigned StartIdx = Offset / ElementBytes;
  for (unsigned Row = 0; Row < NumElements / Width; ++Row)
    for (unsigned RowIdx = 0; RowIdx < Width; ++RowIdx)
      Res.push_back(StartIdx + Row * VStride + RowIdx * Stride);
  IGC_ASSERT(Res.size() == NumElements);
  return Res;
}

SmallBitVector CMRegion::getAccessBitMap(int MinTrackingOffset) const {
  // Construct bitmap for a single row
  SmallBitVector RowBitMap(getRowLength());
  for (unsigned i = 0; i < Width; i++) {
    RowBitMap <<= (Stride * ElementBytes);
    RowBitMap.set(0, ElementBytes);
  }
  // Apply row bitmap to a whole region bitmap
  // exactly NumRows times
  SmallBitVector BitMap(getLength());
  unsigned NumRows = NumElements / Width;
  if (NumRows != 1) {
    for (unsigned i = 0; i < NumRows; i++) {
      BitMap <<= (VStride * ElementBytes);
      BitMap |= RowBitMap;
    }
  } else
    BitMap = std::move(RowBitMap);
  // Adjust mask according to min tracking
  // offset for comparison
  IGC_ASSERT(Offset >= MinTrackingOffset);
  unsigned Diff = Offset - MinTrackingOffset;
  if (Diff) {
    BitMap.resize(BitMap.size() + Diff);
    BitMap <<= Diff;
  }
  return BitMap;
}

// overlap: Compare two regions to see whether they overlaps each other.
bool CMRegion::overlap(const CMRegion &R2) const {
  // To be conservative, if any of them is indirect, they overlaps.
  if (Indirect || R2.Indirect)
    return true;
  // To be conservative, if different masks are used, they overlaps.
  if (Mask != R2.Mask)
    return true;
  // Check offsets of regions for intersection
  int MaxOffset = std::max(Offset, R2.Offset);
  int MinEndOffset = std::min(Offset + getLength(), R2.Offset + R2.getLength());
  if (MaxOffset > MinEndOffset)
    return false;
  // Check overlapping using bit masks
  int MinOffset = std::min(Offset, R2.Offset);
  auto Mask1 = getAccessBitMap(MinOffset);
  auto Mask2 = R2.getAccessBitMap(MinOffset);
  // If there are any common bits then these regions overlap
  return Mask1.anyCommon(Mask2);
}

/***********************************************************************
 * CMRegion::isContiguous : test whether a region is contiguous
 */
bool CMRegion::isContiguous() const {
  return (Width == 1 || Stride == 1) &&
         (Width == NumElements || VStride == static_cast<int>(Width));
}

/***********************************************************************
 * CMRegion::isWhole : test whether a region covers exactly the whole of the
 *      given type, allowing for the element type being different
 */
bool CMRegion::isWhole(Type *Ty, const DataLayout *DL) const
{
  return isContiguous() && Offset == 0 &&
    NumElements * ElementBytes == vc::getTypeSize(Ty, DL).inBytes();
}

/***********************************************************************
 * evaluateConstantRdRegion : evaluate rdregion with constant input
 */
Constant *CMRegion::evaluateConstantRdRegion(Constant *Input, bool AllowScalar)
{
  IGC_ASSERT(!Indirect);
  if (NumElements != 1)
    AllowScalar = false;
  if (Constant *SV = Input->getSplatValue()) {
    if (AllowScalar)
      return SV;
    return ConstantVector::getSplat(IGCLLVM::getElementCount(NumElements), SV);
  }
  auto VT = cast<IGCLLVM::FixedVectorType>(Input->getType());
  SmallVector<Constant *, 8> Values;
  Constant *Undef = UndefValue::get(
      AllowScalar ? ElementTy
                  : IGCLLVM::FixedVectorType::get(ElementTy, NumElements));
  if (isa<UndefValue>(Input))
    return Undef;
  unsigned RowIdx = getOffsetInElements();
  unsigned Idx = RowIdx;
  unsigned NextRow = Width;
  for (unsigned i = 0; i != NumElements; ++i) {
    if (i == NextRow) {
      RowIdx += VStride;
      Idx = RowIdx;
    }
    if (Idx >= VT->getNumElements())
      return Undef; // out of range index
    // Get the element value and push it into Values.
    if (ConstantDataVector *CDV = dyn_cast<ConstantDataVector>(Input))
      Values.push_back(CDV->getElementAsConstant(Idx));
    else {
      auto CV = cast<ConstantVector>(Input);
      Values.push_back(CV->getOperand(Idx));
    }
    Idx += Stride;
  }
  if (AllowScalar)
    return Values[0];
  return ConstantVector::get(Values);
}

/***********************************************************************
 * evaluateConstantWrRegion : evaluate wrregion with constant inputs
 */
Constant *CMRegion::evaluateConstantWrRegion(Constant *OldVal, Constant *NewVal)
{
  IGC_ASSERT(!Indirect);
  SmallVector<Constant *, 8> Vec;
  for (unsigned i = 0, e = cast<IGCLLVM::FixedVectorType>(OldVal->getType())
                               ->getNumElements();
       i != e; ++i)
    Vec.push_back(OldVal->getAggregateElement(i));
  unsigned Off = getOffsetInElements();
  unsigned Row = Off;
  auto NewVT = dyn_cast<IGCLLVM::FixedVectorType>(NewVal->getType());
  unsigned NewNumEls = !NewVT ? 1 : NewVT->getNumElements();
  for (unsigned i = 0;;) {
    if (Off >= Vec.size())
      return UndefValue::get(OldVal->getType()); // out of range
    Vec[Off] = !NewVT ? NewVal : NewVal->getAggregateElement(i);
    if (++i == NewNumEls)
      break;
    if (i % Width) {
      Off += Stride;
      continue;
    }
    Row += VStride;
    Off = Row;
  }
  return ConstantVector::get(Vec);
}

/***********************************************************************
 * CMRegion::changeElementType : change element type of the region
 *
 * Return:  true if succeeded, false if failed (nothing altered)
 */
bool CMRegion::changeElementType(Type *NewElementType, const DataLayout *DL) {
  // TODO: enable this assert once out codebase is ready
  // IGC_ASSERT(DL);
  IGC_ASSERT(ElementBytes);
  IGC_ASSERT_MESSAGE(Offset % ElementBytes == 0, "Impossible offset (in bytes) for data type");

  if (Indirect || Mask)
    return false;

  unsigned NewElementBytes = vc::getTypeSize(NewElementType, DL).inBytes();

  if (NewElementBytes == ElementBytes) {
    // No change in element size
    ElementTy = NewElementType;
    return true;
  }

  // Check if the new element size is compatible with the current offset or the
  // total region size
  if (Offset % NewElementBytes ||
      (NumElements * ElementBytes) % NewElementBytes)
    return false;

  unsigned Ratio = NewElementBytes / ElementBytes;
  if (Ratio >= 1) {
    IGC_ASSERT_MESSAGE(isPowerOf2_32(Ratio), "Ratio must be pow of 2");
    // Trying to make the element size bigger.
    if (Width & (Ratio - 1))
      return false; // width misaligned
    if (VStride & (Ratio - 1))
      return false; // vstride misaligned
    if (Stride != 1)
      return false; // rows not contiguous
    if (Offset & (Ratio - 1))
      return false;
    unsigned LogRatio = Log2_32(Ratio);
    NumElements >>= LogRatio;
    Width >>= LogRatio;
    VStride >>= LogRatio;
    if (Width == 1) {
      // Width is now 1, so turn it into a 1D region.
      Stride = VStride;
      VStride = 0;
      Width = NumElements;
    }
    ElementTy = NewElementType;
    ElementBytes = NewElementBytes;
    return true;
  }
  // Trying to make the element size smaller.
  IGC_ASSERT(NewElementBytes);
  Ratio = ElementBytes / NewElementBytes;
  IGC_ASSERT_MESSAGE(isPowerOf2_32(Ratio), "Ratio must be pow of 2");
  unsigned LogRatio = Log2_32(Ratio);
  if (Stride == 1 || Width == 1) {
    // Row contiguous.
    Stride = 1;
    NumElements <<= LogRatio;
    Width <<= LogRatio;
    VStride <<= LogRatio;
    ElementTy = NewElementType;
    ElementBytes = NewElementBytes;
    return true;
  }
  if (!is2D()) {
    // 1D and not contiguous. Turn it into a 2D region.
    VStride = Stride << LogRatio;
    Stride = 1;
    Width = Ratio;
    NumElements <<= LogRatio;
    ElementTy = NewElementType;
    ElementBytes = NewElementBytes;
    return true;
  }
  return false;
}

/***********************************************************************
 * CMRegion::append : append region AR to this region
 *
 * Return:  true if succeeded (this region modified)
 *          false if not possible to append (this region in indeterminate state)
 *
 * This succeeds even if it leaves this region in an illegal state where
 * it has a non-integral number of rows. After doing a sequence of appends,
 * the caller needs to check that the resulting region is legal by calling
 * isWholeNumRows().
 */
bool CMRegion::append(CMRegion AR)
{
  IGC_ASSERT(AR.isWholeNumRows());
  if (Indirect != AR.Indirect)
    return false;
  IGC_ASSERT(AR.Width);
  unsigned ARNumRows = AR.NumElements / AR.Width;
  // Consider each row of AR separately.
  for (unsigned ARRow = 0; ARRow != ARNumRows;
      ++ARRow, AR.Offset += AR.VStride * AR.ElementBytes) {
    if (NumElements == Width) {
      // This region is currently 1D.
      if (NumElements == 1) {
        IGC_ASSERT(ElementBytes);
        Stride = (AR.Offset - Offset) / ElementBytes;
      }
      else if (AR.Width != 1 && Stride != AR.Stride)
        return false; // Mismatched stride.
      int NextOffset = Offset + Width * Stride * ElementBytes;
      if (AR.Offset == NextOffset) {
        // AR is a continuation of the same single row.
        Width += AR.Width;
        NumElements = Width;
        continue;
      }
      // AR is the start (or whole) of a second row.
      if (AR.Width > Width)
        return false; // AR row is bigger than this row.
      IGC_ASSERT(ElementBytes);
      VStride = (AR.Offset - Offset) / ElementBytes;
      NumElements += AR.Width;
      continue;
    }
    // This region is already 2D.
    IGC_ASSERT(Width);
    unsigned ExtraBit = NumElements % Width;
    int NextOffset = Offset + ((VStride * (NumElements / Width))
        + ExtraBit) * ElementBytes;
    if (NextOffset != AR.Offset)
      return false; // Mismatched next offset.
    if (AR.Width > Width - ExtraBit)
      return false; // Too much to fill whole row, or remainder of row after
                    //   existing extra bit.
    if (AR.Width != 1 && AR.Stride != Stride)
      return false; // Mismatched stride.
    NumElements += AR.Width;
  }
  return true;
}

/***********************************************************************
 * Region debug dump/print
 */
#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void CMRegion::dump() const
{
  errs() << *this << "\n";
}
#endif

void CMRegion::print(raw_ostream &OS) const
{
  OS << *IGCLLVM::FixedVectorType::get(ElementTy, NumElements) << " <"
     << VStride << ";" << Width << "," << Stride << ">(";
  if (Indirect) {
    OS << Indirect->getName();
    if (auto VT = dyn_cast<IGCLLVM::FixedVectorType>(Indirect->getType()))
      OS << "<" << VT->getNumElements() << ">(" << IndirectIdx << ")";
    OS << " + ";
  }
  OS << Offset << ")";
  if (Indirect && ParentWidth)
    OS << " {parentwidth=" << ParentWidth << "}";
  if (Mask)
    OS << " {mask=" << *Mask << "}";
}

unsigned CMRegion::getOffsetInElements() const {
  IGC_ASSERT_MESSAGE(
      ElementBytes,
      "the class is in invalid state: ElementBytes must not be zero");
  IGC_ASSERT_MESSAGE(Offset % ElementBytes == 0,
                     "offset must be in element bytes");
  return Offset / ElementBytes;
}

Type *CMRegion::getRegionType(bool UseDegenerateVectorType) const {
  IGC_ASSERT_MESSAGE(ElementTy && NumElements, "the class is in invalid state");
  if (UseDegenerateVectorType)
    return IGCLLVM::FixedVectorType::get(ElementTy, NumElements);
  if (NumElements == 1)
    return ElementTy;
  return IGCLLVM::FixedVectorType::get(ElementTy, NumElements);
}

// Analyzes region. Returns whether the region is 1D and what stride this 1D
// region has. The second return value is unspecified when the region is not
// 1D.
static std::pair<bool, int> analyze1DRegion(int NumElements, int VStride,
                                            int Width, int Stride) {
  if (Width == NumElements)
    return {true, Stride};
  if (Width == 1)
    return {true, VStride};
  // Don't care what stride to return when the region is not 1D.
  return {VStride == Width * Stride, Stride};
}

bool CMRegion::is1D() const {
  return analyze1DRegion(NumElements, VStride, Width, Stride).first;
}

int CMRegion::get1DStride() const {
  IGC_ASSERT_MESSAGE(is1D(), "the method can only be called for 1D region");
  return analyze1DRegion(NumElements, VStride, Width, Stride).second;
}

int CMRegion::getDstStride() const {
  auto Stride = get1DStride();
  if (Width == 1)
    return 1;
  IGC_ASSERT_MESSAGE(Stride > 0, "stride cannot be zero");
  return Stride;
}

void CMRegion::setElementTy(Type *Ty, DataLayout *DL) {
  ElementTy = Ty;
  ElementBytes = vc::getTypeSize(ElementTy, DL).inBytes();
}
