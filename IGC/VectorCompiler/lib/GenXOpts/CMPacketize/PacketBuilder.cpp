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

#include "PacketBuilder.h"

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

        mSimd4FP64Ty = VectorType::get(mDoubleTy, 4);

        // Built in types: simd16
        mSimd16Int1Ty     = VectorType::get(mInt1Ty, mVWidth16);
        mSimd16Int16Ty    = VectorType::get(mInt16Ty, mVWidth16);
        mSimd16Int32Ty    = VectorType::get(mInt32Ty, mVWidth16);
        mSimd16Int64Ty    = VectorType::get(mInt64Ty, mVWidth16);
        mSimd16FP16Ty     = VectorType::get(mFP16Ty, mVWidth16);
        mSimd16FP32Ty     = VectorType::get(mFP32Ty, mVWidth16);

        mSimd32Int8Ty = VectorType::get(mInt8Ty, 32);

        if (sizeof(uint32_t*) == 4)
        {
            mIntPtrTy       = mInt32Ty;
            mSimd16IntPtrTy = mSimd16Int32Ty;
        }
        else
        {
            assert(sizeof(uint32_t*) == 8);
            mIntPtrTy       = mInt64Ty;
            mSimd16IntPtrTy = mSimd16Int64Ty;
        }
        // Built in types: target simd
        SetTargetWidth(width);

    }

    void PacketBuilder::SetTargetWidth(uint32_t width)
    {
        mVWidth = width;

        mSimdInt1Ty      = VectorType::get(mInt1Ty, mVWidth);
        mSimdInt16Ty     = VectorType::get(mInt16Ty, mVWidth);
        mSimdInt32Ty     = VectorType::get(mInt32Ty, mVWidth);
        mSimdInt64Ty     = VectorType::get(mInt64Ty, mVWidth);
        mSimdFP16Ty      = VectorType::get(mFP16Ty, mVWidth);
        mSimdFP32Ty      = VectorType::get(mFP32Ty, mVWidth);
        if (sizeof(uint32_t*) == 4)
        {
          mSimdIntPtrTy = mSimdInt32Ty;
        }
        else
        {
          assert(sizeof(uint32_t*) == 8);
          mSimdIntPtrTy = mSimdInt64Ty;
        }
    }

    /// @brief Mark this alloca as temporary to avoid hoisting later on
    void PacketBuilder::SetTempAlloca(Value* inst)
    {
        AllocaInst* pAlloca = dyn_cast<AllocaInst>(inst);
        assert(pAlloca && "Unexpected non-alloca instruction");
        MDNode* N = MDNode::get(getContext(), MDString::get(getContext(), "is_temp_alloca"));
        pAlloca->setMetadata("is_temp_alloca", N);
    }

    bool PacketBuilder::IsTempAlloca(Value* inst)
    {
        AllocaInst* pAlloca = dyn_cast<AllocaInst>(inst);
        assert(pAlloca && "Unexpected non-alloca instruction");

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

        if (pType->isVectorTy())
        {
            uint32_t vectorSize = pType->getVectorNumElements();
            Type*    pElemType = pType->getVectorElementType();
            Type*    pVecType = VectorType::get(pElemType, vectorSize*mVWidth);
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
        Type* vecType = VectorType::get(pType, mVWidth);
        return vecType;
    }
} // end of namespace pktz
