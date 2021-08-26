/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"

namespace pktz
{
    Constant* PacketBuilder::C(bool i) { return ConstantInt::get(IRB()->getInt1Ty(), (i ? 1 : 0)); }

    Constant* PacketBuilder::C(char i) { return ConstantInt::get(IRB()->getInt8Ty(), i); }

    Constant* PacketBuilder::C(uint8_t i) { return ConstantInt::get(IRB()->getInt8Ty(), i); }

    Constant* PacketBuilder::C(int i) { return ConstantInt::get(IRB()->getInt32Ty(), i); }

    Constant* PacketBuilder::C(int64_t i) { return ConstantInt::get(IRB()->getInt64Ty(), i); }

    Constant* PacketBuilder::C(uint16_t i) { return ConstantInt::get(mInt16Ty, i); }

    Constant* PacketBuilder::C(uint32_t i) { return ConstantInt::get(IRB()->getInt32Ty(), i); }

    Constant* PacketBuilder::C(uint64_t i) { return ConstantInt::get(IRB()->getInt64Ty(), i); }

    Constant* PacketBuilder::C(float i) { return ConstantFP::get(IRB()->getFloatTy(), i); }

    Constant* PacketBuilder::PRED(bool pred)
    {
        return ConstantInt::get(IRB()->getInt1Ty(), (pred ? 1 : 0));
    }

    Value* PacketBuilder::VIMMED1(int i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth),
                                      cast<ConstantInt>(C(i)));
    }

    Value* PacketBuilder::VIMMED1_16(int i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth16),
                                      cast<ConstantInt>(C(i)));
    }

    Value* PacketBuilder::VIMMED1(uint32_t i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth),
                                      cast<ConstantInt>(C(i)));
    }

    Value* PacketBuilder::VIMMED1_16(uint32_t i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth16),
                                      cast<ConstantInt>(C(i)));
    }

    Value* PacketBuilder::VIMMED1(float i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth),
                                      cast<ConstantFP>(C(i)));
    }

    Value* PacketBuilder::VIMMED1_16(float i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth16),
                                      cast<ConstantFP>(C(i)));
    }

    Value* PacketBuilder::VIMMED1(bool i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth),
                                      cast<ConstantInt>(C(i)));
    }

    Value* PacketBuilder::VIMMED1_16(bool i)
    {
      return ConstantVector::getSplat(IGCLLVM::getElementCount(mVWidth16),
                                      cast<ConstantInt>(C(i)));
    }

    Value *PacketBuilder::VUNDEF_IPTR() {
      return UndefValue::get(
          IGCLLVM::FixedVectorType::get(mInt32PtrTy, mVWidth));
    }

    Value *PacketBuilder::VUNDEF(Type *t) {
      return UndefValue::get(IGCLLVM::FixedVectorType::get(t, mVWidth));
    }

    Value *PacketBuilder::VUNDEF_I() {
      return UndefValue::get(IGCLLVM::FixedVectorType::get(mInt32Ty, mVWidth));
    }

    Value *PacketBuilder::VUNDEF_I_16() {
      return UndefValue::get(
          IGCLLVM::FixedVectorType::get(mInt32Ty, mVWidth16));
    }

    Value *PacketBuilder::VUNDEF_F() {
      return UndefValue::get(IGCLLVM::FixedVectorType::get(mFP32Ty, mVWidth));
    }

    Value *PacketBuilder::VUNDEF_F_16() {
      return UndefValue::get(IGCLLVM::FixedVectorType::get(mFP32Ty, mVWidth16));
    }

    Value* PacketBuilder::VUNDEF(Type* ty, uint32_t size)
    {
      return UndefValue::get(IGCLLVM::FixedVectorType::get(ty, size));
    }

    Value* PacketBuilder::VBROADCAST(Value* src, const llvm::Twine& name)
    {
        // check if src is already a vector
        if (src->getType()->isVectorTy())
        {
          if (auto CV = dyn_cast<ConstantVector>(src)) {
            if (CV->getSplatValue()) {
              return VECTOR_SPLAT(
                  mVWidth * cast<IGCLLVM::FixedVectorType>(src->getType())
                                ->getNumElements(),
                  CV->getSplatValue(), name);
            }
          }
          return src;
        }

        return VECTOR_SPLAT(mVWidth, src, name);
    }

    Value* PacketBuilder::VBROADCAST_16(Value* src)
    {
        // check if src is already a vector
        if (src->getType()->isVectorTy())
        {
            return src;
        }

        return VECTOR_SPLAT(mVWidth16, src);
    }

    uint32_t PacketBuilder::IMMED(Value* v)
    {
        IGC_ASSERT(isa<ConstantInt>(v));
        ConstantInt* pValConst = cast<ConstantInt>(v);
        return pValConst->getZExtValue();
    }

    int32_t PacketBuilder::S_IMMED(Value* v)
    {
        IGC_ASSERT(isa<ConstantInt>(v));
        ConstantInt* pValConst = cast<ConstantInt>(v);
        return pValConst->getSExtValue();
    }

    CallInst* PacketBuilder::CALL(Value*                               Callee,
                            const std::initializer_list<Value*>& argsList,
                            const llvm::Twine&                   name)
    {
        std::vector<Value*> args;
        for (auto arg : argsList)
            args.push_back(arg);
        return CALLA(Callee, args, name);
    }

    CallInst* PacketBuilder::CALL(Value* Callee, Value* arg)
    {
        std::vector<Value*> args;
        args.push_back(arg);
        return CALLA(Callee, args);
    }

    CallInst* PacketBuilder::CALL2(Value* Callee, Value* arg1, Value* arg2)
    {
        std::vector<Value*> args;
        args.push_back(arg1);
        args.push_back(arg2);
        return CALLA(Callee, args);
    }

    CallInst* PacketBuilder::CALL3(Value* Callee, Value* arg1, Value* arg2, Value* arg3)
    {
        std::vector<Value*> args;
        args.push_back(arg1);
        args.push_back(arg2);
        args.push_back(arg3);
        return CALLA(Callee, args);
    }

    Value* PacketBuilder::VRCP(Value* va, const llvm::Twine& name)
    {
        return FDIV(VIMMED1(1.0f), va, name); // 1 / a
    }

    Value* PacketBuilder::VPLANEPS(Value* vA, Value* vB, Value* vC, Value*& vX, Value*& vY)
    {
        Value* vOut = FMADDPS(vA, vX, vC);
        vOut        = FMADDPS(vB, vY, vOut);
        return vOut;
    }

    Value* PacketBuilder::EXTRACT_16(Value* x, uint32_t imm)
    {
        if (imm == 0)
        {
            return VSHUFFLE(x, UndefValue::get(x->getType()), {0, 1, 2, 3, 4, 5, 6, 7});
        }
        else
        {
            return VSHUFFLE(x, UndefValue::get(x->getType()), {8, 9, 10, 11, 12, 13, 14, 15});
        }
    }

    Value* PacketBuilder::JOIN_16(Value* a, Value* b)
    {
        return VSHUFFLE(a, b, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15});
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief convert x86 <N x float> mask to llvm <N x i1> mask
    Value* PacketBuilder::MASK(Value* vmask)
    {
        Value* src = BITCAST(vmask, mSimdInt32Ty);
        return ICMP_SLT(src, VIMMED1(0));
    }

    Value* PacketBuilder::MASK_16(Value* vmask)
    {
        Value* src = BITCAST(vmask, mSimd16Int32Ty);
        return ICMP_SLT(src, VIMMED1_16(0));
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief convert llvm <N x i1> mask to x86 <N x i32> mask
    Value* PacketBuilder::VMASK(Value* mask) { return S_EXT(mask, mSimdInt32Ty); }

    Value* PacketBuilder::VMASK_16(Value* mask) { return S_EXT(mask, mSimd16Int32Ty); }

    /// @brief Convert <Nxi1> llvm mask to integer
    Value* PacketBuilder::VMOVMSK(Value* mask)
    {
      IGC_ASSERT(cast<VectorType>(mask->getType())->getElementType() ==
                 mInt1Ty);
      uint32_t numLanes =
          cast<IGCLLVM::FixedVectorType>(mask->getType())->getNumElements();
      Value *i32Result;
      if (numLanes == 8) {
        i32Result = BITCAST(mask, mInt8Ty);
        }
        else if (numLanes == 16)
        {
            i32Result = BITCAST(mask, mInt16Ty);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "Unsupported vector width");
            i32Result = BITCAST(mask, mInt8Ty);
        }
        return Z_EXT(i32Result, mInt32Ty);
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief Generate a VPSHUFB operation in LLVM IR.  If not
    /// supported on the underlying platform, emulate it
    /// @param a - 256bit SIMD(32x8bit) of 8bit integer values
    /// @param b - 256bit SIMD(32x8bit) of 8bit integer mask values
    /// Byte masks in lower 128 lane of b selects 8 bit values from lower
    /// 128bits of a, and vice versa for the upper lanes.  If the mask
    /// value is negative, '0' is inserted.
    Value* PacketBuilder::PSHUFB(Value* a, Value* b)
    {
        Value* res;
        Constant* cB = dyn_cast<Constant>(b);
        IGC_ASSERT(cB);
        // number of 8 bit elements in b
        uint32_t numElms =
            cast<IGCLLVM::FixedVectorType>(cB->getType())->getNumElements();
        // output vector
        Value *vShuf =
            UndefValue::get(IGCLLVM::FixedVectorType::get(mInt8Ty, numElms));

        // insert an 8 bit value from the high and low lanes of a per loop iteration
        numElms /= 2;
        for (uint32_t i = 0; i < numElms; i++)
        {
            ConstantInt* cLow128b  = cast<ConstantInt>(cB->getAggregateElement(i));
            ConstantInt* cHigh128b = cast<ConstantInt>(cB->getAggregateElement(i + numElms));

            // extract values from constant mask
            char valLow128bLane  = (char)(cLow128b->getSExtValue());
            char valHigh128bLane = (char)(cHigh128b->getSExtValue());

            Value* insertValLow128b;
            Value* insertValHigh128b;

            // if the mask value is negative, insert a '0' in the respective output position
            // otherwise, lookup the value at mask position (bits 3..0 of the respective mask
            // byte) in a and insert in output vector
            insertValLow128b =
                (valLow128bLane < 0) ? C((char)0) : VEXTRACT(a, C((valLow128bLane & 0xF)));
            insertValHigh128b = (valHigh128bLane < 0)
                                    ? C((char)0)
                                    : VEXTRACT(a, C((valHigh128bLane & 0xF) + numElms));

            vShuf = VINSERT(vShuf, insertValLow128b, i);
            vShuf = VINSERT(vShuf, insertValHigh128b, (i + numElms));
        }
        res = vShuf;
        return res;
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief Generate a VPSHUFB operation (sign extend 8 8bit values to 32
    /// bits)in LLVM IR.  If not supported on the underlying platform, emulate it
    /// @param a - 128bit SIMD lane(16x8bit) of 8bit integer values.  Only
    /// lower 8 values are used.
    Value* PacketBuilder::PMOVSXBD(Value* a)
    {
        // VPMOVSXBD output type
        Type *v8x32Ty = IGCLLVM::FixedVectorType::get(mInt32Ty, 8);
        // Extract 8 values from 128bit lane and sign extend
        return S_EXT(VSHUFFLE(a, a, C<int>({0, 1, 2, 3, 4, 5, 6, 7})), v8x32Ty);
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief Generate a VPSHUFB operation (sign extend 8 16bit values to 32
    /// bits)in LLVM IR.  If not supported on the underlying platform, emulate it
    /// @param a - 128bit SIMD lane(8x16bit) of 16bit integer values.
    Value* PacketBuilder::PMOVSXWD(Value* a)
    {
        // VPMOVSXWD output type
        Type *v8x32Ty = IGCLLVM::FixedVectorType::get(mInt32Ty, 8);
        // Extract 8 values from 128bit lane and sign extend
        return S_EXT(VSHUFFLE(a, a, C<int>({0, 1, 2, 3, 4, 5, 6, 7})), v8x32Ty);
    }

    Value* PacketBuilder::PMAXSD(Value* a, Value* b)
    {
        Value* cmp = ICMP_SGT(a, b);
        return SELECT(cmp, a, b);
    }

    Value* PacketBuilder::PMINSD(Value* a, Value* b)
    {
        Value* cmp = ICMP_SLT(a, b);
        return SELECT(cmp, a, b);
    }

    Value* PacketBuilder::PMAXUD(Value* a, Value* b)
    {
        Value* cmp = ICMP_UGT(a, b);
        return SELECT(cmp, a, b);
    }

    Value* PacketBuilder::PMINUD(Value* a, Value* b)
    {
        Value* cmp = ICMP_ULT(a, b);
        return SELECT(cmp, a, b);
    }

    // Helper function to create alloca in entry block of function
    Value* PacketBuilder::CreateEntryAlloca(Function* pFunc, Type* pType)
    {
        auto saveIP = IRB()->saveIP();
        IRB()->SetInsertPoint(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
        Value* pAlloca = ALLOCA(pType);
        if (saveIP.isSet())
            IRB()->restoreIP(saveIP);
        return pAlloca;
    }

    Value* PacketBuilder::CreateEntryAlloca(Function* pFunc, Type* pType, Value* pArraySize)
    {
        auto saveIP = IRB()->saveIP();
        IRB()->SetInsertPoint(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
        Value* pAlloca = ALLOCA(pType, pArraySize);
        if (saveIP.isSet())
            IRB()->restoreIP(saveIP);
        return pAlloca;
    }

    Value* PacketBuilder::VABSPS(Value* a)
    {
        Value* asInt  = BITCAST(a, mSimdInt32Ty);
        Value* result = BITCAST(AND(asInt, VIMMED1(0x7fffffff)), mSimdFP32Ty);
        return result;
    }

    Value* PacketBuilder::ICLAMP(Value* src, Value* low, Value* high, const llvm::Twine& name)
    {
        Value* lowCmp = ICMP_SLT(src, low);
        Value* ret    = SELECT(lowCmp, low, src);

        Value* highCmp = ICMP_SGT(ret, high);
        ret            = SELECT(highCmp, high, ret, name);

        return ret;
    }

    Value* PacketBuilder::FCLAMP(Value* src, Value* low, Value* high)
    {
        Value* lowCmp = FCMP_OLT(src, low);
        Value* ret    = SELECT(lowCmp, low, src);

        Value* highCmp = FCMP_OGT(ret, high);
        ret            = SELECT(highCmp, high, ret);

        return ret;
    }

    Value* PacketBuilder::FCLAMP(Value* src, float low, float high)
    {
        Value* result = VMAXPS(src, VIMMED1(low));
        result        = VMINPS(result, VIMMED1(high));

        return result;
    }

    Value* PacketBuilder::FMADDPS(Value* a, Value* b, Value* c)
    {
        Value* vOut;

        vOut = FADD(FMUL(a, b), c);
        return vOut;
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief pop count on vector mask (e.g. <8 x i1>)
    Value* PacketBuilder::VPOPCNT(Value* a) { return POPCNT(VMOVMSK(a)); }

    //////////////////////////////////////////////////////////////////////////
    /// @brief C functions called by LLVM IR
    //////////////////////////////////////////////////////////////////////////

    Value* PacketBuilder::VEXTRACTI128(Value* a, Constant* imm8)
    {
        bool                      flag = !imm8->isZeroValue();
        SmallVector<Constant*, 8> idx;
        for (unsigned i = 0; i < mVWidth / 2; i++)
        {
            idx.push_back(C(flag ? i + mVWidth / 2 : i));
        }
        return VSHUFFLE(a, VUNDEF_I(), ConstantVector::get(idx));
    }

    Value* PacketBuilder::VINSERTI128(Value* a, Value* b, Constant* imm8)
    {
        bool                      flag = !imm8->isZeroValue();
        SmallVector<Constant*, 8> idx;
        for (unsigned i = 0; i < mVWidth; i++)
        {
            idx.push_back(C(i));
        }
        Value* inter = VSHUFFLE(b, VUNDEF_I(), ConstantVector::get(idx));

        SmallVector<Constant*, 8> idx2;
        for (unsigned i = 0; i < mVWidth / 2; i++)
        {
            idx2.push_back(C(flag ? i : i + mVWidth));
        }
        for (unsigned i = mVWidth / 2; i < mVWidth; i++)
        {
            idx2.push_back(C(flag ? i + mVWidth / 2 : i));
        }
        return VSHUFFLE(a, inter, ConstantVector::get(idx2));
    }

    uint32_t PacketBuilder::GetTypeSize(Type* pType)
    {
        if (pType->isStructTy())
        {
            uint32_t numElems = pType->getStructNumElements();
            Type*    pElemTy  = pType->getStructElementType(0);
            return numElems * GetTypeSize(pElemTy);
        }

        if (pType->isArrayTy())
        {
            uint32_t numElems = pType->getArrayNumElements();
            Type*    pElemTy  = pType->getArrayElementType();
            return numElems * GetTypeSize(pElemTy);
        }

        if (pType->isIntegerTy())
        {
            uint32_t bitSize = pType->getIntegerBitWidth();
            return bitSize / 8;
        }

        if (pType->isFloatTy())
        {
            return 4;
        }

        if (pType->isHalfTy())
        {
            return 2;
        }

        if (pType->isDoubleTy())
        {
            return 8;
        }

        IGC_ASSERT_MESSAGE(0, "Unimplemented type.");
        return 0;
    }
}
