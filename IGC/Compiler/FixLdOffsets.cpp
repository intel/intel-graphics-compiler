/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FixLdOffsets.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"


namespace IGC
{
    class FixLdOffsetsPass : public llvm::FunctionPass
    {
    public:
        FixLdOffsetsPass() :
            llvm::FunctionPass(ID)
        { }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        virtual llvm::StringRef getPassName() const override { return "FixLdOffsets"; }
        static char ID;
    };

    bool FixLdOffsetsPass::runOnFunction(llvm::Function& F)
    {
        bool changed = false;

        // Add offsets to coordinates.
        // Sampler clamps coordinates of ld message and then adds texel offsets.
        // This HW behavior is not what's expected from the API point of view i.e.
        // it's ok to create a message with a negative coordinate value and a positive offset (where coord+offset >= 0).

        for (llvm::BasicBlock& BB : F)
        {
            for (llvm::Instruction& inst : BB)
            {
                if (llvm::SamplerLoadIntrinsic* intrinsic = llvm::dyn_cast<llvm::SamplerLoadIntrinsic>(&inst))
                {
                    if (intrinsic->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_ldptr)
                    {
                        llvm::Value* U = intrinsic->getCoordinateValue(0);
                        llvm::Value* V = intrinsic->getCoordinateValue(1);
                        llvm::Value* R = intrinsic->getCoordinateValue(2);

                        llvm::Value* Uoffset = intrinsic->getOffsetValue(0);
                        llvm::Value* Voffset = intrinsic->getOffsetValue(1);
                        llvm::Value* Roffset = intrinsic->getOffsetValue(2);

                        llvm::IRBuilder<> builder(intrinsic);
                        llvm::Value* UplusOffset = builder.CreateAdd(U, Uoffset);
                        llvm::Value* VplusOffset = builder.CreateAdd(V, Voffset);
                        llvm::Value* RplusOffset = builder.CreateAdd(R, Roffset);

                        intrinsic->setCoordinateValue(0, UplusOffset);
                        intrinsic->setCoordinateValue(1, VplusOffset);
                        intrinsic->setCoordinateValue(2, RplusOffset);

                        intrinsic->setOffsetValue(0, builder.getInt32(0));
                        intrinsic->setOffsetValue(1, builder.getInt32(0));
                        intrinsic->setOffsetValue(2, builder.getInt32(0));

                        changed = true;
                    }
                }
            }
        }

        return changed;
    }

    char FixLdOffsetsPass::ID = 0;

    llvm::FunctionPass* createFixLdOffsetsPass()
    {
        return new FixLdOffsetsPass();
    }
}
