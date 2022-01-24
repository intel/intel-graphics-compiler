/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SamplerPerfOptPass.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/igc_resourceDimTypes.h"

using namespace llvm;
using namespace IGC;

class SamplerPerfOptPass : public FunctionPass
{
public:
    static char ID;

    SamplerPerfOptPass();

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<CodeGenContextWrapper>();
        AU.setPreservesCFG();
    }

    StringRef getPassName() const { return "SamplerPerfOptPass"; }

    bool runOnFunction(Function& F);

private:
    template<typename SampleOrGather4Intrinsic>
    bool isSampleLorBAndIsNotHalfType(SampleOrGather4Intrinsic* inst);
    bool isSampleWithHalfType(SampleIntrinsic* sampleInst);
    template<typename SampleOrGather4Intrinsic>
    bool isSamplingFromCubeSurface(SampleOrGather4Intrinsic* inst);
    bool FixCubeHFPrecisionBug(SampleIntrinsic* sampleInst);
    template<typename SampleOrGather4Intrinsic>
    bool DoAIParameterCombiningWithLODBias(SampleOrGather4Intrinsic* inst);
};

char SamplerPerfOptPass::ID = 0;

#define PASS_FLAG     "igc-SamplerPerfOptPass"
#define PASS_DESC     "IGC runs sampler performance dedicated optimizations"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SamplerPerfOptPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SamplerPerfOptPass, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)


SamplerPerfOptPass::SamplerPerfOptPass()
    :FunctionPass(ID)
{
    initializeSamplerPerfOptPassPass(*PassRegistry::getPassRegistry());
}

namespace IGC
{
    llvm::FunctionPass* createSamplerPerfOptPass()
    {
        return new SamplerPerfOptPass();
    }
}

template<typename SampleOrGather4Intrinsic>
bool SamplerPerfOptPass::isSampleLorBAndIsNotHalfType(SampleOrGather4Intrinsic* inst)
{
    if (
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleLptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBCptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleLCptr)
    {
        for (uint srcOp = 0; srcOp < inst->getNumOperands(); srcOp++)
        {
            // Do not run this optimization for half precision.
            if (inst->getOperand(srcOp)->getType()->isHalfTy())
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool SamplerPerfOptPass::isSampleWithHalfType(SampleIntrinsic* sampleInst)
{
    for (uint srcOp = 0; srcOp < sampleInst->getNumOperands(); srcOp++)
    {
        if (sampleInst->getOperand(srcOp)->getType()->isHalfTy())
        {
            return true;
        }
    }
    return false;
}

template<typename SampleOrGather4Intrinsic>
bool SamplerPerfOptPass::isSamplingFromCubeSurface(SampleOrGather4Intrinsic* inst)
{
    llvm::Value* textureOp = inst->getTextureValue();
    if (textureOp && (textureOp->getType()->getNumContainedTypes() != 0) &&
        ((textureOp->getType()->getContainedType(0) == IGC::GetResourceDimensionType(*inst->getModule(), IGC::DIM_CUBE_ARRAY_TYPE)) ||
         (textureOp->getType()->getContainedType(0) == IGC::GetResourceDimensionType(*inst->getModule(), IGC::DIM_CUBE_TYPE))))
    {
        return true;
    }
    return false;
}

bool SamplerPerfOptPass::FixCubeHFPrecisionBug(SampleIntrinsic* sampleInst)
{
    if (isSampleWithHalfType(sampleInst))
    {
        // Below WA code should be under
        // if (ctx->platform.isAIParameterCombiningWithLODBiasEnabled()) statement.
        //
        // MMIO: Enable-new-message-layout-for-cube-array bit must be set.
        // KM_GEN9_HALF_SLICE_CHICKEN7, KM_XE_HP_ENABLE_NEW_MSG_LAYOUT_FOR_CUBE_ARRAY

        // WA details
        //    The WA must be applied only for : cubes + half-float types
        //
        //    When in float16 cube :
        //    1.    Convert the coordinate half to float
        //    2.    Do the math operation in float, (inv and multiplication) - the part of the cube coords normalization code
        //    3.    Convert back to half-float from float

        IRBuilder<> builder(sampleInst->getContext());
        builder.SetInsertPoint(sampleInst);

        for (unsigned int opId = 0; opId < 3; opId++)
        {
            bool fdivInstrForMaxCoordFound = false;

            if (BinaryOperator * fmulInstr = dyn_cast<BinaryOperator>(sampleInst->getOperand(opId)))
            {
                if (fmulInstr->getOpcode() == llvm::Instruction::BinaryOps::FMul)
                {
                    llvm::Value* fmul_src0 = nullptr;
                    llvm::Value* fmul_src1 = nullptr;
                    if (BinaryOperator* fdivInstr = dyn_cast<BinaryOperator>(fmulInstr->getOperand(0)))
                    {
                        if (fdivInstr->getOpcode() == llvm::Instruction::BinaryOps::FDiv)
                        {
                            llvm::Value* src0 = builder.CreateFPExt(fdivInstr->getOperand(0), builder.getFloatTy());
                            llvm::Value* src1 = builder.CreateFPExt(fdivInstr->getOperand(1), builder.getFloatTy());
                            fmul_src0 = builder.CreateFDiv(src0, src1);
                            fdivInstrForMaxCoordFound = true;
                        }
                    }
                    if (BinaryOperator* fdivInstr = dyn_cast<BinaryOperator>(fmulInstr->getOperand(1)))
                    {
                        if (fdivInstr->getOpcode() == llvm::Instruction::BinaryOps::FDiv)
                        {
                            llvm::Value* src0 = builder.CreateFPExt(fdivInstr->getOperand(0), builder.getFloatTy());
                            llvm::Value* src1 = builder.CreateFPExt(fdivInstr->getOperand(1), builder.getFloatTy());
                            fmul_src1 = builder.CreateFDiv(src0, src1);
                            fdivInstrForMaxCoordFound = true;
                        }
                    }
                    if (fdivInstrForMaxCoordFound)
                    {
                        if (!fmul_src0)
                            fmul_src0 = builder.CreateFPExt(fmulInstr->getOperand(0), builder.getFloatTy());
                        if (!fmul_src1)
                            fmul_src0 = builder.CreateFPExt(fmulInstr->getOperand(1), builder.getFloatTy());

                        llvm::Value* fpFMulRes = builder.CreateFMul(fmul_src0, fmul_src1);
                        llvm::Value* fpTohfOp = builder.CreateFPTrunc(fpFMulRes, builder.getHalfTy());
                        sampleInst->setOperand(opId, fpTohfOp);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

template<typename SampleOrGather4Intrinsic>
bool SamplerPerfOptPass::DoAIParameterCombiningWithLODBias(SampleOrGather4Intrinsic* inst)
{
    if (isSampleLorBAndIsNotHalfType(inst))
    {
        IRBuilder<> builder(inst->getContext());
        builder.SetInsertPoint(inst);

        // intAi = Ftouint rnde(ai)
        // intAiLsb &= 0x1FF
        // (f0) cmp intAi, intAilsb
        // intAiLSb = (f0) Sel intAiLsb, 511
        // lod_Ai =  LOD & 0xFFFFFE00
        // lod_Ai  |= intAiLSb

        // mnemonic      parameters
        //                0       1       2    3    4     5
        // sample_b      bias    u       v    r    ai    --
        // sample_l      lod     u       v    r    ai    --
        // sample_b_c    ref     bias    u    v    r     ai
        // sample_l_c    ref     lod     u    v    r     ai

        uint aiOffset = inst->hasRef() ? 5 : 4;
        uint lodOrBiasOffset = inst->hasRef() ? 1 : 0;
        uint numBits = 9; // number of bits to use to encode AI into BIAS or LOD
        Value* lodAi = CombineSampleOrGather4Params(
            builder,
            inst->getOperand(aiOffset),
            inst->getOperand(lodOrBiasOffset),
            numBits,
            std::string("AI"),
            std::string(inst->hasBias() ? "Bias" : "Lod"));

        // set new lod_ai or bias_ai parameter
        inst->setOperand(lodOrBiasOffset, lodAi);

        // clear ai parameter
        inst->setOperand(aiOffset, ConstantFP::get(builder.getFloatTy(), 0.0));

        return true;
    }
    return false;
}


bool SamplerPerfOptPass::runOnFunction(Function& F)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    bool changed = false;

    if (ctx->platform.isProductChildOf(IGFX_DG2))
    {
        for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
        {
            for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
            {
                if (SampleIntrinsic* sampleInst = dyn_cast<SampleIntrinsic>(II))
                {
                    if (isSamplingFromCubeSurface(sampleInst))
                    {
                        if (ctx->platform.WaCubeHFPrecisionBug())
                        {
                            changed = (FixCubeHFPrecisionBug(sampleInst)) ? true : changed;
                        }
                        if (ctx->platform.supportAIParameterCombiningWithLODBiasEnabled())
                        {
                            changed = DoAIParameterCombiningWithLODBias(sampleInst) ? true : changed;
                        }
                    }
                    // Insert here the next sampler specific performance optimization.
                }
            }
        }
    }
    return changed;
}

