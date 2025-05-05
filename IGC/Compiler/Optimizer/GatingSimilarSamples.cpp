/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GatingSimilarSamples.hpp"
#include "common/IGCIRBuilder.h"
#include "common/igc_regkeys.hpp"
#include "IGC/LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvmWrapper/IR/InstrTypes.h>
#include <llvmWrapper/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

//This code must check that all the similar sample inst results are divided by the same value (= 1+loop trip count)
//And must also check that the motion(first) sample inst result is also divided by the same value (=1+loop trip count)
static bool samplesAveragedEqually(const std::vector<Instruction*>& similarSampleInsts)
{
    unsigned similarToTexelSampleInstsCount = similarSampleInsts.size();
    unsigned totalSimilarSamples = similarToTexelSampleInstsCount + 1; //texel(sample2) + similar to texel(sample3,4,5)
    const float cmpAveragingFactor = (float)1.0 / (float(totalSimilarSamples));
    for (auto sampleInst : similarSampleInsts)
    {
        Instruction* inst = sampleInst->getNextNonDebugInstruction();
        std::set<Value*> texels; //for storing texel_x, texel_y, texel_z of this sampleInst

        for (int i = 0; i < 3; i++)
        {
            if (inst->getOpcode() == Instruction::ExtractElement)
            {
                texels.insert(inst);
                inst = inst->getNextNonDebugInstruction();
            }
            else
            {
                return false; //Sample->followed by 3 EE == this  pattern is not matching
            }
        }

        for (int i = 0; i < 3; i++)
        {
            if (inst->getOpcode() == Instruction::FMul)
            {//% 29 = fmul fast float %texel_x, 2.500000e-01
                if (!texels.erase(inst->getOperand(0)) &&
                    !texels.erase(inst->getOperand(1)))
                {
                    return false;
                }
                if (ConstantFP * CF = dyn_cast<ConstantFP>(inst->getOperand(1)))
                {
                    if (!CF->getType()->isFloatTy() || CF->getValueAPF().convertToFloat() != cmpAveragingFactor)
                        return false;
                }
                else if (ConstantFP * CF = dyn_cast<ConstantFP>(inst->getOperand(0)))
                {
                    if (!CF->getType()->isFloatTy() || CF->getValueAPF().convertToFloat() != cmpAveragingFactor)
                        return false;
                }
                else
                {
                    return false; //texel x/y/z not multiplied by const avg factor
                }
            }
            else
            {
                return false; //3 EE -> followed by 3 FMuls == this  pattern is not matching
            }
            inst = inst->getNextNonDebugInstruction();
        }
        IGC_ASSERT_MESSAGE(texels.size() == 0, " All texels.x/y/z were not multiplied by same float");
        texels.clear();
    }
    return true;
}

// detect the pattern where all sample results are added together then
// multiply by constant
static bool
detectSampleAveragePattern2(const std::vector<Instruction*>& sampleInsts, Instruction* texSample)
{
    unsigned nSampleInsts = sampleInsts.size();
    float averagingFactor = float(1.0 / (nSampleInsts + 1));

    Instruction* base[3];
    for (auto* UI : texSample->users())
    {
        ExtractElementInst* ui = dyn_cast<ExtractElementInst>(UI);
        if (ui == nullptr)
        {
            return false;
        }
        ConstantInt* ci = dyn_cast<ConstantInt>(ui->getIndexOperand());
        if (ci == nullptr)
        {
            return false;
        }
        unsigned idx = static_cast<unsigned>(ci->getZExtValue());
        if (idx <= 2)
        {
            base[idx] = ui;
        }
    }

    Instruction* rgb[3] = { nullptr };

    for (unsigned i = 0; i < nSampleInsts; i++)
    {
        Instruction* inst = sampleInsts[i]->getNextNonDebugInstruction();
        for (unsigned j = 0; j < 3; j++)
        {
            ExtractElementInst* ei = dyn_cast<ExtractElementInst>(inst);
            if (!ei)
            {
                return false;
            }
            ConstantInt* ci = dyn_cast<ConstantInt>(ei->getIndexOperand());
            if (!ci)
            {
                return false;
            }
            unsigned idx = static_cast<unsigned>(ci->getZExtValue());
            if (idx > 2)
            {
                return false;
            }
            if (ei->hasNUsesOrMore(2))
            {
                return false;
            }

            if (i == 0)
            {
                rgb[idx] = ei;
            }
            else
            {
                Instruction* fadd = dyn_cast<Instruction>(*ei->users().begin());
                if (fadd == nullptr || fadd->getOpcode() != Instruction::FAdd ||
                    fadd->hasNUsesOrMore(2))
                {
                    return false;
                }
                if (fadd->getOperand(0) != rgb[idx] &&
                    fadd->getOperand(1) != rgb[idx])
                {
                    return false;
                }
                rgb[idx] = fadd;
            }
            inst = inst->getNextNonDebugInstruction();
        }
        if (isa<ExtractElementInst>(inst))
        {
            return false;
        }
    }

    for (unsigned i = 0; i < 3; i++)
    {
        Instruction* fadd = dyn_cast<Instruction>(*rgb[i]->users().begin());
        if (fadd == nullptr || fadd->getOpcode() != Instruction::FAdd ||
            fadd->hasNUsesOrMore(2))
        {
            return false;
        }
        if (fadd->getOperand(0) != base[i] &&
            fadd->getOperand(1) != base[i])
        {
            return false;
        }

        Instruction* fmul = dyn_cast<Instruction>(*fadd->users().begin());
        if (fmul == nullptr || fmul->getOpcode() != Instruction::FMul ||
            fmul->hasNUsesOrMore(2))
        {
            return false;
        }
        ConstantFP* cf;
        if (fmul->getOperand(0) == fadd)
        {
            cf = dyn_cast<ConstantFP>(fmul->getOperand(1));
        }
        else
        {
            cf = dyn_cast<ConstantFP>(fmul->getOperand(0));
        }
        if (cf == nullptr ||
            !cf->getType()->isFloatTy() ||
            cf->getValueAPF().convertToFloat() != averagingFactor)
        {
            return false;
        }
    }
    return true;
}

// Need to match a very specific pattern here
// @llvm.genx.GenISA.sampleptr1 => samples(tex0....) ---> This will be motionSample, sampling from tex0
// @llvm.genx.GenISA.sampleptr2 => samples(tex1....) ---> This will be texelSample, sampling from tex1. We search similar to this
// @llvm.genx.GenISA.sampleptr3 => samples(tex1....)
// @llvm.genx.GenISA.sampleptr4 => samples(tex1....)
// @llvm.genx.GenISA.sampleptr5 => samples(tex1....)
bool GatingSimilarSamples::checkAndSaveSimilarSampleInsts()
{
    for (auto& I : *BB)
    {
        if (SampleIntrinsic * SI = dyn_cast<SampleIntrinsic>(&I))
        {
            if (motionSample == nullptr)
            {
                motionSample = SI;
                continue;
            }
            if (!texelSample)
            {
                texelSample = SI;
                continue;
            }
            if (areSampleInstructionsSimilar(texelSample, SI))
            {
                similarSampleInsts.push_back(SI);
            }
            else
            { //we can't have a different texel sample between 2 matching(similar) texel samples!
                return false;
            }
        }
    }
    if (similarSampleInsts.size() == 0)
        return false;
    return true;
}

bool GatingSimilarSamples::setOrCmpGatingValue(Value*& gatingValueToCmp1, Instruction* mulInst, const Instruction* texelSampleInst)
{
    if (!gatingValueToCmp1)
    {
        //This is the first texel sample inst from the loop after unrolled
        IGC_ASSERT_MESSAGE(texelSampleInst == similarSampleInsts[0], "incorrect inst sequence while extracting the loop gating value");
        gatingValueToCmp1 = mulInst;
        return true;
    }
    else
    {
        if (gatingValueToCmp1 != mulInst->getOperand(0) && gatingValueToCmp1 != mulInst->getOperand(1))
        {
            return false;
        }
    }
    return true;
}

//This function makes sure that all similar sample insts calculate cords such that they use same gating value motion.xy
//Outside the loop, we're looking at this:
//          motion.xy = (motion.xy - 0.5) * vec2(0.0666667, .125);
//          motion.xy *= texel.a;
//Check that inside the loop, we're looking at something like this:
//          vec2 tc = out_texcoord0 - motion.xy * float(i);
//          color += texture2D(texture_unit0, tc).xyz / float(n);
bool GatingSimilarSamples::findAndSetCommonGatingValue()
{
    gatingValue_mul1 = nullptr;
    gatingValue_mul2 = nullptr;

    for (auto& texelSampleInstInLoop : similarSampleInsts)
    {
        Instruction* firstOp = dyn_cast<Instruction>(texelSampleInstInLoop->getOperand(0)); //tc.1
        Instruction* secondOp = dyn_cast<Instruction>(texelSampleInstInLoop->getOperand(1)); //tc.2
        if (!(firstOp && secondOp)) return false;
        if (firstOp->getOpcode() == Instruction::FSub || firstOp->getOpcode() == Instruction::FAdd)
        {//i.e. (texcoord0 (+/-) something)
            Instruction* mayBeMulInst = dyn_cast<Instruction>(firstOp->getOperand(1));
            if (!mayBeMulInst) return false;
            //that "texcoord0 - something" might be "texcoord0 - FMul" OR it might be "tc - (0 - -FMul)"
            if (mayBeMulInst->getOpcode() == Instruction::FMul)
            {//i.e. something is FMul!
                if (!setOrCmpGatingValue(gatingValue_mul1, mayBeMulInst, texelSampleInstInLoop))
                    return false;
            }
            else if (mayBeMulInst->getOpcode() == Instruction::FSub || mayBeMulInst->getOpcode() == Instruction::FAdd)
            {//that means we have this "tc - (0 - -FMul)"
                Instruction* realMulInst = dyn_cast<Instruction>(mayBeMulInst->getOperand(1));
                if (!realMulInst) return false;
                if (ConstantFP * mustBeZero = dyn_cast<ConstantFP>(mayBeMulInst->getOperand(0)))
                {
                    if (!mustBeZero->getType()->isFloatTy() || mustBeZero->getValueAPF().convertToFloat() != 0.0f)
                        return false;
                }
                else
                {
                    return false;
                }
                if (!setOrCmpGatingValue(gatingValue_mul1, realMulInst, texelSampleInstInLoop))
                    return false;
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
        if (secondOp->getOpcode() == Instruction::FSub || secondOp->getOpcode() == Instruction::FAdd)
        {
            //i.e. (out_texcoord0 (+/-) something)
            Instruction* mayBeMulInst = dyn_cast<Instruction>(secondOp->getOperand(1));
            if (!mayBeMulInst) return false;
            //that "tc - something" might be "tc - FMul" OR it might be "tc - (0 - -FMul)"
            if (mayBeMulInst->getOpcode() == Instruction::FMul)
            {//i.e. something is FMul!
                if (!setOrCmpGatingValue(gatingValue_mul2, mayBeMulInst, texelSampleInstInLoop))
                    return false;
            }
            else if (mayBeMulInst->getOpcode() == Instruction::FSub || mayBeMulInst->getOpcode() == Instruction::FAdd)
            {//that means we have this "tc - (0 - -FMul)"
                Instruction* realMulInst = dyn_cast<Instruction>(mayBeMulInst->getOperand(1));
                if (!realMulInst) return false;
                if (ConstantFP * mustBeZero = dyn_cast<ConstantFP>(mayBeMulInst->getOperand(0)))
                {
                    if (!mustBeZero->getType()->isFloatTy() || mustBeZero->getValueAPF().convertToFloat() != 0.0f)
                        return false;
                }
                else
                {
                    return false;
                }
                if (!setOrCmpGatingValue(gatingValue_mul2, realMulInst, texelSampleInstInLoop))
                    return false;
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
    }
    return true; //a common gating value was found and set
}

//check if 2 sample insts sample from the same texture
bool GatingSimilarSamples::areSampleInstructionsSimilar(Instruction* firstSampleInst, Instruction* secondSampleInst)
{
    if (!firstSampleInst || !secondSampleInst) return false;
    IGC_ASSERT(isSampleInstruction(firstSampleInst));
    IGC_ASSERT(isSampleInstruction(secondSampleInst));
    if (firstSampleInst->getNumOperands() != secondSampleInst->getNumOperands())
        return false;

    if (firstSampleInst->getOpcode() != secondSampleInst->getOpcode())
    {
        return false;
    }

    //all operands except the first two operands should be the same
    unsigned int numOperands = firstSampleInst->getNumOperands();
    for (unsigned int i = 2; i < numOperands; i++)
    {
        if (firstSampleInst->getOperand(i) != secondSampleInst->getOperand(i))
            return false;
    }
    return true;
}


//This pass assumes loop unrolling has been performed
bool GatingSimilarSamples::runOnFunction(llvm::Function& F)
{
    BB = nullptr; //opt runs only if single BB in function
    motionSample = nullptr;
    texelSample = nullptr;
    resultInst = nullptr;
    gatingValue_mul1 = nullptr;
    gatingValue_mul2 = nullptr;
    similarSampleInsts.clear();

    if (IGC_GET_FLAG_VALUE(DisableGatingSimilarSamples))
        return false;
    if (F.size() != 1)
        return false;
    BB = &*F.begin();

    if (!checkAndSaveSimilarSampleInsts())
        return false;

    bool pattern1 = samplesAveragedEqually(similarSampleInsts);
    bool pattern2 = detectSampleAveragePattern2(similarSampleInsts, texelSample);
    if (!pattern1 && !pattern2)
        return false;

    //By now we know that all similar sample inst results are divided by the same values and added with equal weights.
    if (!findAndSetCommonGatingValue())
        return false;

    //save the final result inst
    for (BasicBlock::reverse_iterator rItr = BB->rbegin(); rItr != BB->rend(); rItr++)
    {
        if (GenIntrinsicInst * GenI = dyn_cast<GenIntrinsicInst>(&*rItr))
        {
            bool isOutputInstr = GenI->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT;
            if (isOutputInstr)
            {
                resultInst = &*rItr;
                break;
            }
        }
    }
    if (resultInst == nullptr)
        return false;

    //extract original texel.xyz and averaged color.xyz values for creating 3 PHI nodes
    Instruction* texel_x = texelSample->getNextNonDebugInstruction();
    if (!dyn_cast<ExtractElementInst>(texel_x)) return false;
    Instruction* texel_y = texel_x->getNextNonDebugInstruction();
    if (!dyn_cast<ExtractElementInst>(texel_y)) return false;
    Instruction* texel_z = texel_y->getNextNonDebugInstruction();
    if (!dyn_cast<ExtractElementInst>(texel_z)) return false;



    //create a if-then basic block with the gating condition
    IGCIRBuilder<> IRB(F.getContext());
    FastMathFlags FMF;
    FMF.setFast();
    IRB.setFastMathFlags(FMF);
    IRB.SetInsertPoint(similarSampleInsts[0]);
    Value* gatingVal1 = IRB.CreateBitCast(gatingValue_mul1, IRB.getFloatTy());
    Value* cnd1 = IRB.CreateFCmpONE(gatingVal1, ConstantFP::get(IRB.getFloatTy(), 0.0f));
    Value* gatingVal2 = IRB.CreateBitCast(gatingValue_mul2, IRB.getFloatTy());
    Value* cnd2 = IRB.CreateFCmpONE(gatingVal2, ConstantFP::get(IRB.getFloatTy(), 0.0f));
    Value* isGatingValueNotZero = IRB.CreateOr(cnd1, cnd2);
    IGCLLVM::TerminatorInst* thenBlockTerminator = SplitBlockAndInsertIfThen(isGatingValueNotZero, similarSampleInsts[0], false);
    BasicBlock* thenBlock = thenBlockTerminator->getParent();
    if (thenBlockTerminator->getNumSuccessors() != 1)
    {
        return false;
    }


    //move all insts starting from similarSampleInst[0] upto resultInst(non-inluding) into the new then block
    BasicBlock* tailBlock = thenBlockTerminator->getSuccessor(0);
    IGCLLVM::splice(thenBlock, thenBlock->begin(), tailBlock, similarSampleInsts[0]->getIterator(), resultInst->getIterator());

    Value* avg_color_x = resultInst->getOperand(0);
    Value* avg_color_y = resultInst->getOperand(1);
    Value* avg_color_z = resultInst->getOperand(2);

    //Add 3 phi nodes - for x (result op0), y(result op1) and z(result op2).
    IRB.SetInsertPoint(resultInst);
    PHINode* PN1 = IRB.CreatePHI(avg_color_x->getType(), 2);
    PN1->addIncoming(avg_color_x, thenBlock);
    PN1->addIncoming(texel_x, texelSample->getParent());
    resultInst->setOperand(0, PN1);

    PHINode* PN2 = IRB.CreatePHI(avg_color_y->getType(), 2);
    PN2->addIncoming(avg_color_y, thenBlock);
    PN2->addIncoming(texel_y, texelSample->getParent());
    resultInst->setOperand(1, PN2);

    PHINode* PN3 = IRB.CreatePHI(avg_color_z->getType(), 2);
    PN3->addIncoming(avg_color_z, thenBlock);
    PN3->addIncoming(texel_z, texelSample->getParent());
    resultInst->setOperand(2, PN3);

    return true;
}

char IGC::GatingSimilarSamples::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(GatingSimilarSamples, "loop-gating",
    "Loop Gating Optimization", false, false)
    INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_END(GatingSimilarSamples, "loop-gating",
        "Loop Gating Optimization", false, false)

    llvm::FunctionPass* IGC::CreateGatingSimilarSamples()
{
    return new GatingSimilarSamples();
}



