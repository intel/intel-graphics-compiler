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
#include "Probe/Assertion.h"

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

void FindInterestingConstants::UpdateInstCount(Instruction* inst)
{
    if (IsExtendedMathInstruction(inst))
    {
        m_extendedMath++;
    }
    else if (SelectInst * selInst = dyn_cast<SelectInst>(inst))
    {
        if (dyn_cast<Constant>(selInst->getOperand(1)) && dyn_cast<Constant>(selInst->getOperand(2)))
            m_instCount++;
        else
            m_selectCount++;
    }
    else
        m_instCount++;
}


bool FindInterestingConstants::isReverseOpInstPair(llvm::Intrinsic::ID intr1, llvm::Intrinsic::ID intr2)
{
    std::map<llvm::Intrinsic::ID, llvm::Intrinsic::ID> reverseOpInstPair;
    reverseOpInstPair[llvm::Intrinsic::exp] = llvm::Intrinsic::log;
    reverseOpInstPair[llvm::Intrinsic::log] = llvm::Intrinsic::exp;
    reverseOpInstPair[llvm::Intrinsic::exp2] = llvm::Intrinsic::log2;
    reverseOpInstPair[llvm::Intrinsic::log2] = llvm::Intrinsic::exp2;

    if ((reverseOpInstPair.find(intr1) != reverseOpInstPair.end()) && (reverseOpInstPair[intr1] == intr2))
        return true;
    return false;
}

bool FindInterestingConstants::FoldsToConst(Instruction* inst, Instruction* use, bool& propagate)
{
    propagate = false;

    // "use" instruction should have some operand(s)
    IGC_ASSERT(use->getNumOperands() != 0);
    if (BranchInst* Br = dyn_cast<BranchInst>(use))
    {
        bool isLoop;
        unsigned int brSize;
        brSize = BranchSize(inst, Br, isLoop);
        if (isLoop)
        {
            m_loopSize += brSize;
            m_constFoldLoopBranch++;
        }
        else
        {
            m_branchsize += brSize;
            m_constFoldBranch++;
        }
        visitedForFolding.insert(Br);
        return false;
    }
    if (llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(use))
    {
        // Figure out the intrinsic operands for texture & sampler
        Value* pTextureValue = nullptr, * pSamplerValue = nullptr;
        IGC::getTextureAndSamplerOperands(pIntr, pTextureValue, pSamplerValue);
        if (pSamplerValue && pSamplerValue == inst)
        {
            m_samplerCount++;
            visitedForFolding.insert(pIntr);
            return false;
        }
        if (!IsMathIntrinsic(GetOpCode(use)))
        {
            return false;
        }
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
            if (SelectInst* selInst = dyn_cast<SelectInst>(use))
            {
                // For select instruction all operands need not be constants to simplify the instruction
                if (selInst->getOperand(0) == inst)
                    return true;
            }
            else if (CmpInst* Cmp = dyn_cast<CmpInst>(use))
            {
                // For Cmp instruction we dont need the other operand to be a constant if it is leading to a loop terminating condition
                // If any of the uses of cmp instruction are loop terminating conditions, update the loop stats accordingly,
                //      as this constant buffer access in compare instruction can help loop unroll
                for (auto UI = Cmp->user_begin(), UE = Cmp->user_end(); (UI != UE); ++UI)
                {
                    if (BranchInst* Br = dyn_cast<BranchInst>(*UI))
                    {
                        bool isLoop;
                        unsigned int brSize;
                        brSize = BranchSize(inst, Br, isLoop);
                        if (isLoop)
                        {
                            visitedForFolding.insert(Br);
                            m_loopSize += brSize;
                            m_constFoldLoopBranch++;
                        }
                    }
                }
            }
            return false;
        }
    }
    propagate = true;
    return true;
}

void FindInterestingConstants::FoldsToConstPropagate(llvm::Instruction* I)
{
    bool propagate = false;
    // Look for all instructions that can be folded to constant if the value of I is known
    //      Also look for branches/loops in that path
    for (auto UI = I->user_begin(), UE = I->user_end(); (UI != UE); ++UI)
    {
        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (visitedForFolding.find(useInst) == visitedForFolding.end())
            {
                if (useInst->getParent() == I->getParent())    // TBD Do we need this
                {
                    if (FoldsToConst(I, useInst, propagate))
                    {
                        UpdateInstCount(useInst);
                        visitedForFolding.insert(useInst);
                        if (propagate)
                            FoldsToConstPropagate(useInst);
                    }
                }
            }
        }
    }
}

bool FindInterestingConstants::allUsersVisitedForFolding(Instruction* inst, Instruction* use)
{
    if (visitedForFolding.find(use) != visitedForFolding.end())
        return false;
    for (auto UI = inst->user_begin(), UE = inst->user_end(); (UI != UE); ++UI)
    {
        Value* anotherUse = *UI;
        if (Instruction* anotherUseInst = dyn_cast<Instruction>(anotherUse))
        {
            if ((anotherUseInst != use) && (visitedForFolding.find(anotherUseInst) == visitedForFolding.end()))
            {
                return false;
            }
            // If the use instruction is selectInst/GenISA instruction then dont count as visited for folding
            //      as such instructions are currently added to visitedForFolding to avoid double counting in stats
            llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(anotherUse);
            SelectInst* selInst = dyn_cast<SelectInst>(anotherUse);
            if ((pIntr && !IsMathIntrinsic(GetOpCode(anotherUseInst))) || selInst)
            {
                return false;
            }
        }
        else
            return false;
    }
    return true;    // All other uses other than use are part of visitedForFolding, use must not be part of visitedForFolding already
}

void FindInterestingConstants::CheckIfSampleBecomesDeadCode(Instruction* inst, Instruction* use)
{
    if (ExtractElementInst* ee = llvm::dyn_cast<llvm::ExtractElementInst> (inst))
    {
        llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(ee->getOperand(0));
        // Figure out if inst is smaple instruction, update m_samplerCount/m_foldsToZero/visitedForFolding accordingly
        if (pIntr)
        {
            Value* pTextureValue = nullptr, * pSamplerValue = nullptr;
            IGC::getTextureAndSamplerOperands(pIntr, pTextureValue, pSamplerValue);
            if (pSamplerValue)
            {
                if (allUsersVisitedForFolding(inst, use))
                {
                    m_instCount += IGC_GET_FLAG_VALUE(WeightSamplerScalarResult);
                    if (allUsersVisitedForFolding(pIntr, inst))
                    {
                        m_samplerCount++;
                        IGC_ASSERT(visitedForFolding.find(pIntr) == visitedForFolding.end());
                        visitedForFolding.insert(pIntr);
                    }
                    IGC_ASSERT(visitedForFolding.find(inst) == visitedForFolding.end());
                    visitedForFolding.insert(inst);
                }
            }
        }
    }
}

bool FindInterestingConstants::FoldsToZero(Instruction* inst, Instruction* use)
{
    bool propagate = false;
    if (BinaryOperator * binInst = dyn_cast<BinaryOperator>(use))
    {
        if (binInst->getOpcode() == Instruction::FMul)
        {
            Value* binOperand = nullptr;
            if (binInst->getOperand(0) == inst)
            {
                binOperand = binInst->getOperand(1);
            }
            else
            {
                binOperand = binInst->getOperand(0);
            }
            if (Instruction* binOperandDef = dyn_cast<Instruction>(binOperand))
                CheckIfSampleBecomesDeadCode(binOperandDef, binInst);
            return true;
        }
        else if (binInst->getOpcode() == Instruction::FDiv &&
            inst == binInst->getOperand(0) && inst != binInst->getOperand(1))
        {
            Value* binOperand = binInst->getOperand(1);
            // Figure out if binOperand is smaple instruction, update m_samplerCount accordingly
            if (Instruction* binOperandDef = dyn_cast<Instruction>(binOperand))
                CheckIfSampleBecomesDeadCode(binOperandDef, binInst);

            return true;
        }
    }
    // We can keep looking for FoldsToConst case when FoldsToZero cannot be propagated further
    if (FoldsToConst(inst, use, propagate))
    {
        UpdateInstCount(use);
        visitedForFolding.insert(use);
        if (propagate)
            FoldsToConstPropagate(use);
    }
    return false;
}

void FindInterestingConstants::FoldsToZeroPropagate(llvm::Instruction* I)
{
    for (auto UI = I->user_begin(), UE = I->user_end(); (UI != UE); ++UI)
    {
        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (visitedForFolding.find(useInst) == visitedForFolding.end())
            {
                if (FoldsToZero(I, useInst))
                {
                    visitedForFolding.insert(useInst);
                    UpdateInstCount(useInst);
                    FoldsToZeroPropagate(useInst);
                }
            }

        }
    }
}

bool FindInterestingConstants::FoldsToSource(llvm::Instruction* inst, llvm::Instruction* use)
{
    bool foldsToSource = false;
    llvm::Value* binOperand = nullptr;

    if (BinaryOperator * binInst = dyn_cast<BinaryOperator>(use))
    {
        if (binInst->getOpcode() == Instruction::FMul)
        {
            if (binInst->getOperand(0) == inst)
            {
                binOperand = binInst->getOperand(1);
            }
            else
            {
                binOperand = binInst->getOperand(0);
            }
            foldsToSource = true;
        }
        else if (binInst->getOpcode() == Instruction::FDiv &&
            inst == binInst->getOperand(1))
        {
            binOperand = binInst->getOperand(0);
            foldsToSource = true;
        }
    }

    // Check for cases where folds to source triggers optimizations like showing up instruction sequence with exp-log/log-exp
    if (foldsToSource)
    {
        // Figure out if binOperand is part of reverseOpInstPair (eg: log-exp/exp-log), update extended math count accordingly
        if (llvm::IntrinsicInst * intr = dyn_cast<IntrinsicInst>(binOperand))
        {
            for (auto UI = use->user_begin(), UE = use->user_end(); UI != UE; ++UI)
            {
                if (llvm::IntrinsicInst * useIntr = dyn_cast<IntrinsicInst>(*UI))
                {
                    if (isReverseOpInstPair(intr->getIntrinsicID(), useIntr->getIntrinsicID()))
                    {
                        // Increment extended math count
                        m_extendedMath += 2;
                        visitedForFolding.insert(intr);
                        visitedForFolding.insert(useIntr);
                    }
                }
            }
        }
    }
    return foldsToSource;
}

void FindInterestingConstants::FoldsToSourcePropagate(llvm::Instruction* I)
{
    for (auto UI = I->user_begin(), UE = I->user_end(); UI != UE; ++UI)
    {
        if (Instruction * useInst = dyn_cast<Instruction>(*UI))
        {
            if (visitedForFolding.find(useInst) == visitedForFolding.end())
            {
                if (FoldsToSource(I, useInst))
                {
                    UpdateInstCount(useInst);
                    visitedForFolding.insert(useInst);
                }
            }
        }
    }
}

unsigned int FindInterestingConstants::BranchSize(llvm::Instruction* I, llvm::BranchInst* Br, bool& isLoop)
{
    std::vector<int> BasicBlock_size;
    unsigned int size = 0;
    bool isDummyBB;
    //For loop instruction
    for (auto& LI : *m_LI)
    {
        Loop* L = &(*LI);
        BasicBlock* body = L->getLoopLatch();
        BranchInst* br = cast<BranchInst>(body->getTerminator());
        if (br == dyn_cast<BranchInst>(Br))
        {
            for (BasicBlock* BB : L->getBlocks())
            {
                size += BB->size();
            }
            isLoop = true;
            return size;
        }
        for (Loop* subLoop : L->getSubLoops())
        {
            body = subLoop->getLoopLatch();
            br = cast<BranchInst>(body->getTerminator());
            if (br == dyn_cast<BranchInst>(Br))
            {
                for (BasicBlock* BB : subLoop->getBlocks())
                {
                    size += BB->size();
                }
                isLoop = true;
                return size;
            }
        }
    }
    for (auto& U : Br->operands())
    {
        Value* V = U.get();
        if (V == I)
            continue;
        // For If/else condition
        else if (BasicBlock* BB = dyn_cast<BasicBlock>(V))
        {
            isDummyBB = IGC::isDummyBasicBlock(BB);
            if (isDummyBB)
            {
                BasicBlock_size.push_back(succ_begin(BB)->size());
            }
            else
                BasicBlock_size.push_back(BB->size());
        }
        else
            break;
    }
    if (BasicBlock_size.size())
        size += *std::max_element(BasicBlock_size.begin(), BasicBlock_size.end());
    isLoop = false;
    return size;
}

// Get constant address from load instruction
bool FindInterestingConstants::getConstantAddress(llvm::LoadInst& I, unsigned& bufIdOrGRFOffset, int& eltId, int& size_in_bytes)
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
    size_in_bytes = (unsigned int)I.getType()->getPrimitiveSizeInBits() / 8;
    return true;
}

void FindInterestingConstants::addInterestingConstant(llvm::Type* loadTy, unsigned bufIdOrGRFOffset, unsigned eltId, int size_in_bytes, bool anyValue, uint32_t constValue, InstructionStats stats)
{
    SConstantAddrValue interestingConst;
    interestingConst.ca.bufId = bufIdOrGRFOffset;
    interestingConst.ca.eltId = eltId;
    interestingConst.anyValue = anyValue;
    interestingConst.value = constValue;
    interestingConst.ca.size = size_in_bytes;
    interestingConst.instCount = stats.instCount;
    interestingConst.branchCount = stats.branchCount;
    interestingConst.loopCount = stats.loopCount;
    interestingConst.samplerCount = stats.samplerCount;
    interestingConst.selectCount = stats.selectCount;
    interestingConst.extendedMath = stats.extendedMath;
    interestingConst.weight = stats.weight;
    // For constant buffer accesses of size <= 32bit.
    if (!loadTy->isVectorTy())
    {
        if (size_in_bytes <= 4)
        {

            std::vector<SConstantAddrValue>& interestingConstantVector = m_InterestingConstants[interestingConst.ca.bufId];
            for (unsigned i = 0; i < interestingConstantVector.size(); i++)
            {
                if ((interestingConstantVector[i].ca.eltId == interestingConst.ca.eltId) &&
                    (interestingConstantVector[i].ca.size == interestingConst.ca.size) &&
                    (interestingConstantVector[i].anyValue == interestingConst.anyValue) &&
                    (interestingConstantVector[i].value == interestingConst.value))
                {

                    interestingConstantVector[i].instCount += interestingConst.instCount;
                    interestingConstantVector[i].branchCount += interestingConst.branchCount;
                    interestingConstantVector[i].loopCount += interestingConst.loopCount;
                    interestingConstantVector[i].samplerCount += interestingConst.samplerCount;
                    interestingConstantVector[i].extendedMath += interestingConst.extendedMath;
                    interestingConstantVector[i].weight += interestingConst.weight;
                    return;
                }
            }
            interestingConstantVector.push_back(interestingConst);
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
            unsigned eltSize_in_bytes = (unsigned int)srcEltTy->getPrimitiveSizeInBits() / 8;
            interestingConst.ca.size = eltSize_in_bytes;
            for (unsigned i = 0; i < srcNElts; i++)
            {
                interestingConst.ca.eltId = eltId + (i * eltSize_in_bytes);
                std::vector<SConstantAddrValue>& interestingConstantVector = m_InterestingConstants[interestingConst.ca.bufId];
                for (unsigned j = 0; j < interestingConstantVector.size(); j++)
                {
                    if ((interestingConstantVector[j].ca.eltId == interestingConst.ca.eltId) &&
                        (interestingConstantVector[j].ca.size == interestingConst.ca.size) &&
                        (interestingConstantVector[j].anyValue == interestingConst.anyValue) &&
                        (interestingConstantVector[j].value == interestingConst.value))
                    {

                        interestingConstantVector[i].instCount += interestingConst.instCount;
                        interestingConstantVector[i].branchCount += interestingConst.branchCount;
                        interestingConstantVector[i].loopCount += interestingConst.loopCount;
                        interestingConstantVector[i].samplerCount += interestingConst.samplerCount;
                        interestingConstantVector[i].extendedMath += interestingConst.extendedMath;
                        interestingConstantVector[i].weight += interestingConst.weight;
                        return;
                    }
                }
                interestingConstantVector.push_back(interestingConst);
            }
        }
    }
}

void FindInterestingConstants::ResetStatCounters()
{
    m_instCount = 0;
    m_constFoldBranch = 0;
    m_constFoldLoopBranch = 0;
    m_samplerCount = 0;
    m_selectCount = 0;
    m_extendedMath = 0;
    m_branchsize = 0;
    m_loopSize = 0;
    visitedForFolding.clear();
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
    int eltId;
    int size_in_bytes;
    uint32_t constValue;
    constValue = 0;
    ResetStatCounters();

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
        if (m_instCount || m_constFoldBranch || m_constFoldLoopBranch || m_samplerCount || m_extendedMath)
        {
            InstructionStats stats;
            stats.instCount = m_instCount;
            stats.branchCount = m_constFoldBranch;
            stats.loopCount = m_constFoldLoopBranch;
            stats.samplerCount = m_samplerCount;
            stats.extendedMath = m_extendedMath;
            stats.weight = (m_instCount * IGC_GET_FLAG_VALUE(WeightOtherInstruction)) + std::max(m_constFoldBranch * IGC_GET_FLAG_VALUE(BaseWeightBranch), m_branchsize * IGC_GET_FLAG_VALUE(WeightBranch)) +
                std::max(m_constFoldLoopBranch * IGC_GET_FLAG_VALUE(BaseWeightLoop), m_loopSize * IGC_GET_FLAG_VALUE(WeightLoop)) + (m_samplerCount * IGC_GET_FLAG_VALUE(WeightSampler)) +
                (m_extendedMath * IGC_GET_FLAG_VALUE(WeightExtendedMath)) + (m_selectCount * IGC_GET_FLAG_VALUE(WeightSelect));
            constValue = 0;
            // Get the ConstantAddress from LoadInst and log it in interesting constants
            addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, true, constValue, stats);
        }
        ResetStatCounters();
        FoldsToZeroPropagate(&I);
        // If m_foldsToZero is greater than threshold or some branch instruction gets simplified because of this constant
        if (m_constFoldBranch || m_constFoldLoopBranch || m_instCount || m_samplerCount || m_extendedMath)
        {
            InstructionStats stats;
            stats.instCount = m_instCount;
            stats.branchCount = m_constFoldBranch;
            stats.loopCount = m_constFoldLoopBranch;
            stats.samplerCount = m_samplerCount;
            stats.extendedMath = m_extendedMath;
            stats.selectCount = m_selectCount;
            stats.weight = (m_instCount * IGC_GET_FLAG_VALUE(WeightOtherInstruction)) + std::max(m_constFoldBranch * IGC_GET_FLAG_VALUE(BaseWeightBranch), m_branchsize * IGC_GET_FLAG_VALUE(WeightBranch)) +
                std::max(m_constFoldLoopBranch * IGC_GET_FLAG_VALUE(BaseWeightLoop), m_loopSize * IGC_GET_FLAG_VALUE(WeightLoop)) + (m_samplerCount * IGC_GET_FLAG_VALUE(WeightSampler)) +
                (m_extendedMath * IGC_GET_FLAG_VALUE(WeightExtendedMath)) + (m_selectCount * IGC_GET_FLAG_VALUE(WeightSelect));
            constValue = 0;
            // Zero value for this constant is interesting
            // Get the ConstantAddress from LoadInst and log it in interesting constants
            addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, constValue, stats);
            // Continue finding if ONE_VALUE is beneficial for this constant
        }

        if (IGC_IS_FLAG_ENABLED(EnableFoldsToSourceCheck))
        {
            //visitedForFolding.insert(&I); // TBD: No need to insert load instruction
            ResetStatCounters();
            FoldsToSourcePropagate(&I);
            if (m_instCount)
            {
                InstructionStats stats;
                stats.instCount = m_instCount;
                stats.extendedMath = m_extendedMath;
                stats.weight = (m_instCount * IGC_GET_FLAG_VALUE(WeightOtherInstruction)) + (m_extendedMath * IGC_GET_FLAG_VALUE(WeightExtendedMath));
                // One value for this constant is interesting
                // Get the ConstantAddress from LoadInst and log it in interesting constants
                if (I.getType()->isIntegerTy())
                {
                    constValue = 1;
                    addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, constValue, stats);
                }
                else if (I.getType()->isFloatTy())
                {
                    uint32_t val;
                    float floatValue = 1.0;
                    memcpy_s(&val, sizeof(uint32_t), &floatValue, sizeof(float));
                    constValue = val;
                    addInterestingConstant(I.getType(), bufIdOrGRFOffset, eltId, size_in_bytes, false, constValue, stats);
                }
            }
        }
    }
}

bool SConstantAddrValue_LessThanOp(const SConstantAddrValue& left, const SConstantAddrValue& right)
{
    if (left.ca.bufId < right.ca.bufId)
    {
        return true;
    }
    else if (left.ca.bufId == right.ca.bufId)
    {
        if (left.ca.eltId < right.ca.eltId)
        {
            return true;
        }
        else if (left.ca.eltId == right.ca.eltId)
        {
            if (left.anyValue < right.anyValue)
            {
                return true;
            }
        }
    }
    return false;
}

template<typename ContextT>
void FindInterestingConstants::copyInterestingConstants(ContextT* pShaderCtx)
{
    pShaderCtx->programOutput.m_pInterestingConstants.clear();
    for (auto I = m_InterestingConstants.begin(), E = m_InterestingConstants.end(); I != E; I++)
    {
        if (I->second.size())
        {
            std::sort(I->second.begin(), I->second.end(), SConstantAddrValue_LessThanOp);
            for (unsigned i = 0; i < I->second.size(); i++)
            {
                pShaderCtx->programOutput.m_pInterestingConstants.push_back(I->second[i]);
            }
        }
    }
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
    m_LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    visit(F);
    return false;
}
