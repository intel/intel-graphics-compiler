/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

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
    bool ConvertToSampleMlod(SampleIntrinsic* inst);
    bool ConvertLdToLdl(SamplerLoadIntrinsic* inst);
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
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4Bptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_gather4Lptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleLptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBCptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBCMlodptr ||
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
                            fmul_src1 = builder.CreateFPExt(fmulInstr->getOperand(1), builder.getFloatTy());

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
        // gather4_l     lod     u       v    r    ai    --
        // gather4_b     bias    u       v    r    ai    --

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

// Converts sample/sample_c to sample_mlod/sample_c_mlod if the mlod parameter
// is present.
bool SamplerPerfOptPass::ConvertToSampleMlod(SampleIntrinsic* inst)
{
    IGC_ASSERT(inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleptr ||
        inst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleCptr);

    // We cannot convert instructions with non-const offsets because there is no po versions of
    // mlod instrinsics.
    bool hasNonConstOffset = false;
    for (uint i = 0; i < 3; ++i)
    {
        hasNonConstOffset |= !isa<Constant>(inst->getImmediateOffsetsValue(i));
    }

    if (hasNonConstOffset)
    {
        return false;
    }

    Value* mlod = inst->getOperand(inst->hasRef() ? 5 : 4);
    if (isa<Constant>(mlod) &&
        cast<Constant>(mlod)->isNullValue())
    {
        return false;
    }

    GenISAIntrinsic::ID id = inst->hasRef() ?
        GenISAIntrinsic::GenISA_sampleCMlodptr : GenISAIntrinsic::GenISA_sampleMlodptr;
    llvm::Type* types[] = {
        inst->getType(),
        inst->getOperand(0)->getType(),
        inst->getPairedTextureValue()->getType(),
        inst->getTextureValue()->getType(),
        inst->getSamplerValue()->getType()
    };
    Function* newFunc = GenISAIntrinsic::getDeclaration(
        inst->getModule(),
        id,
        types);
    IGC_ASSERT(newFunc->getType() == inst->getCalledFunction()->getType());
    inst->setCalledFunction(newFunc);
    // sample        U    V     R   Ai  MLOD
    // sample_mlod   MLOD U     V   R   AI
    // sample_c      Ref  U     V   R   Ai   MLOD
    // sample_c_mlod MLOD Ref   U   V   R    AI
    // Move mlod before coordinates or the reference value:
    uint operandIndex = 0;
    Value* ref = inst->hasRef() ? inst->getOperand(operandIndex++) : nullptr;
    Value* u = inst->getOperand(operandIndex++);
    Value* v = inst->getOperand(operandIndex++);
    Value* r = inst->getOperand(operandIndex++);
    Value* ai = inst->getOperand(operandIndex++);
    operandIndex = 0;
    inst->setOperand(operandIndex++, mlod);
    if (inst->hasRef())
    {
        inst->setOperand(operandIndex++, ref);
    }
    inst->setOperand(operandIndex++, u);
    inst->setOperand(operandIndex++, v);
    inst->setOperand(operandIndex++, r);
    inst->setOperand(operandIndex++, ai);
    return true;
}

// Converts LD to LD_L instruction (to match LSC parameter order)
bool SamplerPerfOptPass::ConvertLdToLdl(SamplerLoadIntrinsic* inst)
{
    IGC_ASSERT(inst->getIntrinsicID() == GenISAIntrinsic::GenISA_ldptr);

    GenISAIntrinsic::ID id = GenISAIntrinsic::GenISA_ldlptr;
    llvm::Type* types[] = {
        inst->getType(),
        inst->getPairedTextureValue()->getType(),
        inst->getTextureValue()->getType(),
        inst->getOperand(0)->getType()
    };
    Function* newFunc = GenISAIntrinsic::getDeclaration(
        inst->getModule(),
        id,
        types);
    IGC_ASSERT(newFunc->getType() == inst->getCalledFunction()->getType());
    inst->setCalledFunction(newFunc);
    // LD     U   V   LOD R
    // LD_L   U   V   R   LOD
    // move R before LOD
    Value* lod = inst->getOperand(2);
    Value* r   = inst->getOperand(3);
    inst->setOperand(2, r);
    inst->setOperand(3, lod);
    return true;
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
                    // sample_b           bias    u       v    r    ai    mlod
                    // sample_b_c_mlod    ref     bias    u    v    r     ai    mlod
                    // For sample_b* the mlod parameter must be shifted left as
                    // 32bit versions of messages no longer have the AI param:
                    if (!sampleInst->getOperand(0)->getType()->isHalfTy() &&
                        ctx->platform.supportAIParameterCombiningWithLODBiasEnabled() &&
                        (sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBptr ||
                         sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBCMlodptr))
                    {
                        uint mlodOffset = sampleInst->hasRef() ? 6 : 5;
                        uint aiOffset = sampleInst->hasRef() ? 5 : 4;
                        Value* mlod = sampleInst->getOperand(mlodOffset);
                        sampleInst->setOperand(aiOffset, mlod);
                        if (!(ctx->platform.getWATable().Wa_14014595444 &&
                              sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleBptr &&
                              IGC_IS_FLAG_ENABLED(EnableSampleBMLODWA)))
                        {
                            Value* zero = ConstantFP::getNullValue(mlod->getType());
                            sampleInst->setOperand(mlodOffset, zero);
                        }
                    }
                    // sample/sample_c to sample_mlod/sample_c_mlod
                    if (ctx->platform.hasSampleMlodMessage() &&
                        IGC_IS_FLAG_ENABLED(EnablePromotionToSampleMlod) &&
                        (sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleptr ||
                         sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleCptr))
                    {
                        changed |= ConvertToSampleMlod(sampleInst);
                    }
                    // Insert here the next sampler specific performance optimization.
                }
                if (SamplerGatherIntrinsic* gatherInst = dyn_cast<SamplerGatherIntrinsic>(II))
                {
                    if (ctx->platform.isCoreChildOf(IGFX_XE2_HPG_CORE) && isSamplingFromCubeSurface(gatherInst))
                    {
                        IGC_ASSERT_MESSAGE(!ctx->platform.WaCubeHFPrecisionBug(),
                            "This WA should be absent on this platform.");
                        if (ctx->platform.supportAIParameterCombiningWithLODBiasEnabled())
                        {
                            changed = DoAIParameterCombiningWithLODBias(gatherInst) ? true : changed;
                        }
                    }
                }
                if (SamplerLoadIntrinsic* loadInst = dyn_cast<SamplerLoadIntrinsic>(II))
                {
                    // EnableLscSamplerRouting key is true (default)
                    // DisableLscSamplerRouting is from UMD AIL to turn off per shader
                    if (ctx->platform.hasLSCSamplerRouting() &&
                        IGC_IS_FLAG_ENABLED(EnableLscSamplerRouting) &&
                        ctx->m_DriverInfo.supportLscSamplerRouting() &&
                        !ctx->getModuleMetaData()->compOpt.DisableLscSamplerRouting &&
                        loadInst->getIntrinsicID() == GenISAIntrinsic::GenISA_ldptr)
                    {
                        changed = ConvertLdToLdl(loadInst);
                    }
                }
            }
        }
    }
    return changed;
}
