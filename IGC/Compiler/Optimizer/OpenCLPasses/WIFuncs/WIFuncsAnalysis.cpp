/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
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
IGC_INITIALIZE_PASS_END(WIFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WIFuncsAnalysis::ID = 0;

const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_X = "__builtin_IB_get_local_id_x";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_Y = "__builtin_IB_get_local_id_y";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_ID_Z = "__builtin_IB_get_local_id_z";
const llvm::StringRef WIFuncsAnalysis::GET_GROUP_ID = "__builtin_IB_get_group_id";
const llvm::StringRef WIFuncsAnalysis::GET_GLOBAL_SIZE = "__builtin_IB_get_global_size";
const llvm::StringRef WIFuncsAnalysis::GET_LOCAL_SIZE = "__builtin_IB_get_local_size";
const llvm::StringRef WIFuncsAnalysis::GET_GLOBAL_OFFSET = "__builtin_IB_get_global_offset";
const llvm::StringRef WIFuncsAnalysis::GET_WORK_DIM = "__builtin_IB_get_work_dim";
const llvm::StringRef WIFuncsAnalysis::GET_NUM_GROUPS = "__builtin_IB_get_num_groups";
const llvm::StringRef WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE = "__builtin_IB_get_enqueued_local_size";
const llvm::StringRef WIFuncsAnalysis::GET_STAGE_IN_GRID_ORIGIN = "__builtin_IB_get_stage_in_grid_origin";
const llvm::StringRef WIFuncsAnalysis::GET_STAGE_IN_GRID_SIZE = "__builtin_IB_get_stage_in_grid_size";
const llvm::StringRef WIFuncsAnalysis::GET_SYNC_BUFFER = "__builtin_IB_get_sync_buffer";

WIFuncsAnalysis::WIFuncsAnalysis() : ModulePass(ID)
{
    initializeWIFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool WIFuncsAnalysis::runOnModule(Module& M)
{
    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I)
    {
        Function* pFunc = &(*I);
        if (pFunc->isDeclaration()) continue;
        if (pFunc->hasFnAttribute("IndirectlyCalled")) continue;
        runOnFunction(*pFunc);
    }

    return true;
}

bool WIFuncsAnalysis::runOnFunction(Function& F)
{
    // Processing new function
    m_hasLocalID = false;
    m_hasGlobalSize = false;
    m_hasLocalSize = false;
    m_hasWorkDim = false;
    m_hasNumGroups = false;
    m_hasEnqueuedLocalSize = false;
    m_hasStageInGridOrigin = false;
    m_hasStageInGridSize = false;
    m_hasSyncBuffer = false;

    // Visit the function
    visit(F);

    //Analyze the implicit arguments needed by this function
    SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> implicitArgs;

    // All OpenCL kernels receive R0 and Payload Header implicitly
    implicitArgs.push_back(ImplicitArg::R0);
    implicitArgs.push_back(ImplicitArg::PAYLOAD_HEADER);

    // Check if additional implicit information is needed based on the function analysis
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

    // Create the metadata representing the implicit args needed by this function
    ImplicitArgs::addImplicitArgs(F, implicitArgs, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());

    return true;
}

void WIFuncsAnalysis::visitCallInst(CallInst& CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    // Check for OpenCL WI function calls
    StringRef funcName = CI.getCalledFunction()->getName();
    if (funcName.equals(GET_LOCAL_ID_X) ||
        funcName.equals(GET_LOCAL_ID_Y) ||
        funcName.equals(GET_LOCAL_ID_Z))
    {
        m_hasLocalID = true;
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
}
