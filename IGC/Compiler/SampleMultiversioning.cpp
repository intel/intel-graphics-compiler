/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SampleMultiversioning.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-sample-multiversioning"
#define PASS_DESCRIPTION "sample multiversioning"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SampleMultiversioning, PASS_FLAG, PASS_DESCRIPTION,
    PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_END(SampleMultiversioning, PASS_FLAG, PASS_DESCRIPTION,
        PASS_CFG_ONLY, PASS_ANALYSIS)

    char SampleMultiversioning::ID = 0;

#define DEBUG_TYPE "SampleMultiversioning"

SampleMultiversioning::SampleMultiversioning(CodeGenContext* pContext)
    : FunctionPass(ID), pContext(pContext)
{
    initializeSampleMultiversioningPass(*PassRegistry::getPassRegistry());
}

SampleMultiversioning::SampleMultiversioning() : FunctionPass(ID), pContext(nullptr)
{
    initializeSampleMultiversioningPass(*PassRegistry::getPassRegistry());
}

bool SampleMultiversioning::isOnlyExtractedAfterSample(Value* SampleInst, SmallVector<Instruction*, 4> & ExtrVals)
{
    SmallVector<Instruction*, 4> TmpExtrVals;
    for (auto* Use : SampleInst->users())
    {
        if (auto * EI = dyn_cast<ExtractElementInst>(Use))
        {
            TmpExtrVals.push_back(EI);
        }
        else
        {
            return false;
        }
    }
    for (auto val : TmpExtrVals)
    {
        ExtrVals.push_back(val);
    }
    return true;
}

bool SampleMultiversioning::isOnlyMultiplied(Instruction* Sample, Instruction* Val, SmallSet<Instruction*, 4> & MulVals)
{
    SmallSet<Instruction*, 4> TmpMulVals;
    for (auto* Use2 : Val->users())
    {
        if (auto * BOP = dyn_cast<BinaryOperator>(Use2))
        {
            if (BOP->getOpcode() == Instruction::FMul)
            {
                Value* Dep = BOP->getOperand(1);
                if (Dep == Val)
                {
                    Dep = BOP->getOperand(0);
                }

                // Multiplication by a constant is not supported
                if (isa<Constant>(Dep))
                {
                    return false;
                }

                // Make sure multiplied value dominates the Sample
                auto DepI = dyn_cast<Instruction>(Dep);
                if (Sample->getParent() != DepI->getParent())
                {
                    if (!DT->dominates(DepI->getParent(), Sample->getParent()))
                    {
                        return false;
                    }
                }

                TmpMulVals.insert(DepI);
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
    for (auto val : TmpMulVals)
    {
        MulVals.insert(val);
    }
    return true;
};

// @TODO
// Eliminate recursion

Instruction* SampleMultiversioning::getPureFunction(Value* Val)
{
    if (auto I = dyn_cast<Instruction>(Val))
    {
        if (isa<UnaryInstruction>(I) || isa<BinaryOperator>(I) ||
            isa<CmpInst>(I) || isa<ExtractElementInst>(I) || isa<CastInst>(I) ||
            isa<PHINode>(I) || IsMathIntrinsic(GetOpCode(I)) ||
            isa<SelectInst>(I))
        {
            return I;
        }
    }
    return nullptr;
};

bool SampleMultiversioning::isOnlyMultipliedAfterSample(Instruction* Val,
    SmallSet<Instruction*, 4> & MulVals)
{
    SmallSet<Instruction*, 4> TmpMulVals;

    SmallVector<Instruction*, 4> ExtrVals;
    if (isOnlyExtractedAfterSample(Val, ExtrVals))
    {
        for (auto ExtrVal : ExtrVals)
        {
            if (!isOnlyMultiplied(Val, ExtrVal, TmpMulVals))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
    for (auto val : TmpMulVals)
    {
        MulVals.insert(val);
    }
    return true;
};

bool SampleMultiversioning::runOnFunction(Function& F)
{
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    struct MulAfterSample
    {
        Instruction* Sample;
        SmallSet<Instruction*, 4> MulVals;
    };
    SmallVector<MulAfterSample, 4> SampleInsts;

    SmallSet<Instruction*, 4> TmpMulVals;
    SmallSet<Instruction*, 4> MulVals;
    for (BasicBlock& BB : F)
    {
        for (Instruction& Inst : BB)
        {
            if (isSampleLoadGather4InfoInstruction(&Inst) ||
                isa<LdRawIntrinsic>(Inst))
            {
                if (isOnlyMultipliedAfterSample(&Inst, TmpMulVals))
                {
                    for (auto Val : TmpMulVals)
                    {
                        MulVals.insert(Val);
                    }
                    SampleInsts.push_back({ &Inst, MulVals });
                    MulVals.clear();
                }
                TmpMulVals.clear();
            }
        }
    }

    if (SampleInsts.size() < 4)
    {
        // @TODO
        // bitcast after sample
        // And instead of Mul
        // Reorder instructions if sample dominates the dependendency
        for (auto& SI : SampleInsts)
        {
            Instruction* Sample = SI.Sample;
            BasicBlock* Parent = Sample->getParent();
            IGC_ASSERT(SI.MulVals.size());

            // Check if some multipliers are redundant or duplicated
            SmallSet<Instruction*, 4> ToRemove;
            for (auto CurrMulVal : SI.MulVals)
            {
                if (Instruction * dep = dyn_cast<Instruction>(CurrMulVal))
                {
                    if (dep->getOpcode() == Instruction::FMul)
                    {
                        SmallVector<Instruction*, 4> ToCheck;
                        ToCheck.push_back(dep);
                        while (!ToCheck.empty())
                        {
                            Instruction* Current = ToCheck.pop_back_val();

                            Instruction* op0 = dyn_cast<Instruction>(Current->getOperand(0));
                            if (op0 && (SI.MulVals.find(op0) != SI.MulVals.end()))
                            {
                                ToRemove.insert(dep);
                                break;
                            }

                            Instruction* op1 = dyn_cast<Instruction>(Current->getOperand(1));
                            if (op1 && (SI.MulVals.find(op1) != SI.MulVals.end()))
                            {
                                ToRemove.insert(dep);
                                break;
                            }

                            if (op0 && op0->getOpcode() == Instruction::FMul)
                            {
                                ToCheck.push_back(op0);
                            }

                            if (op1 && op1->getOpcode() == Instruction::FMul)
                            {
                                ToCheck.push_back(op1);
                            }
                        }
                    }
                }
            }

            // Erase redundant multipliers
            for (auto remInsn : ToRemove)
            {
                SI.MulVals.erase(remInsn);
            }

            bool skipOpt = false;
            // Consider instructions after sample blocking optimization for hoisting
            SmallVector<Instruction*, 4> toCheckUses;
            SmallSet<Instruction*, 4> toHoist;
            for (auto val : SI.MulVals)
            {
                if (DT->dominates(SI.Sample, val))
                {
                    if (IGC_IS_FLAG_DISABLED(EnableSMRescheduling))
                        skipOpt = true;
                    toHoist.insert(val);
                    toCheckUses.push_back(val);
                }
            }

           // Consider hoisting some instruction after the sample if they block opt.
            while (toCheckUses.size() > 0)
            {
                Instruction* checkVal = toCheckUses.pop_back_val();
                for (auto op = checkVal->op_begin(); op != checkVal->op_end(); ++op)
                {
                    Value* val = op->get();
                    Instruction* val_insn = dyn_cast<Instruction>(val);
                    if (val_insn)
                    {
                        if ((val_insn->getParent() != Parent) || DT->dominates(val_insn, SI.Sample))
                        {
                            continue;
                        }
                        else
                        {
                            for (const auto &tempSI : SampleInsts)
                            {
                                // Corner case where two samplers can be branched for one Mul.
                                if (val_insn == tempSI.Sample)
                                    skipOpt = true;
                            }
                            toHoist.insert(val_insn);
                            toCheckUses.push_back(val_insn);
                        }
                    }
                }
            }

            if (skipOpt)
                continue;

            int hoistSize = toHoist.size();
            Instruction * iterInsn = SI.Sample->getNextNode();

            while (hoistSize > 0)
            {
                Instruction* tempInsn = iterInsn;
                iterInsn = iterInsn->getNextNode();
                if (toHoist.find(tempInsn) != toHoist.end())
                {
                    tempInsn->moveBefore(SI.Sample);
                    hoistSize--;
                }
            }

            BasicBlock* BB1 = Parent->splitBasicBlock(Sample);
            Parent->getTerminator()->eraseFromParent();
            BasicBlock* BB2 =
                Sample->getParent()->splitBasicBlock(Sample->getNextNode());

            PHINode* Phi;
            IRBuilder<> PHIBuilder(BB2);
            PHIBuilder.SetInsertPoint(&*BB2->begin());
            Phi = PHIBuilder.CreatePHI(Sample->getType(), 2);
            Sample->replaceAllUsesWith(Phi);

            IRBuilder<> AndBuilder(Parent);

            Value* PrevCmp = nullptr;
            Value* And = nullptr;
            for (auto Dep : SI.MulVals)
            {
                auto Cmp = AndBuilder.CreateFCmp(
                    CmpInst::Predicate::FCMP_OEQ, Dep,
                    ConstantFP::get(F.getContext(), APFloat(0.0f)));
                if (PrevCmp)
                {
                    And = AndBuilder.CreateAnd(PrevCmp, Cmp);
                }
                PrevCmp = Cmp;
            }

            if (!And)
            {
                And = PrevCmp;
            }
            AndBuilder.CreateCondBr(And, BB2, Sample->getParent());

            Phi->addIncoming(Sample, BB1);
            Phi->addIncoming(ConstantVector::get({
                               ConstantFP::get(F.getContext(), APFloat(0.0f)),
                               ConstantFP::get(F.getContext(), APFloat(0.0f)),
                               ConstantFP::get(F.getContext(), APFloat(0.0f)),
                               ConstantFP::get(F.getContext(), APFloat(0.0f)),
                }),
                Parent);
            DT->recalculate(F);
        }
        pContext->m_instrTypes.hasMultipleBB = true;
        return true;
    }
    return false;
}
