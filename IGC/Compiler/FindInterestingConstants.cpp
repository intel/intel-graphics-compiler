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

template<typename ContextT>
void FindInterestingConstants::copyInterestingConstants(ContextT* pShaderCtx)
{
    pShaderCtx->programOutput.m_InterestingConstantsSize = m_InterestingConstants.size();
    pShaderCtx->programOutput.m_pInterestingConstants = new USC::ConstantAddrValue[m_InterestingConstants.size()];
    memcpy_s(pShaderCtx->programOutput.m_pInterestingConstants, m_InterestingConstants.size() * sizeof(USC::ConstantAddrValue),
        &m_InterestingConstants[0],
        m_InterestingConstants.size() * sizeof(USC::ConstantAddrValue));
}

bool FindInterestingConstants::doFinalization(llvm::Module &M)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    
    if (m_InterestingConstants.size() != 0)
    {
        if (ctx->type == ShaderType::PIXEL_SHADER)
        {
            PixelShaderContext* pShaderCtx = static_cast <PixelShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
        else if (ctx->type == ShaderType::VERTEX_SHADER)
        {
            VertexShaderContext* pShaderCtx = static_cast <VertexShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
        else if (ctx->type == ShaderType::GEOMETRY_SHADER)
        {
            GeometryShaderContext* pShaderCtx = static_cast <GeometryShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
        else if (ctx->type == ShaderType::HULL_SHADER)
        {
            HullShaderContext* pShaderCtx = static_cast <HullShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
        else if (ctx->type == ShaderType::DOMAIN_SHADER)
        {
            DomainShaderContext* pShaderCtx = static_cast <DomainShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
        else if (ctx->type == ShaderType::COMPUTE_SHADER)
        {
            ComputeShaderContext* pShaderCtx = static_cast <ComputeShaderContext*>(ctx);
            copyInterestingConstants(pShaderCtx);
        }
    }
    return false;
}

bool FindInterestingConstants::runOnFunction(Function &F)
{
    visit(F);
    return false;
}

bool FindInterestingConstants::FoldsToZero(Instruction* inst, Value* use)
{
    if (BranchInst* brInst = dyn_cast<BranchInst>(use))
    {
        m_constFoldBranch = true;
        return false;
    }
    if (BinaryOperator *binInst = dyn_cast<BinaryOperator>(use))
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
    return false;
}

bool FindInterestingConstants::FoldsToConst(Instruction* inst, Instruction* use)
{
    // "use" instruction should have some operand(s)
    assert(use->getNumOperands() != 0);
    if (BranchInst* brInst = dyn_cast<BranchInst>(use))
    {
        m_constFoldBranch = true;
        return false;
    }

    for (auto &U : use->operands())
    {
        Value *V = U.get();
        if (V == inst)
            continue;
        else if (Constant* op = dyn_cast<Constant>(V))
            continue;
        else
            return false;
    }
    return true;
}

void FindInterestingConstants::FoldsToZeroPropagate(llvm::Instruction* I)
{
    for (auto UI = I->user_begin(), UE = I->user_end(); ((UI != UE) && (m_zeroFolded < IGC_GET_FLAG_VALUE(FoldsToZeroPropThreshold))); ++UI)
    {
        if ((m_constFoldBranch) ||
            (m_zeroFolded >= IGC_GET_FLAG_VALUE(FoldsToZeroPropThreshold)))
            break;
        if (Instruction* useInst = dyn_cast<Instruction>(*UI))
        {
            if (FoldsToZero(I, useInst))
            {
                m_zeroFolded++;
                FoldsToZeroPropagate(useInst);
            }
        }
    }
}

void FindInterestingConstants::FoldsToConstPropagate(llvm::Instruction* I)
{
    // if instruction count that can be folded to zero reached threshold, dont loop through 
    for (auto UI = I->user_begin(), UE = I->user_end(); (UI != UE); ++UI)
    {
        if ((m_constFoldBranch) ||
            (m_constFolded >= IGC_GET_FLAG_VALUE(FoldsToConstPropThreshold)))
            break;

        if (Instruction* useInst = dyn_cast<Instruction>(*UI))
        {
            if (useInst->getParent() == I->getParent())	// TBD Do we need this
            {
                if (FoldsToConst(I,useInst))
                {
                    m_constFolded++;
                    FoldsToConstPropagate(useInst);
                }
            }
        }
    }
}

// Get constant address from load instruction
bool FindInterestingConstants::getConstantAddress(llvm::LoadInst &I, unsigned &bufId, unsigned &eltId, int &size_in_bytes)
{
    // Check if the load instruction is with constant buffer address
    unsigned as = I.getPointerAddressSpace();
    bool directBuf;
    size_in_bytes = 0;
    BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);

    if (bufType == CONSTANT_BUFFER && directBuf)
    {
        Value *ptrVal = I.getPointerOperand();
        eltId = 0;

        if (isa<ConstantPointerNull>(ptrVal))
        {
            eltId = 0;
        }
        else if (ConstantExpr *ptrExpr = dyn_cast<ConstantExpr>(ptrVal))
        {
            if (ptrExpr->getOpcode() == Instruction::IntToPtr)
            {
                Value *eltIdxVal = ptrExpr->getOperand(0);
                ConstantInt *eltIdx = dyn_cast<ConstantInt>(eltIdxVal);
                if (!eltIdx)
                    return false;
                eltId = int_cast<unsigned>(eltIdx->getZExtValue());
            }
            else
            {
                return false;
            }
        }
        else if (IntToPtrInst *i2p = dyn_cast<IntToPtrInst>(ptrVal))
        {
            Value *eltIdxVal = i2p->getOperand(0);
            ConstantInt *eltIdx = dyn_cast<ConstantInt>(eltIdxVal);
            if (!eltIdx)
                return false;
            eltId = int_cast<unsigned>(eltIdx->getZExtValue());
        }
        else
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

void FindInterestingConstants::addInterestingConstant(CodeGenContext* ctx, unsigned bufId, unsigned eltId, int size_in_bytes, USC::VALUE_HINT valueHint)
{
    USC::ConstantAddrValue cl;
    cl.ca.bufId = bufId;
    cl.ca.eltId = eltId;
    cl.ca.size = size_in_bytes;
    cl.valueHint = valueHint;

    m_InterestingConstants.push_back(cl);
}

void FindInterestingConstants::visitLoadInst(llvm::LoadInst &I)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    bool isInteresting = false;
    unsigned bufId;
    unsigned eltId;
    int size_in_bytes;
    USC::VALUE_HINT valueHint;

    m_zeroFolded = 0;
    m_constFolded = 0;
    m_constFoldBranch = false;
    valueHint = USC::ANY_VALUE;
    if(getConstantAddress(I, bufId, eltId, size_in_bytes))
    {
        for (auto UI = I.user_begin(), UE = I.user_end(); UI != UE; ++UI)
        {
            if (Instruction* useInst = dyn_cast<Instruction>(*UI))
            {
                /*
                    This Constant is interesting, if the use instruction:
                            is branch
                            and subsequent Instructions get folded to zero if the constant value is 0 (FoldsToZeroPropThreshold)
                            and subsequent Instructions get folded to constant if the constant value is known 
                */

                if (FoldsToConst(&I, useInst))
                {
                    m_constFolded++;
                    FoldsToConstPropagate(useInst);
                }
                // If m_constFolded is greater than threshold or some branch instruction gets simplified because of this constant
                if ((m_constFoldBranch) || (m_constFolded >= IGC_GET_FLAG_VALUE(FoldsToConstPropThreshold)))
                {
                    isInteresting = true;
                    break;
                }
                    
                if (FoldsToZero(&I, useInst))
                {
                    valueHint = USC::ZERO_VALUE;
                    m_zeroFolded++;

                    FoldsToZeroPropagate(useInst);
                }
                // If m_zeroFolded is greater than threshold or some branch instruction gets simplified because of this constant
                if ((m_constFoldBranch) || (m_zeroFolded >= IGC_GET_FLAG_VALUE(FoldsToZeroPropThreshold)))
                {
                    isInteresting = true;
                    break;
                }
            }
        }
        if (isInteresting)
        {
            // Get the ConstantAddress from LoadInst and log it in interesting constants
            addInterestingConstant(ctx, bufId, eltId, size_in_bytes, valueHint);
        }
    }
}
