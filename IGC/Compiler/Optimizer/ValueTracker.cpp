/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/Optimizer/ValueTracker.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "DebugInfo/DebugInfoUtils.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include "IGC/common/StringMacros.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// This function is called when a gen intrinsic instruction is met during the first step
// of the overall algorithm. It currently supports GenISA_GetBufferPtr only, but it could
// be extended in the future.
Value* ValueTracker::handleGenIntrinsic(GenIntrinsicInst* I)
{
    if (I->getIntrinsicID() == GenISAIntrinsic::GenISA_GetBufferPtr)
    {
        // Reached a GetBufferPtr instruction.
        // This will always be true for non-inlined samplers. With the resource pointer change, a GBP is created
        // for each argument sampler. However the argument is still required due to how snap_coord WA and normalized
        // coords are implemented on compute shaders. The argument pointer will have the resource ID and type
        // encoded in its unique addrspace. We can also figure out the addrspace from the GetBufferPtr instruction.
        // So if we reach a GBP, search all the arguments for one that matches its encoded addrspace, and return it.
        Value* bufIdV = I->getOperand(0);
        Value* bufTyV = I->getOperand(1);
        IGC_ASSERT(isa<ConstantInt>(bufIdV));
        IGC_ASSERT(isa<ConstantInt>(bufTyV));
        IGC::BufferType bufType = (IGC::BufferType)(cast<ConstantInt>(bufTyV)->getZExtValue());
        unsigned as = IGC::EncodeAS4GFXResource(*bufIdV, bufType, 0);

        Function* mainFunc = I->getParent()->getParent();
        for (auto& arg : mainFunc->args())
        {
            unsigned argAS = -1;
            if (arg.getType()->isPointerTy())
            {
                argAS = arg.getType()->getPointerAddressSpace();
            }
            if (as == argAS)
            {
                return &arg;
            }
        }

        // If we can't find it via address space, look around in resource allocator.
        if (m_pMDUtils)
        {
            return CImagesBI::CImagesUtils::findImageFromBufferPtr(
                *m_pMDUtils,
                mainFunc,
                bufType,
                cast<ConstantInt>(bufIdV)->getZExtValue(),
                m_pModMD);
        }

        IGC_ASSERT_MESSAGE(0, "Found GetBufferPtr but cannot match it with an argument!");
        return nullptr;
    }
    return nullptr;
}

// This function is called when an extract element instruction is met during the first step
// of the overall algorithm. It currently expects that extract element instruction operand will
// be either InsertElementInst, BitCastInst, PtrToIntInst or ShuffleVectorInst. Other operands
// will trigger an assert.
Value* ValueTracker::handleExtractElement(ExtractElementInst* E)
{
    uint64_t idx = 0;
    if (auto* CI = dyn_cast<ConstantInt>(E->getIndexOperand()))
    {
        idx = CI->getZExtValue();
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "dynamic index");
        return nullptr;
    }

    Value* baseValue = E->getVectorOperand();
    while (true)
    {
        if (auto* I = dyn_cast<InsertElementInst>(baseValue))
        {
            auto* pIdx = I->getOperand(2);
            if (isa<ConstantInt>(pIdx))
            {
                if (cast<ConstantInt>(pIdx)->getZExtValue() == idx)
                {
                    baseValue = I->getOperand(1);
                    break;
                }
                else
                {
                    baseValue = I->getOperand(0);
                }
            }
            else
            {
                IGC_ASSERT_MESSAGE(0, "dynamic index");
                return nullptr;
            }
        }
        else if (auto* I = dyn_cast<BitCastInst>(baseValue))
        {
            auto srcVT = dyn_cast<IGCLLVM::FixedVectorType>(I->getSrcTy());
            auto dstVT = dyn_cast<IGCLLVM::FixedVectorType>(I->getDestTy());

            if (!srcVT || !dstVT) {
                // If any of the two types is not a vector type then it is an unknown situation.
                // Such a bitcast may have not been thought of and needs implementation or code may have been corrupted.
                IGC_ASSERT_MESSAGE(0, "unknown construct!");
                return nullptr;
            }

            auto srcNElts = srcVT->getNumElements();
            auto dstNElts = dstVT->getNumElements();

            if (srcNElts * 2 != dstNElts) {
                IGC_ASSERT_MESSAGE(0, "Can't handle vector bitcast with given sizes");
                return nullptr;
            }

            // Destination vector is twice as long.
            // Check if the dstType is twice as narrow.

            auto srcVEltType = srcVT->getElementType();
            auto dstVEltType = dstVT->getElementType();

            auto srcVEltTypeSize = srcVEltType->getPrimitiveSizeInBits();
            auto dstVEltTypeSize = dstVEltType->getPrimitiveSizeInBits();

            if (srcVEltTypeSize != dstVEltTypeSize * 2) {
                IGC_ASSERT_MESSAGE(0, "Can't handle vector bitcast with given types and sizes");
                return nullptr;
            }

            // Destination type is twice as narrow.
            // Shift the element index and continue.

            idx /= 2;
            baseValue = I->getOperand(0);
            continue;
        }
        else if (auto* I = dyn_cast<PtrToIntInst>(baseValue))
        {
            baseValue = I->getOperand(0);
            continue;
        }
        else if (auto* I = dyn_cast<ShuffleVectorInst>(baseValue))
        {
            auto mask = I->getShuffleMask();
            uint shuffleidx = int_cast<uint>(mask[(uint)idx]);
            auto vType = dyn_cast<IGCLLVM::FixedVectorType>(I->getOperand(0)->getType());
            baseValue = (shuffleidx < vType->getNumElements()) ?
                I->getOperand(0) : I->getOperand(1);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "unknown construct!");
            return nullptr;
        }
    }
    return baseValue;
}

// This function is called when a global variable is met during the first step
// of the overall algorithm. It currently supports global sampler only.
Value* ValueTracker::handleGlobalVariable(GlobalVariable* G)
{
    Constant* pSamplerVal = G->getInitializer();
    // Add debug info intrinsic for this variable inside the function using this sampler.
    Instruction* pEntryPoint = &(*m_Function->getEntryBlock().getFirstInsertionPt());
    IF_DEBUG_INFO(DebugInfoUtils::UpdateGlobalVarDebugInfo(G, pSamplerVal, pEntryPoint, false);)
        // Found a global sampler, return it.
        return isa<ConstantStruct>(pSamplerVal) ?
        pSamplerVal->getAggregateElement(0U) : pSamplerVal;
}

// This function is called when a constant expression is met during the first step
// of the overall algorithm. It currently supports only sampler index retrieving.
Value* ValueTracker::handleConstExpr(ConstantExpr* CE)
{
    uint64_t samplerState;
    uint64_t samplerIndex;

    // To handle Inline samplers defined as global variables
    if (m_pMDUtils == nullptr)
    {
        return nullptr;
    }

    // Get the sampler Index first
    if (CE->getOpcode() == Instruction::PtrToInt)
    {
        Value* ptrVal = CE->getOperand(0);
        if (isa<ConstantPointerNull>(ptrVal))
        {
            samplerIndex = 0;
        }
        else if (auto* ptrExpr = dyn_cast<ConstantExpr>(ptrVal))
        {
            if (ptrExpr->getOpcode() == Instruction::IntToPtr)
            {
                Value* samplerIdxVal = ptrExpr->getOperand(0);
                ConstantInt* C = dyn_cast<ConstantInt>(samplerIdxVal);
                if (!C)
                {
                    // Cannot trace, it could be a bindless or indirect access
                    return nullptr;
                }
                samplerIndex = int_cast<uint64_t>(C->getZExtValue());
            }
            else
            {
                // Cannot trace, it could be a bindless or indirect access
                return nullptr;
            }
        }
        else
        {
            // Cannot trace, it could be a bindless or indirect access
            return nullptr;
        }
    }
    else
    {
        // Cannot trace, it could be a bindless or indirect access
        return nullptr;
    }
    // Get the sampler state value from metadata based on the sampler index
    bool samplerIndexFound = false;
    if (m_pModMD->FuncMD.find(m_Function) != m_pModMD->FuncMD.end())
    {
        FunctionMetaData funcMD = m_pModMD->FuncMD.find(m_Function)->second;
        ResourceAllocMD resAllocMD = funcMD.resAllocMD;
        for (auto i = resAllocMD.inlineSamplersMD.begin(), e = resAllocMD.inlineSamplersMD.end(); i != e; ++i)
        {
            InlineSamplersMD inlineSamplerMD = *i;
            if (samplerIndex == inlineSamplerMD.index)
            {
                samplerState = inlineSamplerMD.m_Value;
                samplerIndexFound = true;
                break;
            }
        }
    }
    if (samplerIndexFound)
    {
        Value* samplerConstValue = ConstantInt::getIntegerValue(Type::getInt64Ty(m_Function->getContext()), APInt(64, samplerState));
        return samplerConstValue;
    }
    else
    {
        // Cannot trace, it could be a bindless or indirect access
        return nullptr;
    }
}

// This function represents the second step of the overall algorithm. It goes
// down through the tree and looks for the value stored in alloca. In most cases
// it returns the final value (image, sampler or constant). For more complex cases,
// alloca can store a pointer, so we need to get back to the first step of the algorithm
// to continue tracking.
Value* ValueTracker::findAllocaValue(Value* V, const uint depth)
{
    if (!V) return nullptr;

    visitedValues.insert(V);
    for (auto U : V->users())
    {
        if (visitedValues.find(U) != visitedValues.end()) continue;
        visitedValues.insert(U);

        if (auto* GEP = dyn_cast<GetElementPtrInst>(U))
        {
            if (!GEP->hasAllConstantIndices())
                return nullptr;

            unsigned numIndices = GEP->getNumIndices();
            if (numIndices > depth + 1)
                continue;

            bool matchingGep = false;
            for (unsigned int i = 1; i < numIndices; ++i)
            {
                if (gepIndices[depth - i]->getZExtValue() == cast<ConstantInt>(GEP->getOperand(i + 1))->getZExtValue())
                    matchingGep = true;
                else
                {
                    matchingGep = false;
                    break;
                }
            }

            if (!matchingGep)
                continue;

            unsigned reducedIndices = numIndices - 1;
            if (auto leaf = findAllocaValue(GEP, depth - reducedIndices))
            {
                IGC_ASSERT(gepIndices.size() >= reducedIndices);
                gepIndices.resize(gepIndices.size() - reducedIndices);
                return leaf;
            }
        }
        else if (auto* CI = dyn_cast<CastInst>(U))
        {
            if (auto leaf = findAllocaValue(CI, depth))
                return leaf;
        }
        else if (auto* CI = dyn_cast<CallInst>(U))
        {
            if (CI->getCalledFunction()->getIntrinsicID() == Intrinsic::memcpy)
            {
                return CI->getOperand(1);
            }
            else if (!CI->getCalledFunction()->isIntrinsic()) // handle user-defined functions
            {
                for (const auto& OP : CI->operands())
                {
                    if (OP == V)
                    {
                        Function* F = CI->getCalledFunction();
                        unsigned OpNo = OP.getOperandNo();
                        IGC_ASSERT(F->arg_size() > OpNo);
                        if (auto leaf = findAllocaValue(F->arg_begin() + OpNo, depth))
                        {
                            callInsts.push_back(CI);
                            return leaf;
                        }
                    }
                }
            }
        }
        else if (auto* LI = dyn_cast<LoadInst>(U))
        {
            // Continue tracing load if it's type is a pointer. Example(tracing %1 alloca value):
            // %0 = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
            // %1 = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
            // %2 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %1, align 8
            // store %opencl.image2d_t.read_only addrspace(1)* %2, %opencl.image2d_t.read_only addrspace(1)** %0, align 8
            // %3 = load % opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %0, align 8
            // We cannot ignore load if alloca type is a pointer.
            if (LI->getType()->isPointerTy())
            {
                if (auto leaf = findAllocaValue(LI, depth))
                    return leaf;
            }
        }
        else if (auto* ST = dyn_cast<StoreInst>(U))
        {
            auto StoredValue = ST->getValueOperand();
            if (StoredValue == V)
            {
                // If we are here, it means that alloca value is stored into another alloca.
                // Check if value is pointer type, if so, it means that our object can be accessed
                // through another alloca and we need to continue tracing it.
                if (StoredValue->getType()->isPointerTy())
                {
                    if (auto leaf = findAllocaValue(ST->getPointerOperand(), depth))
                        return leaf;
                }
            }
            else
                return ST->getValueOperand();
        }
    }
    return nullptr;
}

// This function represents the first step of the overall algorithm. It goes up through
// the tree and looks for the alloca that stores a value used as a call instruction parameter.
// Once alloca is found, the function findAllocaValue is called which is the second step
// of the algorithm.
Value* ValueTracker::trackValue(CallInst* CI, const uint index)
{
    Value* baseValue = CI->getOperand(index);
    auto isFinalValue = [this](auto V) { return callInsts.empty() && (V == nullptr || llvm::isa<Argument>(V) || llvm::isa<ConstantInt>(V)); };

    while (true)
    {
        if (isFinalValue(baseValue))
            return baseValue;

        visitedValues.insert(baseValue);

        if (auto* I = dyn_cast<Argument>(baseValue))
        {
            // If we are here, it means that baseValue is an argument of function not argument of kernel,
            // so we need to continue tracking
            IGC_ASSERT(!callInsts.empty());
            CallInst* CI = callInsts.back();
            IGC_ASSERT(CI->getNumOperands() > I->getArgNo());
            baseValue = CI->getOperand(I->getArgNo());
            // Remove the last call instruction as callee function body has already been processed
            // by tracing algorithm
            callInsts.pop_back();
        }
        else if (auto* I = dyn_cast<AllocaInst>(baseValue))
        {
            // As alloca has been found, proceed with the second step of the algorithm.
            baseValue = findAllocaValue(I, gepIndices.size());
        }
        else if (auto* I = dyn_cast<CallInst>(baseValue))
        {
            Function* F = I->getCalledFunction();
            if (F->getName() == "__translate_sampler_initializer")
            {
                baseValue = cast<CallInst>(baseValue)->getOperand(0);
            }
            else if (auto* I = dyn_cast<GenIntrinsicInst>(baseValue))
            {
                baseValue = handleGenIntrinsic(I);
            }
            else
            {
                baseValue = nullptr;
            }
        }
        else if (auto* I = dyn_cast<CastInst>(baseValue))
        {
            baseValue = I->getOperand(0);
        }
        else if (auto* I = dyn_cast<ExtractElementInst>(baseValue))
        {
            baseValue = handleExtractElement(I);
        }
        else if (auto* I = dyn_cast<GetElementPtrInst>(baseValue))
        {
            if (!I->hasAllConstantIndices())
                return nullptr;

            for (unsigned int i = I->getNumIndices(); i > 1; --i)
                gepIndices.push_back(cast<ConstantInt>(I->getOperand(i)));

            baseValue = I->getOperand(0);
        }
        else if (auto* I = dyn_cast<LoadInst>(baseValue))
        {
            Value* addr = I->getPointerOperand();
            if (GlobalVariable * globalSampler = dyn_cast<GlobalVariable>(addr->stripPointerCasts()))
            {
                return handleGlobalVariable(globalSampler);
            }

            baseValue = addr;
        }
        else if (auto* I = llvm::dyn_cast<ConstantExpr> (baseValue))
        {
            baseValue = handleConstExpr(I);
        }
        else
        {
            baseValue = nullptr;
        }
    }
    return nullptr;
}

// This is a static function, created for user convenience, that creates a ValueTracker
// object and triggers an actual tracking.
Value* ValueTracker::track(
    CallInst* pCallInst,
    const uint index,
    const MetaDataUtils* pMdUtils,
    const IGC::ModuleMetaData* pModMD)
{
    ValueTracker VT(pCallInst->getParent()->getParent(), pMdUtils, pModMD);
    return VT.trackValue(pCallInst, index);
}