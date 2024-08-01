/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

namespace pktz {
Constant *PacketBuilder::C(bool Val) {
  return ConstantInt::get(IRB->getInt1Ty(), (Val ? 1 : 0));
}

Constant *PacketBuilder::C(char Val) {
  return ConstantInt::get(IRB->getInt8Ty(), Val);
}

Constant *PacketBuilder::C(uint8_t Val) {
  return ConstantInt::get(IRB->getInt8Ty(), Val);
}

Constant *PacketBuilder::C(int Val) {
  return ConstantInt::get(IRB->getInt32Ty(), Val);
}

Constant *PacketBuilder::C(int64_t Val) {
  return ConstantInt::get(IRB->getInt64Ty(), Val);
}

Constant *PacketBuilder::C(uint16_t Val) {
  return ConstantInt::get(Int16Ty, Val);
}

Constant *PacketBuilder::C(uint32_t Val) {
  return ConstantInt::get(IRB->getInt32Ty(), Val);
}

Constant *PacketBuilder::C(uint64_t Val) {
  return ConstantInt::get(IRB->getInt64Ty(), Val);
}

Constant *PacketBuilder::C(float Val) {
  return ConstantFP::get(IRB->getFloatTy(), Val);
}

Constant *PacketBuilder::PRED(bool Pred) {
  return ConstantInt::get(IRB->getInt1Ty(), (Pred ? 1 : 0));
}

Value *PacketBuilder::VIMMED1(int Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1_16(int Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth16),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1(uint32_t Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1_16(uint32_t Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth16),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1(float Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantFP>(C(Val)));
}

Value *PacketBuilder::VIMMED1_16(float Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth16),
                                  cast<ConstantFP>(C(Val)));
}

Value *PacketBuilder::VIMMED1(bool Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1_16(bool Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth16),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VUNDEF_IPTR() {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Int32PtrTy, VWidth));
}

Value *PacketBuilder::VUNDEF(Type *Ty) {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Ty, VWidth));
}

Value *PacketBuilder::VUNDEF_I() {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Int32Ty, VWidth));
}

Value *PacketBuilder::VUNDEF_I_16() {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Int32Ty, VWidth16));
}

Value *PacketBuilder::VUNDEF_F() {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(FP32Ty, VWidth));
}

Value *PacketBuilder::VUNDEF_F_16() {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(FP32Ty, VWidth16));
}

Value *PacketBuilder::VUNDEF(Type *Ty, uint32_t Size) {
  return UndefValue::get(IGCLLVM::FixedVectorType::get(Ty, Size));
}

Value *PacketBuilder::VBROADCAST(Value *Src, const llvm::Twine &Name) {
  // check if Src is already a vector
  if (Src->getType()->isVectorTy()) {
    if (auto *CV = dyn_cast<ConstantVector>(Src)) {
      if (CV->getSplatValue()) {
        return VECTOR_SPLAT(VWidth *
                                cast<IGCLLVM::FixedVectorType>(Src->getType())
                                    ->getNumElements(),
                            CV->getSplatValue(), Name);
      }
    }
    return Src;
  }
  return VECTOR_SPLAT(VWidth, Src, Name);
}

Value *PacketBuilder::VBROADCAST_16(Value *Src) {
  // check if Src is already a vector
  if (Src->getType()->isVectorTy()) {
    return Src;
  }
  return VECTOR_SPLAT(VWidth16, Src);
}

uint32_t PacketBuilder::IMMED(Value *V) {
  return cast<ConstantInt>(V)->getZExtValue();
}

int32_t PacketBuilder::S_IMMED(Value *V) {
  return cast<ConstantInt>(V)->getSExtValue();
}

CallInst *PacketBuilder::CALL(Value *Callee,
                              const std::initializer_list<Value *> &ArgsList,
                              const llvm::Twine &Name) {
  std::vector<Value *> Args;
  for (auto *Arg : ArgsList)
    Args.push_back(Arg);
  return CALLA(Callee, Args, Name);
}

CallInst *PacketBuilder::CALL(Value *Callee, Value *Arg) {
  SmallVector<Value *, 1> Args;
  Args.push_back(Arg);
  return CALLA(Callee, Args);
}

CallInst *PacketBuilder::CALL2(Value *Callee, Value *Arg1, Value *Arg2) {
  SmallVector<Value *, 2> Args;
  Args.push_back(Arg1);
  Args.push_back(Arg2);
  return CALLA(Callee, Args);
}

CallInst *PacketBuilder::CALL3(Value *Callee, Value *Arg1, Value *Arg2,
                               Value *Arg3) {
  SmallVector<Value *, 3> Args;
  Args.push_back(Arg1);
  Args.push_back(Arg2);
  Args.push_back(Arg3);
  return CALLA(Callee, Args);
}

Value *PacketBuilder::VRCP(Value *A, const llvm::Twine &Name) {
  return FDIV(VIMMED1(1.0f), A, Name); // 1 / A
}

Value *PacketBuilder::VPLANEPS(Value *A, Value *B, Value *C, Value *&X,
                               Value *&Y) {
  return FMADDPS(B, Y, FMADDPS(A, X, C));
}

Value *PacketBuilder::EXTRACT_16(Value *A, uint32_t Imm) {
  if (Imm == 0)
    return VSHUFFLE(A, UndefValue::get(A->getType()), {0, 1, 2, 3, 4, 5, 6, 7});
  else
    return VSHUFFLE(A, UndefValue::get(A->getType()),
                    {8, 9, 10, 11, 12, 13, 14, 15});
}

Value *PacketBuilder::JOIN_16(Value *A, Value *B) {
  return VSHUFFLE(A, B, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
}

//////////////////////////////////////////////////////////////////////////
/// @brief convert x86 <N x float> mask to llvm <N x i1> mask
Value *PacketBuilder::MASK(Value *VMask) {
  return ICMP_SLT(BITCAST(VMask, SimdInt32Ty), VIMMED1(0));
}

Value *PacketBuilder::MASK_16(Value *VMask) {
  return ICMP_SLT(BITCAST(VMask, Simd16Int32Ty), VIMMED1_16(0));
}

//////////////////////////////////////////////////////////////////////////
/// @brief convert llvm <N x i1> mask to x86 <N x i32> mask
Value *PacketBuilder::VMASK(Value *Mask) { return S_EXT(Mask, SimdInt32Ty); }

Value *PacketBuilder::VMASK_16(Value *Mask) {
  return S_EXT(Mask, Simd16Int32Ty);
}

/// @brief Convert <N x i1> llvm mask to integer
Value *PacketBuilder::VMOVMSK(Value *Mask) {
  IGC_ASSERT(cast<VectorType>(Mask->getType())->getElementType() == Int1Ty);
  uint32_t NumLanes =
      cast<IGCLLVM::FixedVectorType>(Mask->getType())->getNumElements();
  Value *Result = nullptr;
  if (NumLanes == 8) {
    Result = BITCAST(Mask, Int8Ty);
  } else if (NumLanes == 16) {
    Result = BITCAST(Mask, Int16Ty);
  } else {
    IGC_ASSERT_MESSAGE(0, "Unsupported vector width");
    Result = BITCAST(Mask, Int8Ty);
  }
  return Z_EXT(Result, Int32Ty);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Generate a VPSHUFB operation in LLVM IR.  If not
/// supported on the underlying platform, emulate it
/// @param A - 256bit SIMD(32x8bit) of 8bit integer values
/// @param B - 256bit SIMD(32x8bit) of 8bit integer mask values
/// Byte masks in lower 128 lane of b selects 8 bit values from lower
/// 128bits of a, and vice versa for the upper lanes.  If the mask
/// value is negative, '0' is inserted.
Value *PacketBuilder::PSHUFB(Value *A, Value *B) {
  auto *CB = cast<Constant>(B);
  // number of 8 bit elements in B
  uint32_t NumElems =
      cast<IGCLLVM::FixedVectorType>(CB->getType())->getNumElements();
  // output vector
  Value *Result =
      UndefValue::get(IGCLLVM::FixedVectorType::get(Int8Ty, NumElems));
  // insert an 8 bit value from the high and low lanes of a per loop iteration
  NumElems /= 2;
  for (uint32_t Idx = 0; Idx < NumElems; Idx++) {
    auto *Low128b = cast<ConstantInt>(CB->getAggregateElement(Idx));
    auto *High128b = cast<ConstantInt>(CB->getAggregateElement(NumElems + Idx));
    // extract values from constant mask
    char ValLow128bLane = static_cast<char>(Low128b->getSExtValue());
    char ValHigh128bLane = static_cast<char>(High128b->getSExtValue());
    // if the mask value is negative, insert a '0' in the respective output
    // position otherwise, lookup the value at mask position (bits 3..0 of the
    // respective mask byte) in a and insert in output vector
    Value *InsertValLow128b = (ValLow128bLane < 0)
                                  ? C((char)0)
                                  : VEXTRACT(A, C((ValLow128bLane & 0xF)));
    Value *InsertValHigh128b =
        (ValHigh128bLane < 0)
            ? C((char)0)
            : VEXTRACT(A, C((ValHigh128bLane & 0xF) + NumElems));

    Result = VINSERT(Result, InsertValLow128b, Idx);
    Result = VINSERT(Result, InsertValHigh128b, NumElems + Idx);
  }
  return Result;
}

//////////////////////////////////////////////////////////////////////////
/// @brief Generate a VPSHUFB operation (sign extend 8 8bit values to 32
/// bits) in LLVM IR.  If not supported on the underlying platform, emulate it
/// @param A - 128bit SIMD lane(16x8bit) of 8bit integer values.  Only
/// lower 8 values are used.
Value *PacketBuilder::PMOVSXBD(Value *A) {
  // VPMOVSXBD output type
  auto *Ty = IGCLLVM::FixedVectorType::get(Int32Ty, 8);
  // Extract 8 values from 128bit lane and sign extend
  return S_EXT(VSHUFFLE(A, A, C<int>({0, 1, 2, 3, 4, 5, 6, 7})), Ty);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Generate a VPSHUFB operation (sign extend 8 16bit values to 32
/// bits) in LLVM IR.  If not supported on the underlying platform, emulate it
/// @param A - 128bit SIMD lane(8x16bit) of 16bit integer values.
Value *PacketBuilder::PMOVSXWD(Value *A) {
  // VPMOVSXWD output type
  auto *Ty = IGCLLVM::FixedVectorType::get(Int32Ty, 8);
  // Extract 8 values from 128bit lane and sign extend
  return S_EXT(VSHUFFLE(A, A, C<int>({0, 1, 2, 3, 4, 5, 6, 7})), Ty);
}

Value *PacketBuilder::PMAXSD(Value *A, Value *B) {
  return SELECT(ICMP_SGT(A, B), A, B);
}

Value *PacketBuilder::PMINSD(Value *A, Value *B) {
  return SELECT(ICMP_SLT(A, B), A, B);
}

Value *PacketBuilder::PMAXUD(Value *A, Value *B) {
  return SELECT(ICMP_UGT(A, B), A, B);
}

Value *PacketBuilder::PMINUD(Value *A, Value *B) {
  return SELECT(ICMP_ULT(A, B), A, B);
}

// Helper function to create alloca in entry block of function
Value *PacketBuilder::createEntryAlloca(Function *F, Type *Ty) {
  auto IP = IRB->saveIP();
  IRB->SetInsertPoint(&F->getEntryBlock(), F->getEntryBlock().begin());
  auto *Alloca = ALLOCA(Ty);
  if (IP.isSet())
    IRB->restoreIP(IP);
  return Alloca;
}

Value *PacketBuilder::createEntryAlloca(Function *F, Type *Ty, Value *ArrSize) {
  auto IP = IRB->saveIP();
  IRB->SetInsertPoint(&F->getEntryBlock(), F->getEntryBlock().begin());
  auto *Alloca = ALLOCA(Ty, ArrSize);
  if (IP.isSet())
    IRB->restoreIP(IP);
  return Alloca;
}

Value *PacketBuilder::VABSPS(Value *A) {
  return BITCAST(AND(BITCAST(A, SimdInt32Ty), VIMMED1(0x7fffffff)), SimdFP32Ty);
}

Value *PacketBuilder::ICLAMP(Value *Src, Value *Low, Value *High,
                             const llvm::Twine &Name) {
  auto *LowCmp = ICMP_SLT(Src, Low);
  auto *Tmp = SELECT(LowCmp, Low, Src);
  auto *HighCmp = ICMP_SGT(Tmp, High);
  return SELECT(HighCmp, High, Tmp, Name);
}

Value *PacketBuilder::FCLAMP(Value *Src, Value *Low, Value *High) {
  auto *LowCmp = FCMP_OLT(Src, Low);
  auto *Tmp = SELECT(LowCmp, Low, Src);
  auto *HighCmp = FCMP_OGT(Tmp, High);
  return SELECT(HighCmp, High, Tmp);
}

Value *PacketBuilder::FCLAMP(Value *Src, float Low, float High) {
  return VMINPS(VMAXPS(Src, VIMMED1(Low)), VIMMED1(High));
}

Value *PacketBuilder::FMADDPS(Value *A, Value *B, Value *C) {
  return FADD(FMUL(A, B), C);
}

//////////////////////////////////////////////////////////////////////////
/// @brief pop count on vector mask (e.g. <8 x i1>)
Value *PacketBuilder::VPOPCNT(Value *A) { return POPCNT(VMOVMSK(A)); }

//////////////////////////////////////////////////////////////////////////
/// @brief C functions called by LLVM IR
//////////////////////////////////////////////////////////////////////////

Value *PacketBuilder::VEXTRACTI128(Value *A, Constant *Imm8) {
  bool Flag = !Imm8->isZeroValue();
  SmallVector<Constant *, 8> Indices;
  for (unsigned Idx = 0; Idx < VWidth / 2; Idx++) {
    Indices.push_back(C(Flag ? Idx + VWidth / 2 : Idx));
  }
  return VSHUFFLE(A, VUNDEF_I(), ConstantVector::get(Indices));
}

Value *PacketBuilder::VINSERTI128(Value *A, Value *B, Constant *Imm8) {
  bool Flag = !Imm8->isZeroValue();
  SmallVector<Constant *, 8> Indices;
  for (unsigned Idx = 0; Idx < VWidth; Idx++) {
    Indices.push_back(C(Idx));
  }
  Value *Tmp = VSHUFFLE(B, VUNDEF_I(), ConstantVector::get(Indices));
  SmallVector<Constant *, 8> Indices2;
  for (unsigned Idx = 0; Idx < VWidth / 2; Idx++) {
    Indices2.push_back(C(Flag ? Idx : Idx + VWidth));
  }
  for (unsigned Idx = VWidth / 2; Idx < VWidth; Idx++) {
    Indices2.push_back(C(Flag ? Idx + VWidth / 2 : Idx));
  }
  return VSHUFFLE(A, Tmp, ConstantVector::get(Indices2));
}

uint32_t PacketBuilder::getTypeSize(Type *Ty) {
  if (Ty->isStructTy()) {
    uint32_t NumElems = Ty->getStructNumElements();
    auto *ElemTy = Ty->getStructElementType(0);
    return NumElems * getTypeSize(ElemTy);
  }
  if (Ty->isArrayTy()) {
    uint32_t NumElems = Ty->getArrayNumElements();
    auto *ElemTy = Ty->getArrayElementType();
    return NumElems * getTypeSize(ElemTy);
  }
  if (Ty->isIntegerTy()) {
    uint32_t BitSize = Ty->getIntegerBitWidth();
    return BitSize / 8;
  }
  if (Ty->isFloatTy())
    return 4;
  if (Ty->isHalfTy())
    return 2;
  if (Ty->isDoubleTy())
    return 8;
  IGC_ASSERT_MESSAGE(0, "Unimplemented type.");
  return 0;
}
} // namespace pktz
