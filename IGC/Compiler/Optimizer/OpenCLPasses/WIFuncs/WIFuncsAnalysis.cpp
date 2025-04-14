/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-wi-func-analysis"
#define PASS_DESCRIPTION "Analyzes work item functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WIFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(WIFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIFuncsAnalysis::ID = 0;

const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_X = "__builtin_IB_get_local_id_x";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_Y = "__builtin_IB_get_local_id_y";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_Z = "__builtin_IB_get_local_id_z";
const llvm::StringRef WIFuncsAnalysis::GET_GROUP_ID = "__builtin_IB_get_group_id";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_THREAD_ID = "__builtin_IB_get_local_thread_id";
const llvm::StringRef WIFuncsAnalysis::GET_GLOBAL_SIZE = "__builtin_IB_get_global_size";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_SIZE = "__builtin_IB_get_local_size";
const llvm::StringRef WIFuncsAnalysis::GET_GLOBAL_OFFSET = "__builtin_IB_get_global_offset";
const llvm::StringRef WIFuncsAnalysis::GET_WORK_DIM = "__builtin_IB_get_work_dim";
const llvm::StringRef WIFuncsAnalysis::GET_NUM_GROUPS = "__builtin_IB_get_num_groups";
const llvm::StringRef WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE = "__builtin_IB_get_enqueued_local_size";
const llvm::StringRef WIFuncsAnalysis::GET_STAGE_IN_GRID_ORIGIN = "__builtin_IB_get_stage_in_grid_origin";
const llvm::StringRef WIFuncsAnalysis::GET_STAGE_IN_GRID_SIZE = "__builtin_IB_get_stage_in_grid_size";
const llvm::StringRef WIFuncsAnalysis::GET_SYNC_BUFFER = "__builtin_IB_get_sync_buffer";
const llvm::StringRef WIFuncsAnalysis::GET_ASSERT_BUFFER = "__builtin_IB_get_assert_buffer";

WIFuncsAnalysis::WIFuncsAnalysis() : ModulePass(ID)
{
    initializeWIFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool WIFuncsAnalysis::runOnModule(Module& M)
{
    m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        if (pFunc->isDeclaration()) continue;
        runOnFunction(*pFunc);
    }

    // Update LLVM metadata based on IGC MetadataUtils
    m_pMDUtils->save(M.getContext());

    return true;
}

bool WIFuncsAnalysis::runOnFunction(Function& F)
{
    // Processing new function
    m_hasGroupID = false;
    m_hasLocalThreadID = false;
    m_hasGlobalOffset = false;
    m_hasLocalID = false;
    m_hasGlobalSize = false;
    m_hasLocalSize = false;
    m_hasWorkDim = false;
    m_hasNumGroups = false;
    m_hasEnqueuedLocalSize = false;
    m_hasStageInGridOrigin = false;
    m_hasStageInGridSize = false;
    m_hasSyncBuffer = false;
    m_hasAssertBuffer = false;
    m_hasStackCalls = false;

    // Visit the function
    visit(F);

    //Analyze the implicit arguments needed by this function
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

    const bool RequirePayloadHeader = m_ctx->m_DriverInfo.RequirePayloadHeader();
    // 8xi32 payload header packs 3xi32 global offset. If possible, use global offset directly.
    const auto PayloadHeaderType = AllowShortImplicitPayloadHeader(m_ctx) ? ImplicitArg::GLOBAL_OFFSET : ImplicitArg::PAYLOAD_HEADER;

    // All OpenCL kernels receive R0 and Payload Header implicitly
    if (isEntryFunc(m_pMDUtils, &F))
    {
        implicitArgs.push_back(ImplicitArg::R0);

        if (RequirePayloadHeader)
            implicitArgs.push_back(PayloadHeaderType);

        if (!m_ctx->platform.isProductChildOf(IGFX_XE_HP_SDV) &&
            IGC_IS_FLAG_ENABLED(EnableGlobalStateBuffer))
        {
            if (m_hasStackCalls)
            {
                implicitArgs.push_back(ImplicitArg::ArgType::IMPLICIT_ARG_BUFFER_PTR);

                // force add local id implicit args to kernel as we need
                // those to populate local id buffer that stack call functions
                // may use.
                implicitArgs.push_back(ImplicitArg::ArgType::LOCAL_ID_X);
                implicitArgs.push_back(ImplicitArg::ArgType::LOCAL_ID_Y);
                implicitArgs.push_back(ImplicitArg::ArgType::LOCAL_ID_Z);
            }
        }

        if (m_ctx->type == ShaderType::OPENCL_SHADER)
        {
            auto* Ctx = static_cast<OpenCLProgramContext*>(m_ctx);
            if (Ctx->needsDivergentBarrierHandling())
                m_hasLocalSize = true;
        }
    }
    else
    {
        if (m_hasGroupID)
        {
            implicitArgs.push_back(ImplicitArg::R0);
        }
        else if (m_hasLocalThreadID)
        {
            implicitArgs.push_back(ImplicitArg::R0);
        }
        if (m_hasGlobalOffset && RequirePayloadHeader)
        {
            implicitArgs.push_back(PayloadHeaderType);
        }
    }
    if (m_hasGlobalOffset && !RequirePayloadHeader)
    {
        implicitArgs.push_back(PayloadHeaderType);
    }
    if (m_hasWorkDim)
    {
        implicitArgs.push_back(ImplicitArg::WORK_DIM);
    }
    if (m_hasNumGroups)
    {
        implicitArgs.push_back(ImplicitArg::NUM_GROUPS);
    }
    if (m_hasGlobalSize)
    {
        implicitArgs.push_back(ImplicitArg::GLOBAL_SIZE);
    }
    if (m_hasLocalSize)
    {
        implicitArgs.push_back(ImplicitArg::LOCAL_SIZE);
    }
    if (m_hasLocalID)
    {
        implicitArgs.push_back(ImplicitArg::LOCAL_ID_X);
        implicitArgs.push_back(ImplicitArg::LOCAL_ID_Y);
        implicitArgs.push_back(ImplicitArg::LOCAL_ID_Z);
    }
    if (m_hasEnqueuedLocalSize)
    {
        implicitArgs.push_back(ImplicitArg::ENQUEUED_LOCAL_WORK_SIZE);
    }
    if (m_hasStageInGridOrigin)
    {
        implicitArgs.push_back(ImplicitArg::STAGE_IN_GRID_ORIGIN);
    }
    if (m_hasStageInGridSize)
    {
        implicitArgs.push_back(ImplicitArg::STAGE_IN_GRID_SIZE);
    }
    if (m_hasSyncBuffer)
    {
        implicitArgs.push_back(ImplicitArg::SYNC_BUFFER);
    }
    if (m_hasAssertBuffer)
    {
        implicitArgs.push_back(ImplicitArg::ASSERT_BUFFER_POINTER);
    }

    // Create the metadata representing the implicit args needed by this function
    ImplicitArgs::addImplicitArgs(F, implicitArgs, m_pMDUtils);

    return true;
}

void WIFuncsAnalysis::visitCallInst(CallInst& CI)
{
    Function* F = CI.getCalledFunction();

    if (CI.isIndirectCall() || (F && F->hasFnAttribute("visaStackCall")))
    {
        m_hasStackCalls = true;
        return;
    }
    if (F == nullptr)
    {
        return;
    }

    // Check for OpenCL WI function calls
    StringRef funcName = F->getName();
    if (funcName.equals(GET_LOCAL_ID_X) ||
        funcName.equals(GET_LOCAL_ID_Y) ||
        funcName.equals(GET_LOCAL_ID_Z))
    {
        m_hasLocalID = true;
    }
    else if (funcName.equals(GET_GROUP_ID))
    {
        m_hasGroupID = true;
    }
    else if (funcName.equals(GET_LOCAL_THREAD_ID))
    {
        m_hasLocalThreadID = true;
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_OFFSET))
    {
        m_hasGlobalOffset = true;
    }
    else if (funcName.equals(GET_GLOBAL_SIZE))
    {
        m_hasGlobalSize = true;
    }
    else if (funcName.equals(GET_LOCAL_SIZE))
    {
        m_hasLocalSize = true;
    }
    else if (funcName.equals(GET_WORK_DIM))
    {
        m_hasWorkDim = true;
    }
    else if (funcName.equals(GET_NUM_GROUPS))
    {
        m_hasNumGroups = true;
    }
    else if (funcName.equals(GET_ENQUEUED_LOCAL_SIZE)) {
        m_hasEnqueuedLocalSize = true;
    }
    else if (funcName.equals(GET_STAGE_IN_GRID_ORIGIN)) {
        m_hasStageInGridOrigin = true;
    }
    else if (funcName.equals(GET_STAGE_IN_GRID_SIZE)) {
        m_hasStageInGridSize = true;
    }
    else if (funcName.equals(GET_SYNC_BUFFER)) {
        m_hasSyncBuffer = true;
    }
    else if (funcName.equals(GET_ASSERT_BUFFER)) {
        m_hasAssertBuffer = true;
    }
}
