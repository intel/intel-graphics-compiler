/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RTBuilder.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

//////////////////////////////////////////////////////////////////////////
//
// Pass replace printBuffer with RayDispatchGlobalData::printfBufferBasePtr
// Note this is different with OCL where implicitArgument printBuffer is to hold this ptr.
// which means, OCL's printBuffer is in EU payload, but RT's one is in memory and needs to be loaded with kernel.
class RayTracingPrintfPostProcess : public FunctionPass, public InstVisitor<RayTracingPrintfPostProcess>
{
public:
    RayTracingPrintfPostProcess();
    bool runOnFunction(Function& F) override;
    StringRef getPassName() const override
    {
        return "RayTracingPrintfPostProcess";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    static char ID;
};

#define PASS_FLAG "raytracing-printf-postprocess"
#define PASS_DESCRIPTION "Replace printfBuffer with printfBufferBasePtr"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingPrintfPostProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(RayTracingPrintfPostProcess, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

RayTracingPrintfPostProcess::RayTracingPrintfPostProcess() : FunctionPass(ID) {
    initializeRayTracingPrintfPostProcessPass(*PassRegistry::getPassRegistry());
}

char RayTracingPrintfPostProcess::ID = 0;

bool RayTracingPrintfPostProcess::runOnFunction(Function& F)
{
    RayDispatchShaderContext* ctx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());

    RTBuilder builder(&F.getEntryBlock(), *ctx);

    auto* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        return false;

    ImplicitArgs implicitArgs(F, pMdUtils);
    unsigned numImplicitArgs = implicitArgs.size();
    IGC_ASSERT_MESSAGE(F.arg_size() >= numImplicitArgs, "Function arg size does not match meta data args.");
    llvm::Function::arg_iterator arg = F.arg_begin() + (F.arg_size() - numImplicitArgs);
    for (unsigned i = 0; i < numImplicitArgs; ++i, ++arg) {
        ImplicitArg implictArg = implicitArgs[i];
        if (implictArg.getArgType() == ImplicitArg::PRINTF_BUFFER) {
            if (F.begin()->empty())
            {
                builder.SetInsertPoint(&*F.begin());
            }
            else
            {
                builder.SetInsertPoint(&*F.begin()->begin());
            }
            llvm::Value* printfBufferPtr = builder.getPrintfBufferBasePtr();
            printfBufferPtr = builder.CreateIntToPtr(
                printfBufferPtr, arg->getType(), VALUE_NAME("printfBufferBasePtr"));
            arg->replaceAllUsesWith(printfBufferPtr);
            return true;
        }
    }

    return false;
}
namespace IGC
{
    Pass* createRayTracingPrintfPostProcessPass()
    {
        return new RayTracingPrintfPostProcess();
    }
}
