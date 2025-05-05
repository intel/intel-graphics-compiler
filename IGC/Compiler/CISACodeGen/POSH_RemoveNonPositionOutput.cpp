/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "common/LLVMWarningsPop.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CISACodeGen/helper.h"


using namespace IGC;
using namespace llvm;

class RemoveNonPositionOutput : public llvm::FunctionPass
{
public:
    static char ID;

    RemoveNonPositionOutput();

    ~RemoveNonPositionOutput() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
    }

    virtual bool runOnFunction(llvm::Function& F) override;

    virtual llvm::StringRef getPassName() const override
    {
        return "remove non-position output in vertex shader";
    }
};

llvm::FunctionPass* createRemoveNonPositionOutputPass()
{
    return new RemoveNonPositionOutput();
}

// Register pass to igc-opt
#define PASS_FLAG_POSH "igc-remove-nonposition-output"
#define PASS_DESCRIPTION_POSH "Custom Pass for Position-Only Shader"
#define PASS_CFG_ONLY_POSH false
#define PASS_ANALYSIS_POSH false
IGC_INITIALIZE_PASS_BEGIN(RemoveNonPositionOutput, PASS_FLAG_POSH, PASS_DESCRIPTION_POSH, PASS_CFG_ONLY_POSH, PASS_ANALYSIS_POSH)
IGC_INITIALIZE_PASS_END(RemoveNonPositionOutput, PASS_FLAG_POSH, PASS_DESCRIPTION_POSH, PASS_CFG_ONLY_POSH, PASS_ANALYSIS_POSH)

char RemoveNonPositionOutput::ID = 0;

RemoveNonPositionOutput::RemoveNonPositionOutput() : FunctionPass(ID)
{
    initializeRemoveNonPositionOutputPass(*PassRegistry::getPassRegistry());
}

bool RemoveNonPositionOutput::runOnFunction(Function& F)
{
    // Initialize the worklist to all of the instructions ready to process...
    SmallVector<Instruction*, 10> instructionToRemove;
    for (inst_iterator II = inst_begin(F), E = inst_end(F); II != E; ++II)
    {
        if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(&*II))
        {
            if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT)
            {
                const ShaderOutputType usage = static_cast<ShaderOutputType>(
                    llvm::cast<llvm::ConstantInt>(inst->getOperand(4))->getZExtValue());
                if (usage != SHADER_OUTPUT_TYPE_POSITION &&
                    usage != SHADER_OUTPUT_TYPE_POINTWIDTH &&
                    usage != SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX &&
                    usage != SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO &&
                    usage != SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI)
                {
                    instructionToRemove.push_back(inst);
                }
            }
        }
    }
    bool changed = false;
    uint num = instructionToRemove.size();
    for (uint i = 0; i < num; ++i)
    {
        instructionToRemove[i]->eraseFromParent();
        changed = true;
    }
    return changed;
}
