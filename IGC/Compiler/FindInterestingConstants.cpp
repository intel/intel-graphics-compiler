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

#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Compiler/CodeGenPublic.h"
#include "common/secure_mem.h"
#include "FindInterestingConstants.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-find-interesting-constants"
#define PASS_DESCRIPTION "Find interesting constants"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FindInterestingConstants, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FindInterestingConstants, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char FindInterestingConstants::ID = 0;

#define DEBUG_TYPE "FindInterestingConstants"

FindInterestingConstants::FindInterestingConstants() : FunctionPass(ID)
{
    initializeFindInterestingConstantsPass(*PassRegistry::getPassRegistry());
}

bool FindInterestingConstants::FoldsToConst(Instruction* inst, Instruction* use, bool& propagate)
{
    propagate = false;

    // "use" instruction should have some operand(s)
    assert(use->getNumOperands() != 0);
    if (dyn_cast<BranchInst>(use))
    {
        m_constFoldBranch = true;
        return false;
    }

    for (auto& U : use->operands())
    {
        Value* V = U.get();
        if (V == inst)
            continue;
        else if (dyn_cast<Constant>(V))
            continue;
        else
        {
            // For select instruction all operands need not be constants to simplify the instruction
            if (SelectInst * selInst = dyn_cast<SelectInst>(use))
            {
                if (selInst->getOperand(0) == inst)
                    return true;
            }
            else
                return false;
        }
    }
    propagate = true;
    return true;
}

void FindInterestingConstants::FoldsToConstPropagate(llvm::Instruction* I)
{
    bool propagate = false;
    // if instruction count that can be folded to zero reached threshold, dont loop through
    for (auto UI = I->user_begin(), UE = I->user_end(); (UI != UE); ++UI)
    {
        if ((m_constFoldBranch) ||
            (m_foldsToConst >= IGC_GET_FLAG_VALUE(FoldsToConstPropThreshold)))
            break;

        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (useInst->getParent() == I->getParent())    // TBD Do we need this
            {
                if (FoldsToConst(I, useInst, propagate))
                {
                    m_foldsToConst++;
                    if (propagate)
                        FoldsToConstPropagate(useInst);
                }
            }
        }
    }
}

bool FindInterestingConstants::FoldsToZero(Instruction* inst, Instruction* use)
{
    bool propagate = false;
    if (dyn_cast<BranchInst>(use))
    {
        m_constFoldBranch = true;
        return false;
    }
    if (BinaryOperator * binInst = dyn_cast<BinaryOperator>(use))
    {
        if (binInst->getOpcode() == Instruction::FMul)
        {
            return true;
        }
        else if (binInst->getOpcode() == Instruction::FDiv &&
            inst == binInst->getOperand(0) && inst != binInst->getOperand(1))
        {
            return true;
        }
    }
    if (FoldsToConst(inst, use, propagate))
    {
        m_foldsToConst++;
        if (propagate)
            FoldsToConstPropagate(use);
    }
    return false;
}

void FindInterestingConstants::FoldsToZeroPropagate(llvm::Instruction* I)
{
    for (auto UI = I->user_begin(), UE = I->user_end(); (UI != UE); ++UI)
    {
        if ((m_constFoldBranch) ||
            (m_foldsToZero >= IGC_GET_FLAG_VALUE(FoldsToZeroPropThreshold)))
            break;
        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (FoldsToZero(I, useInst))
            {
                m_foldsToZero++;
                FoldsToZeroPropagate(useInst);
            }
        }
    }
}

bool FindInterestingConstants::FoldsToSource(llvm::Instruction* inst, llvm::Instruction* use)
{
    if (BinaryOperator * binInst = dyn_cast<BinaryOperator>(use))
    {
        if (binInst->getOpcode() == Instruction::FMul)
        {
            return true;
        }
        else if (binInst->getOpcode() == Instruction::FDiv &&
            inst == binInst->getOperand(1))
        {
            return true;
        }
    }
    return false;
}

void FindInterestingConstants::FoldsToSourcePropagate(llvm::Instruction* I)
{
    for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI)
    {
        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (FoldsToSource(I, useInst))
            {
                m_foldsToSource++;
                if (m_foldsToSource >= IGC_GET_FLAG_VALUE(FoldsToSourceThreshold))
                {
                    break;
                }
            }
        }
    }
}

// Get constant address from load instruction
bool FindInterestingConstants::getConstantAddress(llvm::LoadInst& I, unsigned& bufIdOrGRFOffset, unsigned& eltId, int& size_in_bytes)
{
    // Check if the load instruction is with constant buffer address
    unsigned as = I.getPointerAddressSpace();
    bool directBuf = false;
    bool statelessBuf = false;
    size_in_bytes = 0;
    BufferType bufType;
    Value * pointerSrc = nullptr;

    if (as == ADDRESS_SPACE_CONSTANT)
    {
        // If the buffer info is not encoded in the address space, we can still find it by
        // tracing the pointer to where it's created.
        if (!GetStatelessBufferInfo(I.getPointerOperand(), bufIdOrGRFOffset, bufType, pointerSrc, directBuf))
        {
            return false;
        }
        if (!directBuf)
        {
            // Make sure constant folding is safe by looking up in pushableAddresses
            PushInfo& pushInfo = m_context->getModuleMetaData()->pushInfo;

            for (auto it : pushInfo.pushableAddresses)
            {
                if ((bufIdOrGRFOffset * 4 == it.addressOffset) && (IGC_IS_FLAG_ENABLED(DisableStaticCheck) || it.isStatic))
                {
                    statelessBuf = true;
                    break;
                }
            }
        }
    }
    else
    {
        bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufIdOrGRFOffset);
    }
    // If it is statelessBuf, we made sure it is a constant buffer by finding it in pushableAddresses
    if ((directBuf && (bufType == CONSTANT_BUFFER)) || statelessBuf)
    {
        Value* ptrVal = I.getPointerOperand();
        eltId = 0;

        if (!EvalConstantAddress(ptrVal, eltId, m_DL, pointerSrc))
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    size_in_bytes = I.getType()->getPrimitiveSizeInBits() / 8;
    return true;
}

void FindInterestingConstants::addInterestingConstant(llvm::Type* loadTy, unsigned bufIdOrGRFOffset, unsigned eltId, int size_in_bytes, bool anyValue, uint32_t value = 0)
{
    // For constant buffer accesses of size <= 32bit.
    if (!loadTy->isVectorTy())
    {
        if (size_in_bytes <= 4)
        {
            SConstantAddrValue cl;
            cl.ca.bufId = bufIdOrGRFOffset;
            cl.ca.eltId = eltId;
            cl.ca.size = size_in_bytes;
            cl.anyValue = anyValue;
            cl.value = value;

            m_InterestingConstants.push_back(cl);
        }
    }
    else
    {
        // Vectors case
        //  For now we can only detect anyValue=1 cases in vector type loads
        if (anyValue)
        {
            Type * srcEltTy = loadTy->getVectorElementType();
            unsigned srcNElts = loadTy->getVectorNumElements();
            unsigned eltSize_in_bytes = srcEltTy->getPrimitiveSizeInBits() / 8;
            for (unsigned i = 0; i < srcNElts; i++)
            {
                SConstantAddrValue cl;
                cl.ca.bufId = bufIdOrGRFOffset;
                cl.ca.eltId = eltId + (i * eltSize_in_bytes);
                cl.ca.size = eltSize_in_bytes;
                cl.anyValue = anyValue;
                cl.value = value;

                m_InterestingConstants.push_back(cl);
            }
        }
    }
}

void FindInterestingConstants::visitLoadInst(llvm::LoadInst& I)
{

    switch (IGC_GET_FLAG_VALUE(ConstantLoadTypeCheck))
    {
    case 1:
        if (!I.getType()->isIntOrIntVectorTy())  return;
        break;
    case 2:
        if (!I.getType()->isFPOrFPVectorTy())  return;
        break;
    case 0:
    default:
        break;
    }

    unsigned bufIdOrGRFOffset;
    unsigned eltId;
    int size_in_bytes;

    m_foldsToZero = 0;
    m_foldsToConst = 0;
    m_foldsToSource = 0;
    m_constFoldBranch = false;
    if (getConstantAddress(I, bufIdOrGRFOffset, eltId, size_in_bytes))
    {
        /*
        This Constant is interesting, if the use instruction:
        is branch
        or subsequent Instructions get folded to constant if the constant value is known
        or subsequent Instructions get folded to zero if the constant value is 0
        or subsequent Instructions get folded to its source if the constant value is 1 (mul/div by 1 scenarios)
        */
        FoldsToConstPropagate(&I);
        // If m_foldsToConst is greater than threshold or some branch instruction gets simplified because of this constant
        if ((m_constFoldBranch) || (m_foldsToConst >= IGC_GET_FLAG_VALUE(FoldsToConstPropThreshold)))
        {
            // Get the ConstantAddress from LoadInst and log it in interesting constants
            addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, true);
        }
        else
        {
            m_foldsToConst = 0;     // Reset FoldsToConst count to zero. We can keep looking for this case when FoldsToZero cannot be propagated further
            FoldsToZeroPropagate(&I);
            // If m_foldsToZero is greater than threshold or some branch instruction gets simplified because of this constant
            if ((m_constFoldBranch) ||
                ((m_foldsToZero + m_foldsToConst) >= IGC_GET_FLAG_VALUE(FoldsToZeroPropThreshold)))
            {
                // Zero value for this constant is interesting
                // Get the ConstantAddress from LoadInst and log it in interesting constants
                addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, 0);
                // Continue finding if ONE_VALUE is beneficial for this constant
            }

            FoldsToSourcePropagate(&I);
            if (m_foldsToSource >= IGC_GET_FLAG_VALUE(FoldsToSourceThreshold))
            {
                // One value for this constant is interesting
                // Get the ConstantAddress from LoadInst and log it in interesting constants
                if (I.getType()->isIntegerTy())
                {
                    addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, 1);
                }
                else if (I.getType()->isFloatTy())
                {
                    uint32_t value;
                    float floatValue = 1.0;
                    memcpy_s(&value, sizeof(uint32_t), &floatValue, sizeof(float));
                    addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, value);
                }
            }
        }
    }
}

template<typename ContextT>
void FindInterestingConstants::copyInterestingConstants(ContextT* pShaderCtx)
{
    pShaderCtx->programOutput.m_pInterestingConstants = m_InterestingConstants;
}

bool FindInterestingConstants::doFinalization(llvm::Module& M)
{
    if (m_InterestingConstants.size() != 0)
    {

        if (m_context->type == ShaderType::PIXEL_SHADER)
        {
            PixelShaderContext* pShaderCtx = static_cast <PixelShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
        else if (m_context->type == ShaderType::VERTEX_SHADER)
        {
            VertexShaderContext* pShaderCtx = static_cast <VertexShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
        else if (m_context->type == ShaderType::GEOMETRY_SHADER)
        {
            GeometryShaderContext* pShaderCtx = static_cast <GeometryShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
        else if (m_context->type == ShaderType::HULL_SHADER)
        {
            HullShaderContext* pShaderCtx = static_cast <HullShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
        else if (m_context->type == ShaderType::DOMAIN_SHADER)
        {
            DomainShaderContext* pShaderCtx = static_cast <DomainShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
        else if (m_context->type == ShaderType::COMPUTE_SHADER)
        {
            ComputeShaderContext* pShaderCtx = static_cast <ComputeShaderContext*>(m_context);
            copyInterestingConstants(pShaderCtx);
        }
    }
    return false;
}

bool FindInterestingConstants::runOnFunction(Function& F)
{
    m_DL = &F.getParent()->getDataLayout();
    m_context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    visit(F);
    return false;
}
