/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/DeviceEnqueueFuncs/DeviceEnqueue.hpp"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-device-enqueue-func-analysis"
#define PASS_DESCRIPTION "Analyzes device enqueue functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DeviceEnqueueFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(DeviceEnqueueFuncsAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


// Register pass to igc-opt
#define PASS_FLAG2 "igc-device-enqueue-func-resolution"
#define PASS_DESCRIPTION2 "Resolve device enqueue functions"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(DeviceEnqueueFuncsResolution, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(DeviceEnqueueFuncsResolution, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)



char DeviceEnqueueFuncsAnalysis::ID = 0;

const llvm::StringRef GET_DEFAULT_DEVICE_QUEUE = "__builtin_IB_get_default_device_queue";
const llvm::StringRef GET_EVENT_POOL = "__builtin_IB_get_event_pool";
const llvm::StringRef GET_MAX_WORKGROUP_SIZE = "__builtin_IB_get_max_workgroup_size";
const llvm::StringRef GET_PARENT_EVENT = "__builtin_IB_get_parent_event";
const llvm::StringRef GET_PREFERED_WORKGROUP_MULTIPLE = "__builtin_IB_get_prefered_workgroup_multiple";
const llvm::StringRef GET_OBJECT_ID = "__builtin_IB_get_object_id";
const llvm::StringRef GET_BLOCK_SIMD_SIZE = "__builtin_IB_get_block_simd_size";

DeviceEnqueueFuncsAnalysis::DeviceEnqueueFuncsAnalysis() :
    ModulePass(ID),
    m_hasDeviceEnqueue(false)
{
    initializeDeviceEnqueueFuncsAnalysisPass(*PassRegistry::getPassRegistry());
}

bool DeviceEnqueueFuncsAnalysis::runOnModule(Module& M) {
    bool changed = false;
    m_pMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    // Run on all functions defined in this module
    for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
        Function* pFunc = &(*I);
        if (pFunc->isDeclaration())
        {
            continue;
        }
        if (runOnFunction(*pFunc))
        {
            changed = true;
        }
    }

    if(changed)
        m_pMDUtils->save(M.getContext());

    return changed;
}

bool DeviceEnqueueFuncsAnalysis::runOnFunction(Function& F) {

    m_newImplicitArgs.clear();
    m_newNumberedImplicitArgs.clear();

    // Visit the function
    visit(F);

    ImplicitArgs::addImplicitArgs(F, m_newImplicitArgs, m_pMDUtils);
    ImplicitArgs::addNumberedArgs(F, m_newNumberedImplicitArgs, m_pMDUtils);

    return m_hasDeviceEnqueue;
}

void DeviceEnqueueFuncsAnalysis::visitCallInst(CallInst& CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    StringRef funcName = CI.getCalledFunction()->getName();

    // Check for OpenCL image dimension function calls
    ImplicitArg::ArgType argType = ImplicitArg::NUM_IMPLICIT_ARGS;

    if (funcName == GET_DEFAULT_DEVICE_QUEUE)
    {
        argType = ImplicitArg::DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE;
    }
    else if (funcName == GET_EVENT_POOL)
    {
        argType = ImplicitArg::DEVICE_ENQUEUE_EVENT_POOL;
    }
    else if (funcName == GET_MAX_WORKGROUP_SIZE)
    {
        argType = ImplicitArg::DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE;
    }
    else if (funcName == GET_PARENT_EVENT)
    {
        argType = ImplicitArg::DEVICE_ENQUEUE_PARENT_EVENT;
    }
    else if (funcName == GET_PREFERED_WORKGROUP_MULTIPLE)
    {
        argType = ImplicitArg::DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE;
    }
    else if (funcName == GET_OBJECT_ID)
    {
        // Extract the arg num and add it to the appropriate data structure
        IGC_ASSERT_MESSAGE(IGCLLVM::getNumArgOperands(&CI) == 1, "get_object_id function is expected to have only one argument");

        // We support only compile-time constants as arguments of get_object_id()
        ConstantInt* callArg = dyn_cast<ConstantInt>(CI.getArgOperand(0));
        IGC_ASSERT_MESSAGE(callArg != NULL, "get_object_id function is expected to have only conatnt argument");

        m_newNumberedImplicitArgs[ImplicitArg::GET_OBJECT_ID].insert((int)callArg->getZExtValue());
        m_hasDeviceEnqueue = true;
    }
    else if (funcName == GET_BLOCK_SIMD_SIZE)
    {
        // Extract the arg num and add it to the appropriate data structure
        IGC_ASSERT_MESSAGE(IGCLLVM::getNumArgOperands(&CI) == 1, "get_block_simd_size function is expected to have only one argument");

        // We support only compile-time constants as arguments of get_object_id()
        ConstantInt* callArg = dyn_cast<ConstantInt>(CI.getArgOperand(0));
        IGC_ASSERT_MESSAGE(callArg != NULL, "get_block_simd_size function is expected to have only constant argument");

        m_newNumberedImplicitArgs[ImplicitArg::GET_BLOCK_SIMD_SIZE].insert((int)callArg->getZExtValue());
        m_hasDeviceEnqueue = true;
    }


    if (argType != ImplicitArg::NUM_IMPLICIT_ARGS)
    {

        if (std::find(m_newImplicitArgs.begin(), m_newImplicitArgs.end(), argType) == m_newImplicitArgs.end())
        {
            m_newImplicitArgs.push_back(argType);
        }

        m_hasDeviceEnqueue = true;
    }
}



char DeviceEnqueueFuncsResolution::ID = 0;

DeviceEnqueueFuncsResolution::DeviceEnqueueFuncsResolution() :
    FunctionPass(ID),
    m_Changed(false)
{
    initializeDeviceEnqueueFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool DeviceEnqueueFuncsResolution::runOnFunction(Function& F)
{
    m_Changed = false;

    m_implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());

    visit(F);

    return m_Changed;
}

void DeviceEnqueueFuncsResolution::visitCallInst(CallInst& CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    StringRef funcName = CI.getCalledFunction()->getName();
    Function& F = *(CI.getParent()->getParent());


    // Check for OpenCL image dimension function calls
    Value* enqueueResource = NULL;

    if (funcName == GET_DEFAULT_DEVICE_QUEUE)
    {
        enqueueResource = m_implicitArgs.getImplicitArg(F, ImplicitArg::DEVICE_ENQUEUE_DEFAULT_DEVICE_QUEUE);
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_EVENT_POOL)
    {
        enqueueResource = m_implicitArgs.getImplicitArg(F, ImplicitArg::DEVICE_ENQUEUE_EVENT_POOL);
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_MAX_WORKGROUP_SIZE)
    {
        enqueueResource = m_implicitArgs.getImplicitArg(F, ImplicitArg::DEVICE_ENQUEUE_MAX_WORKGROUP_SIZE);
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_PARENT_EVENT)
    {
        enqueueResource = m_implicitArgs.getImplicitArg(F, ImplicitArg::DEVICE_ENQUEUE_PARENT_EVENT);
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_PREFERED_WORKGROUP_MULTIPLE)
    {
        enqueueResource = m_implicitArgs.getImplicitArg(F, ImplicitArg::DEVICE_ENQUEUE_PREFERED_WORKGROUP_MULTIPLE);
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_OBJECT_ID)
    {
        enqueueResource = m_implicitArgs.getNumberedImplicitArg(F, ImplicitArg::GET_OBJECT_ID, (int)(cast<ConstantInt>(CI.getArgOperand(0))->getZExtValue()));
        IGC_ASSERT(enqueueResource != NULL);
    }
    else if (funcName == GET_BLOCK_SIMD_SIZE)
    {
        enqueueResource = m_implicitArgs.getNumberedImplicitArg(F, ImplicitArg::GET_BLOCK_SIMD_SIZE, (int)(cast<ConstantInt>(CI.getArgOperand(0))->getZExtValue()));
        IGC_ASSERT(enqueueResource != NULL);
    }


    if (enqueueResource != NULL)
    {
        CI.replaceAllUsesWith(enqueueResource);
        CI.eraseFromParent();

        m_Changed = true;
    }
}
