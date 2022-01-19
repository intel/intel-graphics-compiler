/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMUtils.h"
#include "common/IGCIRBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

//#define DEBUG_BLENDTODISCARD

namespace {

    /**
     * Check sampler result and do early out using discard based on blend state.
     */
    class BlendToDiscard : public FunctionPass
    {
    public:
        static char ID;

        BlendToDiscard() : FunctionPass(ID)
        {
            initializeBlendToDiscardPass(*PassRegistry::getPassRegistry());
        }

        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "BlendToDiscard";
        }

        bool runOnFunction(Function& F) override;

    private:
        CodeGenContext* m_cgCtx;
        ModuleMetaData* m_modMD;
        Module* m_module;

        bool discardOnAlpha(GenIntrinsicInst* out);

        // Blend to discard opt for multiple render target writes
        bool blendToDiscardMRT(GenIntrinsicInst** outInst, unsigned nOutInsts);

        Instruction* isFMul(Value* v)
        {
            Instruction* inst = dyn_cast<Instruction>(v);
            if (inst && inst->getOpcode() == Instruction::FMul)
            {
                return inst;
            }
            return nullptr;
        }

        void createDiscard(Instruction* extract, bool check0);
    };

} // namespace

char BlendToDiscard::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(BlendToDiscard,
    "BlendToDiscard", "BlendToDiscard", false, false)
    IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
    IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper);
IGC_INITIALIZE_PASS_END(BlendToDiscard,
    "BlendToDiscard", "BlendToDiscard", false, false)

    FunctionPass* IGC::createBlendToDiscardPass()
{
    return new BlendToDiscard();
}

bool BlendToDiscard::runOnFunction(Function& F)
{
    const int MAX_NUM_OUTPUTS = 4;
    const int MAX_NUM_OUTPUT_VALUES = 6;

    m_cgCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (!IGC_IS_FLAG_ENABLED(EnableBlendToDiscard) || !m_cgCtx->platform.enableBlendToDiscardAndFill())
    {
        return false;
    }


    // Skip non-kernel function.
    IGCMD::MetaDataUtils* mdu = nullptr;
    mdu = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = mdu->findFunctionsInfoItem(&F);
    if (FII == mdu->end_FunctionsInfo())
        return false;

    m_modMD = m_cgCtx->getModuleMetaData();
    m_module = F.getParent();

    IGC_ASSERT(m_cgCtx->type == ShaderType::PIXEL_SHADER);

    std::vector<int>& blendOpt = m_modMD->psInfo.blendOptimizationMode;
    if (!blendOpt.size() || m_cgCtx->m_instrTypes.hasDiscard)
    {
        return false;
    }

    smallvector<GenIntrinsicInst*, 8> outInst(MAX_NUM_OUTPUTS);
    unsigned nOutInsts = 0, nOutValues = 0;

    for (auto& BI : F)
    {
        for (auto& II : BI)
        {
            GenIntrinsicInst* out = dyn_cast<GenIntrinsicInst>(
                &II, GenISAIntrinsic::GenISA_OUTPUT);

            if (out)
            {
                IGC_ASSERT(isa<ConstantInt>(out->getOperand(4)));
                IGC_ASSERT(isa<ConstantInt>(out->getOperand(5)));

                ShaderOutputType oType = static_cast<ShaderOutputType>(
                    out->getImm64Operand(4));
                unsigned rtIdx = static_cast<unsigned>(
                    out->getImm64Operand(5));

                if (rtIdx >= MAX_NUM_OUTPUTS ||
                    blendOpt[rtIdx] == USC::BLEND_OPTIMIZATION_NONE ||
                    blendOpt[rtIdx] == USC::BLEND_OPTIMIZATION_SRC_ALPHA_FILL_ONLY)
                {
                    return false;
                }

                if (oType == SHADER_OUTPUT_TYPE_DEFAULT)
                {
                    outInst[rtIdx] = out;
                    nOutInsts++;

                    if (nOutInsts >= MAX_NUM_OUTPUTS)
                    {
                        return false;
                    }

                    for (unsigned i = 0; i < 4; i++)
                    {
                        if (!isa<UndefValue>(out->getOperand(i)))
                        {
                            nOutValues++;
                        }
                    }

                    if (nOutValues > MAX_NUM_OUTPUT_VALUES)
                    {
                        return false;
                    }
                }
            }
        }
    }

    bool changed = false;
    if (nOutInsts == 1 && outInst[0] != nullptr)
    {
        changed = discardOnAlpha(outInst[0]);
    }
    else
        if (nOutInsts == 2 && outInst[0] != nullptr && outInst[1] != nullptr)
        {
            changed = blendToDiscardMRT(outInst.data(), nOutInsts);
        }

#ifdef DEBUG_BLENDTODISCARD
    if (changed)
    {
        DumpLLVMIR(m_cgCtx, "blendtodiscard");
    }
#endif

    return changed;
}

bool BlendToDiscard::discardOnAlpha(GenIntrinsicInst* out)
{
    Instruction* alpha = isFMul(out->getArgOperand(3));
    bool check0;
    bool changed = false;

    if (m_modMD->psInfo.blendOptimizationMode.size() == 0)
        return false;

    switch (m_modMD->psInfo.blendOptimizationMode[0])
    {
    case USC::BLEND_OPTIMIZATION_SRC_ALPHA:
    case USC::BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY:
        check0 = true;
        break;

    case USC::BLEND_OPTIMIZATION_INV_SRC_ALPHA:
        check0 = false;
        break;

    default:
        return false;
    }

    if (alpha)
    {
        // search back all fmul for sample result
        std::vector<Instruction*> opnds;
        SmallPtrSet<Value*, 32> visited;
        unsigned li = 0;

        opnds.push_back(alpha);
        visited.insert(alpha);

        while (li < opnds.size())
        {
            Instruction* inst = opnds[li];
            li++;

            for (unsigned i = 0; i < inst->getNumOperands(); i++)
            {
                Value* v = inst->getOperand(i);

                if (visited.count(v))
                {
                    continue;
                }

                if (ExtractElementInst * extract = dyn_cast<ExtractElementInst>(v))
                {
                    GenIntrinsicInst* intrin = dyn_cast<GenIntrinsicInst>(
                        extract->getVectorOperand());
                    if (intrin && isSampleInstruction(intrin))
                    {
                        createDiscard(extract, check0);
                        m_modMD->psInfo.forceEarlyZ = true;
                        changed = true;
                        return changed;
                    }
                }

                Instruction* fmul = isFMul(v);
                if (fmul && !visited.count(fmul))
                {
                    opnds.push_back(fmul);
                }

                visited.insert(v);
            }
        }
    }

    return changed;
}

void BlendToDiscard::createDiscard(Instruction* extract, bool check0)
{
    if (extract != nullptr)
    {
        Function* fDiscard = GenISAIntrinsic::getDeclaration(m_module,
            GenISAIntrinsic::GenISA_discard);

        IRBuilder<> irb(extract->getNextNode());
        Value* cond;
        if (check0)
        {
            // check against 0
            cond = irb.CreateFCmpOEQ(extract, ConstantFP::get(extract->getType(), 0.0));
        }
        else
        {
            // check against 1
            cond = irb.CreateFCmpOEQ(extract, ConstantFP::get(extract->getType(), 1.0));
        }
        irb.CreateCall(fDiscard, { cond });
    }
}

bool BlendToDiscard::blendToDiscardMRT(
    GenIntrinsicInst** outInst, unsigned nOutInsts)
{
    IGCIRBuilder<> irb(outInst[0]);
    std::vector<int>& blendOpt = m_modMD->psInfo.blendOptimizationMode;

    Value* discardCond = nullptr;
    for (unsigned i = 0; i < 2; i++)
    {
        Value* colors[4] = { nullptr };
        unsigned nValues = 0, nColors = 0;

        for (unsigned j = 0; j < 4; j++)
        {
            if (!isa<UndefValue>(outInst[i]->getOperand(j)))
            {
                colors[nValues] = outInst[i]->getOperand(j);
                nValues++;
                if (j != 3)
                {
                    nColors++;
                }
            }
        }

        Value* cond = nullptr;
        Value* alpha = outInst[i]->getOperand(3);
        Value* f0 = ConstantFP::get(alpha->getType(), 0.0);
        Value* f1 = ConstantFP::get(alpha->getType(), 1.0);
        switch (blendOpt[i])
        {
        case USC::BLEND_OPTIMIZATION_SRC_ALPHA:
        case USC::BLEND_OPTIMIZATION_SRC_ALPHA_DISCARD_ONLY:
            // discard: src.a == 0
            cond = irb.CreateFCmpOEQ(alpha, f0);
            break;

        case USC::BLEND_OPTIMIZATION_INV_SRC_ALPHA:
            // discard: src.a == 1
            cond = irb.CreateFCmpOEQ(alpha, f1);
            break;

        case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO:
            // discard: src.rgb == 0
            cond = irb.CreateAllValuesAreZeroF(colors, nColors);
            break;

        case USC::BLEND_OPTIMIZATION_SRC_COLOR_ONE:
            // discard if src.rgb == 1
            cond = irb.CreateAllValuesAreOneF(colors, nColors);
            break;

        case USC::BLEND_OPTIMIZATION_SRC_BOTH_ZERO:
            // discard: src.rgba == 0
            cond = irb.CreateAllValuesAreZeroF(colors, nValues);
            break;

        case USC::BLEND_OPTIMIZATION_SRC_BOTH_ONE:
            // discard if src.rgba == 1
            cond = irb.CreateAllValuesAreOneF(colors, nValues);
            break;

        case USC::BLEND_OPTIMIZATION_SRC_ALPHA_OR_COLOR_ZERO:
            // discard: src.a == 0 || src.rgb == 0
            cond = irb.CreateOr(
                irb.CreateFCmpOEQ(alpha, f0),
                irb.CreateAllValuesAreZeroF(colors, nColors));
            break;

        case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_ONE:
            // discard: src.rgb == 0 && src.a == 1
            cond = irb.CreateAnd(
                irb.CreateFCmpOEQ(alpha, f1),
                irb.CreateAllValuesAreZeroF(colors, nColors));
            break;

        case USC::BLEND_OPTIMIZATION_SRC_COLOR_ZERO_ALPHA_IGNORE:
        {
            // Discard: src.rgb == 0 and don't compute src.a
            cond = irb.CreateAllValuesAreZeroF(colors, nColors);
            Value* nAlpha = IGC_IS_FLAG_ENABLED(EnableUndefAlphaOutputAsRed) ?
                outInst[i]->getOperand(0) : f0;

            outInst[i]->setOperand(3, nAlpha);
            break;
        }

        default:
            IGC_ASSERT_MESSAGE(0, "Need to handle more cases");
            break;
        }

        if (discardCond)
        {
            discardCond = irb.CreateAnd(discardCond, cond);
        }
        else
        {
            discardCond = cond;
        }
    }

    Function* fDiscard = GenISAIntrinsic::getDeclaration(m_module,
        GenISAIntrinsic::GenISA_discard);

    irb.CreateCall(fDiscard, { discardCond });

    return true;
}
