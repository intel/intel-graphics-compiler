/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "igc-opencl-printf-analysis"
#define PASS_DESCRIPTION "Analyzes OpenCL printf calls"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(OpenCLPrintfAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(OpenCLPrintfAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char OpenCLPrintfAnalysis::ID = 0;

OpenCLPrintfAnalysis::OpenCLPrintfAnalysis() : ModulePass(ID)
{
    initializeOpenCLPrintfAnalysisPass(*PassRegistry::getPassRegistry());
}

//TODO: move to a common place
const StringRef OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME = "printf";

bool OpenCLPrintfAnalysis::runOnModule(Module& M)
{
    m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    visit(M);
    bool changed = false;
    if (m_hasPrintfs.size())
    {
        for (Function& func : M.getFunctionList())
        {
            if (!func.isDeclaration() &&
                m_hasPrintfs.find(&func) != m_hasPrintfs.end())
            {
                addPrintfBufferArgs(func);
                changed = true;
            }
        }
    }

    // Update LLVM metadata based on IGC MetadataUtils
    if (changed)
        m_pMDUtils->save(M.getContext());

    return m_hasPrintfs.size();
}

void OpenCLPrintfAnalysis::visitCallInst(llvm::CallInst& callInst)
{
    Function* pF = callInst.getParent()->getParent();
    if (!callInst.getCalledFunction() || m_hasPrintfs.find(pF)!=m_hasPrintfs.end())
    {
        return;
    }

    StringRef  funcName = callInst.getCalledFunction()->getName();
    bool hasPrintf = (funcName == OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME);
    if (hasPrintf)
    {
        m_hasPrintfs.insert(pF);
    }
}

void OpenCLPrintfAnalysis::addPrintfBufferArgs(Function& F)
{
    SmallVector<ImplicitArg::ArgType, 1> implicitArgs;
    implicitArgs.push_back(ImplicitArg::PRINTF_BUFFER);
    ImplicitArgs::addImplicitArgs(F, implicitArgs, m_pMDUtils);
}

bool OpenCLPrintfAnalysis::isTopLevelUserPrintf(llvm::Value* V)
{
    // Recursively check the users of the value until reaching the top level
    // user. Note that printf has no users.

    // Base case: return true when the current value has no user and is a call
    // to printf. Otherwise return false.
    if (V->user_empty())
    {
        if (llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(V))
        {
            return call->getCalledFunction()->getName() ==
                OpenCLPrintfAnalysis::OPENCL_PRINTF_FUNCTION_NAME;
        }
        return false;
    }

    // Check users recursively.
    for (auto user : V->users()) {
        if (!isTopLevelUserPrintf(user))
            return false;
    }

    // Return true as every top level user is a printf call.
    return true;
}
