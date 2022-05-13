/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Pass inserting a message to retire the stackID at the end of all raygen
/// shaders. This needs to be done before creating the continuation shaders
///
//===----------------------------------------------------------------------===//

#include "RayTracingInterface.h"
#include "RTBuilder.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"

using namespace IGC;
using namespace llvm;

class RetireStackIDPass : public FunctionPass
{
public:
    RetireStackIDPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override;
    llvm::StringRef getPassName() const override
    {
        return "RetireStackIDPass";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }
    static char ID;
};

char RetireStackIDPass::ID = 0;

bool RetireStackIDPass::runOnFunction(Function &F)
{
    auto* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = pCtx->getModuleMetaData();
    auto functionMD = modMD->FuncMD.find(&F);
    if(functionMD == modMD->FuncMD.end())
    {
        return false;
    }
    // Only retire stack IDs when exiting raygen shaders
    if(functionMD->second.functionType != FunctionTypeMD::KernelFunction)
    {
        return false;
    }

    for (auto &BB : F)
    {
        ReturnInst *RI = dyn_cast<ReturnInst>(BB.getTerminator());
        if (!RI)
            continue;

        RTBuilder builder(RI, *pCtx);
        builder.CreateStackIDRelease();
    }

    return true;
}
namespace IGC
{
    Pass* CreateStackIDRetirement()
    {
        return new RetireStackIDPass();
    }
}
