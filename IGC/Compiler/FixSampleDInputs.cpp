/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FixSampleDInputs.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGC
{
    class FixSampleDInputsPass : public FunctionPass
    {
    public:
        FixSampleDInputsPass() :
            FunctionPass(ID)
        { }

        virtual bool runOnFunction(Function& F) override;

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual StringRef getPassName() const override { return "FixSampleDInputs"; }
        static char ID;
    };

#define PASS_FLAG "igc-fix-sampled-inputs"
#define PASS_DESCRIPTION "Fix sample_d inputs"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FixSampleDInputsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(FixSampleDInputsPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

    bool FixSampleDInputsPass::runOnFunction(Function& F)
    {
        bool changed = false;
        Module* M = F.getParent();
        Type* volumeTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_3D_TYPE);
        Type* cubeTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_CUBE_TYPE);
        Type* cubeArrayTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_CUBE_ARRAY_TYPE);
        for (BasicBlock& BB : F)
        {
            for (Instruction& inst : BB)
            {
                SampleIntrinsic* sampleInst = dyn_cast<SampleIntrinsic>(&inst);
                Type* textureType = sampleInst ? sampleInst->getTexturePtrEltTy() : nullptr;
                // 3D/Cube/CubeArray access is emulated in CodeGen, see EmitPass::emulateSampleD()
                if (sampleInst &&
                    (textureType != volumeTextureType && textureType != cubeTextureType && textureType != cubeArrayTextureType))
                {
                    Value* zero = ConstantFP::get(sampleInst->getArgOperand(0)->getType(), 0.0);
                    if (sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleDptr)
                    {
                        // (1) Message format with 3D/Cube/CubeArray support
                        //   sample_d   u   dudx    dudy    v   dvdx    dvdy    r   drdx    drdy    ai  mlod
                        // (2) Message format without 3D/Cube/CubeArray support
                        //   sample_d   u   dudx    dudy    u   dvdx    dvdy    r   mlod

                        // Copy MLOD param to the place expected in the 1D/2D/2DArray message format (2)
                        changed = true;
                        sampleInst->setArgOperand(7, sampleInst->getArgOperand(10));
                        sampleInst->setArgOperand(10, zero);
                    }
                    else if (sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleDCMlodptr)
                    {
                        // Combine the array index and mlod for 2DArray textures
                        // and place it in the mlod_r param for other texture
                        // types just copy mlod over to the mlod_r.

                        // Intrinsic parameters support all dimensions, HW
                        // message supports only 1D, 1DArray, 2D and 2DArray.
                        // Emulation is done in EmitPass::emulateSampleD().

                        // Intrinsic params:
                        //   sample_d_c_mlod    ref u   dudx    dudy    v   dvdx    dvdy    r   drdx    drdy    ai  mlod
                        // Message format
                        //   sample_d_c_mlod    ref u   dudx    dudy    u   dvdx    dvdy    mlod_r
                        uint mlod_rParamIndex = 7;
                        uint mlodParamIndex = 11;
                        Value* mlod_r = sampleInst->getArgOperand(mlodParamIndex); // MLOD
                        Type* type2DArray = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_2D_ARRAY_TYPE);
                        Type* typeTex = sampleInst->getTexturePtrEltTy();
                        if (typeTex == type2DArray)
                        {
                            // combine MLOD and R
                            uint numBits = 12; // number of bits to use to encode AI into MLOD
                            IRBuilder<> builder(sampleInst);
                            mlod_r = CombineSampleOrGather4Params(
                                builder,
                                sampleInst->getArgOperand(mlod_rParamIndex), // R
                                sampleInst->getArgOperand(mlodParamIndex), // MLOD
                                numBits,
                                std::string("R"),
                                std::string("MLOD"));
                        }
                        sampleInst->setArgOperand(mlod_rParamIndex, mlod_r);
                        // clear mlod intrinsic params
                        sampleInst->setArgOperand(mlodParamIndex, zero);
                        changed = true;
                    } else if (sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_samplePODptr)
                    {
                        // Message format for sample_po_d is similar to sample_d format without 3D/Cube/CubeArray support
                        //   sample_po_d   u   dudx    dudy    u   dvdx    dvdy    offuvr_r   mlod

                        // Copy MLOD param to the place expected in sample_po_d
                        changed = true;
                        sampleInst->setArgOperand(7, sampleInst->getArgOperand(9));
                        sampleInst->setArgOperand(9, zero);
                    }

                }
            }
        }
        return changed;
    }

    char FixSampleDInputsPass::ID = 0;

    FunctionPass* createFixSampleDInputsPass()
    {
        return new FixSampleDInputsPass();
    }
}
