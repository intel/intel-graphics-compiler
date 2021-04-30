/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OCLBIConverter.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-conv-ocl-to-common"
#define PASS_DESCRIPTION "Convert builtin functions from OpenCL to common GenISA"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BuiltinsConverter, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BuiltinsConverter, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BuiltinsConverter::ID = 0;


BuiltinsConverter::BuiltinsConverter(void) : FunctionPass(ID)
{
    initializeBuiltinsConverterPass(*PassRegistry::getPassRegistry());
}

bool BuiltinsConverter::fillIndexMap(Function& F)
{
    ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    FunctionMetaData* funcMD = &modMD->FuncMD[&F];
    ResourceAllocMD* resAllocMD = &funcMD->resAllocMD;
    for (Function::arg_iterator arg = F.arg_begin(), e = F.arg_end(); arg != e; ++arg)
    {
        int argNo = (*arg).getArgNo();
        IGC_ASSERT_MESSAGE(resAllocMD->argAllocMDList.size() > 0, "ArgAllocMDList is empty.");
        ArgAllocMD* argAlloc = &resAllocMD->argAllocMDList[argNo];
        if (argAlloc->type == OtherResourceType)
        {
            // Other resource type has no valid index and is not needed in the map.
            continue;
        }
        m_argIndexMap[&(*arg)] = CImagesBI::ParamInfo(
            argAlloc->indexType,
            (ResourceTypeEnum)argAlloc->type,
            (ResourceExtensionTypeEnum)argAlloc->extensionType);
    }

    // The sampler arguments have already been allocated indices by the ResourceAllocator.
    // So, the first sampler we can allocate here may not be 0, but is the number of
    // already allocated indices.
    m_nextSampler = resAllocMD->samplersNumType;

    return true;
}

void BuiltinsConverter::visitCallInst(llvm::CallInst& CI)
{
    Function* callee = CI.getCalledFunction();
    if (!callee) return;

    bool resolved = m_pResolve->resolveBI(&CI);
    if (resolved)
    {
        CI.eraseFromParent();
    }
}

bool BuiltinsConverter::runOnFunction(Function& F)
{
    m_argIndexMap.clear();
    m_inlineIndexMap.clear();

    // Make sure we are running on a kernel.
    auto ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    ModuleMetaData* modMD = ctx->getModuleMetaData();

    if (ctx->getMetaDataUtils()->findFunctionsInfoItem(&F) == ctx->getMetaDataUtils()->end_FunctionsInfo() ||
        modMD->FuncMD.find(&F) == modMD->FuncMD.end())
    {
        return false;
    }

    if (!fillIndexMap(F))
        return false;

    CBuiltinsResolver resolve(&m_argIndexMap, &m_inlineIndexMap, &m_nextSampler, ctx);
    m_pResolve = &resolve;
    visit(F);
    return true;
}

extern "C" FunctionPass* createBuiltinsConverterPass(void)
{
    return new BuiltinsConverter();
}


