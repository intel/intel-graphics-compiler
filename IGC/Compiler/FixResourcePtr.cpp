/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/debug/Debug.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/FixResourcePtr.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-fix-resource-ptr"
#define PASS_DESCRIPTION "Fix the usage of GetBufferPtr, no combination of GetBufferPtr and GetResourcePtr"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FixResourcePtr, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FixResourcePtr, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char FixResourcePtr::ID = 0;

FixResourcePtr::FixResourcePtr() : FunctionPass(ID)
{
    initializeFixResourcePtrPass(*PassRegistry::getPassRegistry());
}

bool FixResourcePtr::runOnFunction(llvm::Function& F)
{
    llvm::IRBuilder<> __builder(F.getContext());
    builder = &__builder;
    DL = &F.getParent()->getDataLayout();
    m_changed = false;
    curFunc = &F;

    std::vector<Instruction*> fixlist;
    // initialize worklist with all the GetBufferPtrs with immed resource/sampler id
    inst_iterator it = inst_begin(&F);
    inst_iterator  e = inst_end(&F);
    for (; it != e; ++it)
    {
        GenIntrinsicInst* inst = dyn_cast<GenIntrinsicInst>(&*it);
        if (inst && inst->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
        {
            unsigned int as = inst->getType()->getPointerAddressSpace();
            Value* bufferIndex = ResolveBufferIndex(inst->getOperand(0));
            if (isa<ConstantInt>(bufferIndex) && IGC::IsDirectIdx(as))
            {
                RemoveGetBufferPtr(inst, bufferIndex);
            }
            else
            {
                fixlist.push_back(inst);
            }
        }
    }

    // fix the load/store that uses pointer coming from a GetBufferPtr/GetElementPtr,
    // change it to intrinsic that directly uses buf-pointer
    while (!fixlist.empty())
    {
        Instruction* inst = fixlist.back();
        fixlist.pop_back();

        FindGetElementPtr(inst, inst);
    }
    while (!eraseList.empty())
    {
        Instruction* inst = eraseList.back();
        eraseList.pop_back();
        inst->eraseFromParent();
    }
    return m_changed;
}

void FixResourcePtr::RemoveGetBufferPtr(GenIntrinsicInst* bufPtr, Value* bufIdx)
{
    uint outAS = bufPtr->getType()->getPointerAddressSpace();
    uint origAS = outAS;
    BufferType bufType = (BufferType)(cast<ConstantInt>(bufPtr->getOperand(1))->getZExtValue());
    uint encodeAS = EncodeAS4GFXResource(*bufIdx, bufType);
    if (outAS != encodeAS &&
        (bufType == CONSTANT_BUFFER || bufType == RESOURCE || bufType == UAV))
    {
        // happens to OGL, need to fix if address-space encoding is wrong
        outAS = encodeAS;
    }

    std::vector<Instruction*> foldlist;
    foldlist.push_back(bufPtr);
    // fold instructions on the worklist to constant null-pointer value
    while (!foldlist.empty())
    {
        Instruction* inst = foldlist.back();
        foldlist.pop_back();

        PointerType* const instType = dyn_cast<PointerType>(inst->getType());
        IGC_ASSERT(nullptr != instType);
        PointerType* ptrType = nullptr;
        if (IGCLLVM::isOpaquePointerTy(instType))
        {
            ptrType = PointerType::get(bufPtr->getContext(), outAS);
        }
        else
        {
            Type* eltType = IGCLLVM::getNonOpaquePtrEltTy(instType);    // Legacy code: getNonOpaquePtrEltTy
            ptrType = PointerType::get(eltType, outAS);
        }
        inst->mutateType(ptrType);
        // iterate all the uses, put bitcast on the worklist
        for (auto UI = inst->user_begin(), E = inst->user_end(); UI != E; ++UI)
        {
            if (BitCastInst * use = dyn_cast<BitCastInst>(*UI))
            {
                foldlist.push_back(use);
            }
            else if (GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(*UI))
            {
                Value* byteOffset = GetByteOffset(gep);
                builder->SetInsertPoint(gep);
                Value* int2ptr = builder->CreateIntToPtr(byteOffset, ptrType);
                gep->mutateType(ptrType);
                gep->replaceAllUsesWith(int2ptr);
                if (outAS != origAS)
                {
                    FixAddressSpaceInAllUses(int2ptr, outAS, origAS);
                }
            }
        }
        ConstantPointerNull* basePtr = ConstantPointerNull::get(ptrType);
        inst->replaceAllUsesWith(basePtr);
    }
}

void FixResourcePtr::FindGetElementPtr(Instruction* bufPtr, Instruction* searchPtr)
{
    // iterate all the uses, recursively find the GetElementPtr
    for (auto UI = searchPtr->user_begin(), E = searchPtr->user_end(); UI != E; ++UI)
    {
        if (BitCastInst * use = dyn_cast<BitCastInst>(*UI))
        {
            FindGetElementPtr(bufPtr, use);
        }
        else if (GetElementPtrInst * use = dyn_cast<GetElementPtrInst>(*UI))
        {
            FindLoadStore(bufPtr, use, use);
        }
        else if (LoadInst * ld = dyn_cast<LoadInst>(*UI))
        {
            // fix load
            Value* offsetValue = builder->getInt32(0);
            Value* loadIndexed = CreateLoadIntrinsic(ld, bufPtr, offsetValue);
            ld->replaceAllUsesWith(loadIndexed);
            eraseList.push_back(ld);
        }
        else if (StoreInst * st = dyn_cast<StoreInst>(*UI))
        {
            // fix store
            Value* offsetValue = builder->getInt32(0);
            Value* storeIndexed = CreateStoreIntrinsic(st, bufPtr, offsetValue);
            st->replaceAllUsesWith(storeIndexed);
            eraseList.push_back(st);
        }
    }
}

void FixResourcePtr::FindLoadStore(Instruction* bufPtr, Instruction* eltPtr, Instruction* searchPtr)
{
    // iterate all the uses, put bitcast on the worklist, recursively find the load/store
    for (auto UI = searchPtr->user_begin(), E = searchPtr->user_end(); UI != E; ++UI)
    {
        if (BitCastInst * use = dyn_cast<BitCastInst>(*UI))
        {
            FindLoadStore(bufPtr, eltPtr, use);
        }
        else if (LoadInst * use = dyn_cast<LoadInst>(*UI))
        {
            // fix load
            Value* offsetValue = GetByteOffset(eltPtr);
            Value* loadIndexed = CreateLoadIntrinsic(use, bufPtr, offsetValue);
            use->replaceAllUsesWith(loadIndexed);
            eraseList.push_back(use);
        }
        else if (StoreInst * use = dyn_cast<StoreInst>(*UI))
        {
            // fix store
            Value* offsetValue = GetByteOffset(eltPtr);
            Value* storeIndexed = CreateStoreIntrinsic(use, bufPtr, offsetValue);
            use->replaceAllUsesWith(storeIndexed);
            eraseList.push_back(use);
        }
    }
}

Value* FixResourcePtr::GetByteOffset(Instruction* eltPtr)
{
    IGC_ASSERT(eltPtr->getNumOperands() == 2);
    IGC_ASSERT(isa<llvm::GetElementPtrInst>(eltPtr));
    Value* eltIdx = eltPtr->getOperand(1);

    builder->SetInsertPoint(eltPtr);
    // decide offset in bytes
    //     may need to create shift
    uint  eltBytes = int_cast<uint>(DL->getTypeStoreSize(cast<llvm::GetElementPtrInst>(eltPtr)->getSourceElementType()));
    APInt eltSize = APInt(32, eltBytes);

    Value* offsetValue = eltIdx;
    if (eltSize != 1)
    {
        if (const ConstantInt * CI = dyn_cast<ConstantInt>(eltIdx))
        {
            uint32_t byteOffset = int_cast<uint32_t>(eltBytes * CI->getSExtValue());
            offsetValue = ConstantInt::get(eltIdx->getType(), byteOffset);
        }
        else if (eltSize.isPowerOf2())
        {
            APInt shift = APInt(eltIdx->getType()->getScalarSizeInBits(), eltSize.logBase2());
            offsetValue = builder->CreateShl(eltIdx, shift);
        }
        else
        {
            offsetValue = builder->CreateMul(eltIdx, builder->getInt(eltSize));
        }
    }
    return offsetValue;
}

Value* FixResourcePtr::CreateLoadIntrinsic(LoadInst* inst, Instruction* bufPtr, Value* offsetVal)
{
    IGC_ASSERT(offsetVal->getType()->getScalarSizeInBits() == 32);
    Function* l;
    builder->SetInsertPoint(inst);
    llvm::Type* tys[2];
    tys[0] = inst->getType();
    tys[1] = bufPtr->getType();
    l = GenISAIntrinsic::getDeclaration(curFunc->getParent(),
        inst->getType()->isVectorTy() ? llvm::GenISAIntrinsic::GenISA_ldrawvector_indexed : llvm::GenISAIntrinsic::GenISA_ldraw_indexed,
        tys);

    alignment_t alignment = std::max((alignment_t)(inst->getType()->getScalarSizeInBits() / 8),
                                  IGCLLVM::getAlignmentValue(inst));

    Value* attr[] =
    {
        bufPtr,
        offsetVal,
        builder->getInt32((uint32_t)alignment),
        builder->getInt1(inst->isVolatile())
    };
    Value* ld = builder->CreateCall(l, attr);
    if (!inst->getType()->isVectorTy())
    {
        if (!inst->getType()->isFloatTy())
        {
            Value* bitcast = dyn_cast<Instruction>(builder->CreateBitCast(ld, inst->getType()));
            ld = bitcast;
        }
    }
    return ld;
}

Value* FixResourcePtr::CreateStoreIntrinsic(StoreInst* inst, Instruction* bufPtr, Value* offsetVal)
{
    IGC_ASSERT(offsetVal->getType()->getScalarSizeInBits() == 32);
    Function* l;
    builder->SetInsertPoint(inst);
    Value* storeVal = inst->getValueOperand();
    if (storeVal->getType()->isVectorTy())
    {
        llvm::Type* tys[2];
        tys[0] = bufPtr->getType();
        tys[1] = inst->getValueOperand()->getType();
        l = GenISAIntrinsic::getDeclaration(curFunc->getParent(),
            llvm::GenISAIntrinsic::GenISA_storerawvector_indexed,
            tys);
    }
    else
    {
        llvm::Type* dataType = storeVal->getType();

        IGC_ASSERT(dataType->getPrimitiveSizeInBits() == 16 || dataType->getPrimitiveSizeInBits() == 32);

        if (!dataType->isFloatingPointTy())
        {
            storeVal = builder->CreateBitCast(
                storeVal,
                dataType->getPrimitiveSizeInBits() == 32 ? builder->getFloatTy() : builder->getHalfTy());
        }

        llvm::Type* types[2] = {
            bufPtr->getType(),
            storeVal->getType() };

        l = GenISAIntrinsic::getDeclaration(curFunc->getParent(),
            llvm::GenISAIntrinsic::GenISA_storeraw_indexed,
            types);

    }
    alignment_t alignment = std::max((alignment_t)(storeVal->getType()->getScalarSizeInBits() / 8),
                                  IGCLLVM::getAlignmentValue(inst));

    Value* attr[] =
    {
        bufPtr,
        offsetVal,
        storeVal,
        builder->getInt32((uint32_t)alignment),
        builder->getInt1(inst->isVolatile())
    };
    Value* st = builder->CreateCall(l, attr);
    return st;
}

///
/// @brief Resolves the buffer index argument of GetBufferPtr intrinsic.
///
/// This method looks for a ConstInt value of the buffer index. It handles
/// cases where the input buffer index is an extracted element of a vector,
/// e.g.
///
///  %17 = extractelement <2 x i32> zeroinitializer, i32 0
///  %19 = insertelement  <4 x i32> undef, i32 %17, i32 0
///  %20 = insertelement  <4 x i32> %19, i32 %15, i32 1
///  %21 = insertelement  <4 x i32> %20, i32 %15, i32 2
///  %Temp-47.i = insertelement <4 x i32> %21, i32 0, i32 3
///  %22 = extractelement <4 x i32> %Temp-47.i, i32 0
///  %23 = extractelement <4 x i32> %Temp-47.i, i32 1
///  %24 = extractelement <4 x i32> %Temp-47.i, i32 2
///  %25 = extractelement <4 x i32> %Temp-47.i, i32 3
///  %26 = call i32 addrspace(1179648)* @llvm.genx.GenISA.GetBufferPtr.p1179648i32(i32 %22, i32 1)
///
/// @param bufferIndex Buffer index value to resolve
/// @param vectorIndex Index inside of a vector where the buffer index is stored
/// @returns Value* ConstantInt buffer index if resolved or the input bufferIndex
///
Value* FixResourcePtr::ResolveBufferIndex(Value* bufferIndex, Value* vectorIndex)
{
    if (Constant * c = dyn_cast<Constant>(bufferIndex))
    {
        if (vectorIndex && isa<ConstantInt>(vectorIndex))
        {
            return c->getAggregateElement(cast<ConstantInt>(vectorIndex));
        }
        else if (isa<ConstantInt>(bufferIndex))
        {
            return bufferIndex;
        }
        else
        {
            IGC_ASSERT(0);
        }
    }
    else if (ExtractElementInst * ee = dyn_cast<ExtractElementInst>(bufferIndex))
    {
        return ResolveBufferIndex(
            ee->getOperand(0),  // vector argument to extract from
            ee->getOperand(1)); // element index in vector
    }
    else if (InsertElementInst * ie = dyn_cast<InsertElementInst>(bufferIndex))
    {
        if (vectorIndex &&
            isa<ConstantInt>(vectorIndex) &&
            isa<ConstantInt>(ie->getOperand(2)))
        {
            // compare indexes in vectors
            if ((cast<ConstantInt>(vectorIndex))->getZExtValue() == (cast<ConstantInt>(ie->getOperand(2)))->getZExtValue())
            {
                // indexes match,this is the insert element instruction with buffer index
                return ResolveBufferIndex(ie->getOperand(1));
            }
            else
            {
                // indexes don't match, keep looking for a matching insert element instruction
                return ResolveBufferIndex(ie->getOperand(0), vectorIndex);
            }
        }
    }

    return bufferIndex;
}
