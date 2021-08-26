/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/IR/DerivedTypes.h"

#include "PacketBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;

namespace pktz
{
    //////////////////////////////////////////////////////////////////////////
    /// @brief Contructor for Builder.
    /// @param pJitMgr - JitManager which contains modules, function passes, etc.
    PacketBuilder::PacketBuilder(Module *pModule, uint32_t width)
    {
        mVWidth16 = 16;
        mpModule = static_cast<IGCLLVM::Module*>(pModule);

        // Built in types: scalar
        LLVMContext& Ctx = getContext();
        mpIRBuilder = new IGCLLVM::IRBuilder<>(Ctx);
        mVoidTy     = Type::getVoidTy(Ctx);
        mFP16Ty     = Type::getHalfTy(Ctx);
        mFP32Ty     = Type::getFloatTy(Ctx);
        mFP32PtrTy  = PointerType::get(mFP32Ty, 0);
        mDoubleTy   = Type::getDoubleTy(Ctx);
        mInt1Ty     = Type::getInt1Ty(Ctx);
        mInt8Ty     = Type::getInt8Ty(Ctx);
        mInt16Ty    = Type::getInt16Ty(Ctx);
        mInt32Ty    = Type::getInt32Ty(Ctx);
        mInt8PtrTy  = PointerType::get(mInt8Ty, 0);
        mInt16PtrTy = PointerType::get(mInt16Ty, 0);
        mInt32PtrTy = PointerType::get(mInt32Ty, 0);
        mInt64Ty    = Type::getInt64Ty(Ctx);

        mSimd4FP64Ty = IGCLLVM::FixedVectorType::get(mDoubleTy, 4);

        // Built in types: simd16
        mSimd16Int1Ty = IGCLLVM::FixedVectorType::get(mInt1Ty, mVWidth16);
        mSimd16Int16Ty = IGCLLVM::FixedVectorType::get(mInt16Ty, mVWidth16);
        mSimd16Int32Ty = IGCLLVM::FixedVectorType::get(mInt32Ty, mVWidth16);
        mSimd16Int64Ty = IGCLLVM::FixedVectorType::get(mInt64Ty, mVWidth16);
        mSimd16FP16Ty = IGCLLVM::FixedVectorType::get(mFP16Ty, mVWidth16);
        mSimd16FP32Ty = IGCLLVM::FixedVectorType::get(mFP32Ty, mVWidth16);

        mSimd32Int8Ty = IGCLLVM::FixedVectorType::get(mInt8Ty, 32);

        if (sizeof(uint32_t*) == 4)
        {
            mIntPtrTy       = mInt32Ty;
            mSimd16IntPtrTy = mSimd16Int32Ty;
        }
        else
        {
            IGC_ASSERT(sizeof(uint32_t*) == 8);
            mIntPtrTy       = mInt64Ty;
            mSimd16IntPtrTy = mSimd16Int64Ty;
        }
        // Built in types: target simd
        SetTargetWidth(width);

    }

    void PacketBuilder::SetTargetWidth(uint32_t width)
    {
        mVWidth = width;

        mSimdInt1Ty = IGCLLVM::FixedVectorType::get(mInt1Ty, mVWidth);
        mSimdInt16Ty = IGCLLVM::FixedVectorType::get(mInt16Ty, mVWidth);
        mSimdInt32Ty = IGCLLVM::FixedVectorType::get(mInt32Ty, mVWidth);
        mSimdInt64Ty = IGCLLVM::FixedVectorType::get(mInt64Ty, mVWidth);
        mSimdFP16Ty = IGCLLVM::FixedVectorType::get(mFP16Ty, mVWidth);
        mSimdFP32Ty = IGCLLVM::FixedVectorType::get(mFP32Ty, mVWidth);
        if (sizeof(uint32_t*) == 4)
        {
          mSimdIntPtrTy = mSimdInt32Ty;
        }
        else
        {
          IGC_ASSERT(sizeof(uint32_t*) == 8);
          mSimdIntPtrTy = mSimdInt64Ty;
        }
    }

    /// @brief Mark this alloca as temporary to avoid hoisting later on
    void PacketBuilder::SetTempAlloca(Value* inst)
    {
        AllocaInst* pAlloca = dyn_cast<AllocaInst>(inst);
        IGC_ASSERT_MESSAGE(pAlloca, "Unexpected non-alloca instruction");
        MDNode* N = MDNode::get(getContext(), MDString::get(getContext(), "is_temp_alloca"));
        pAlloca->setMetadata("is_temp_alloca", N);
    }

    bool PacketBuilder::IsTempAlloca(Value* inst)
    {
        AllocaInst* pAlloca = dyn_cast<AllocaInst>(inst);
        IGC_ASSERT_MESSAGE(pAlloca, "Unexpected non-alloca instruction");

        return (pAlloca->getMetadata("is_temp_alloca") != nullptr);
    }

    // Returns true if able to find a call instruction to mark
    bool PacketBuilder::SetNamedMetaDataOnCallInstr(Instruction* inst, StringRef mdName)
    {
        CallInst* pCallInstr = dyn_cast<CallInst>(inst);
        if (pCallInstr)
        {
            MDNode* N = MDNode::get(getContext(), MDString::get(getContext(), mdName));
            pCallInstr->setMetadata(mdName, N);
            return true;
        }
        else
        {
            // Follow use def chain back up
            for (Use& u : inst->operands())
            {
                Instruction* srcInst = dyn_cast<Instruction>(u.get());
                if (srcInst)
                {
                    if (SetNamedMetaDataOnCallInstr(srcInst, mdName))
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool PacketBuilder::HasNamedMetaDataOnCallInstr(Instruction* inst, StringRef mdName)
    {
        CallInst* pCallInstr = dyn_cast<CallInst>(inst);

        if (!pCallInstr)
        {
            return false;
        }

        return (pCallInstr->getMetadata(mdName) != nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    /// @brief Packetizes the type. Assumes SOA conversion.
    Type* PacketBuilder::GetVectorType(Type* pType)
    {
        if (pType->isVoidTy())
            return pType;

        if (auto VecpType = dyn_cast<IGCLLVM::FixedVectorType>(pType)) {
          uint32_t vectorSize = VecpType->getNumElements();
          Type *pElemType = VecpType->getElementType();
          Type *pVecType =
              IGCLLVM::FixedVectorType::get(pElemType, vectorSize * mVWidth);
          return pVecType;
        }

        // [N x float] should packetize to [N x <8 x float>]
        if (pType->isArrayTy())
        {
            uint32_t arraySize     = pType->getArrayNumElements();
            Type*    pArrayType    = pType->getArrayElementType();
            Type*    pVecArrayType = GetVectorType(pArrayType);
            Type*    pVecType      = ArrayType::get(pVecArrayType, arraySize);
            return pVecType;
        }

        // {float,int} should packetize to {<8 x float>, <8 x int>}
        if (pType->isAggregateType())
        {
            uint32_t              numElems = pType->getStructNumElements();
            SmallVector<Type*, 8> vecTypes;
            for (uint32_t i = 0; i < numElems; ++i)
            {
                Type* pElemType    = pType->getStructElementType(i);
                Type* pVecElemType = GetVectorType(pElemType);
                vecTypes.push_back(pVecElemType);
            }
            Type* pVecType = StructType::get(getContext(), vecTypes);
            return pVecType;
        }

        // <ty> should packetize to <8 x <ty>>
        Type *vecType = IGCLLVM::FixedVectorType::get(pType, mVWidth);
        return vecType;
    }
} // end of namespace pktz
