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

#include "SampleMultiversioning.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"
#include "common/secure_mem.h"

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

bool SampleMultiversioning::isOnlyMultiplied(Instruction* Sample, Instruction* Val, SmallSet<Value*, 4> & MulVals)
{
    SmallSet<Value*, 4> TmpMulVals;
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
                else
                {
                    BasicBlock::iterator II(DepI);
                    BasicBlock::iterator IE = DepI->getParent()->end();
                    for (++II; II != IE; ++II)
                    {
                        if ((&*II) == Sample)
                        {
                            break;
                        }
                    }
                    if (II == IE)
                    {
                        return false;
                    }
                }

                TmpMulVals.insert(Dep);
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
    SmallSet<Value*, 4> & MulVals)
{
    SmallSet<Value*, 4> TmpMulVals;

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

bool SampleMultiversioning::runOnFunction(Function& F) {
    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    struct MulAfterSample {
        Instruction* Sample;
        SmallVector<Value*, 4> MulVals;
    };
    SmallVector<MulAfterSample, 4> SampleInsts;

    SmallSet<Value*, 4> TmpMulVals;
    SmallVector<Value*, 4> MulVals;
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
                        MulVals.push_back(Val);
                    }
                    SampleInsts.push_back({ &Inst, MulVals });
                    MulVals.clear();
                }
                TmpMulVals.clear();
            }
        }
    }

    if (SampleInsts.size())
    {
        // @TODO
        // bitcast after sample
        // And instead of Mul
        // Reorder instructions if sample dominates the dependendency
        for (auto& SI : SampleInsts)
        {
            Instruction* Sample = SI.Sample;
            BasicBlock* Parent = Sample->getParent();
            assert(SI.MulVals.size());

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
            for (auto& Dep : SI.MulVals)
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
        }
        return true;
    }
    return false;
}
