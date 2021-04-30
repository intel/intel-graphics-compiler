/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ComputeShaderLowering.hpp"
#include "IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"
#include "helper.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

class ComputeShaderLowering : public FunctionPass
{
public:
    ComputeShaderLowering() : FunctionPass(ID) {}
    virtual bool runOnFunction(Function& F) override;
    virtual llvm::StringRef getPassName() const override
    {
        return "ComputeShaderLowering";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }
    static char ID;
protected:
    Function* m_function = nullptr;
};

char ComputeShaderLowering::ID = 0;

bool ComputeShaderLowering::runOnFunction(Function& F)
{
    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            if (GenIntrinsicInst * inst = dyn_cast<GenIntrinsicInst>(II))
            {
                if (inst->getIntrinsicID() == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    II = AdjustSystemValueCall(inst)->getIterator();
                }
            }
        }
    }

    return true;
}

namespace IGC {
#define PASS_FLAG "igc-compute-shader-lowering"
#define PASS_DESCRIPTION "This is the compute shader lowering pass "
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
    IGC_INITIALIZE_PASS_BEGIN(ComputeShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_END(ComputeShaderLowering, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        FunctionPass* CreateComputeShaderLowering()
    {
        return new ComputeShaderLowering();
    }
}
