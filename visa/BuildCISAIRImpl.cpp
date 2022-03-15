/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "visa_igc_common_header.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "Common_ISA_framework.h"
#ifdef DLL_MODE
#include "RT_Jitter_Interface.h"
#else
#include "JitterDataStruct.h"
#endif
#include "VISAKernel.h"
#include "BinaryCISAEmission.h"
#include "Timer.h"
#include "BinaryEncoding.h"
#include "IsaDisassembly.h"

#include "G4_IR.hpp"
#include "FlowGraph.h"
#include "DebugInfo.h"
#include "IsaVerification.h"
#include "IGC/common/StringMacros.hpp"

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <sstream>
#include <functional>
#include <mutex>

using namespace vISA;
extern "C" int64_t getTimerTicks(unsigned int idx);

#define IS_GEN_PATH  (mBuildOption == VISA_BUILDER_GEN)
#define IS_BOTH_PATH  (mBuildOption == VISA_BUILDER_BOTH)
#define IS_GEN_BOTH_PATH  (mBuildOption == VISA_BUILDER_GEN || mBuildOption ==  VISA_BUILDER_BOTH)
#define IS_VISA_BOTH_PATH  (mBuildOption == VISA_BUILDER_VISA || mBuildOption ==  VISA_BUILDER_BOTH)

CISA_IR_Builder::~CISA_IR_Builder()
{
    m_cisaBinary->~CisaBinary();

    std::list<VISAKernelImpl *>::iterator iter_start = m_kernelsAndFunctions.begin();
    std::list<VISAKernelImpl *>::iterator iter_end = m_kernelsAndFunctions.end();

    while (iter_start != iter_end)
    {
        VISAKernelImpl *kernel = *iter_start;
        iter_start++;
        // don't call delete since vISAKernelImpl is allocated in memory pool
        kernel->~VISAKernelImpl();
    }

    if (needsToFreeWATable)
    {
        delete m_pWaTable;
    }
}

static const WA_TABLE *CreateVisaWaTable(TARGET_PLATFORM platform, Stepping step)
{
    WA_TABLE *pWaTable = new WA_TABLE;
    memset(pWaTable, 0, sizeof(WA_TABLE));


    if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaHeaderRequiredOnSimd16Sample16bit);
    }

    if ((platform == GENX_SKL) && (step == Step_A))
    {
        VISA_WA_ENABLE(pWaTable, WaSendsSrc1SizeLimitWhenEOT);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaSendsSrc1SizeLimitWhenEOT);
    }

    if ((platform == GENX_SKL && (step == Step_A || step == Step_B)) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(pWaTable, WaDisallow64BitImmMov);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaDisallow64BitImmMov);
    }

    if (platform == GENX_BDW || platform == GENX_CHV ||
        platform == GENX_BXT || platform == GENX_SKL)
    {
        VISA_WA_ENABLE(pWaTable, WaThreadSwitchAfterCall);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaThreadSwitchAfterCall);
    }

    if ((platform == GENX_SKL && step < Step_E) ||
        (platform == GENX_BXT && step <= Step_B))
    {
        VISA_WA_ENABLE(pWaTable, WaSrc1ImmHfNotAllowed);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaSrc1ImmHfNotAllowed);
    }

    if (platform == GENX_SKL && step == Step_A)
    {
        VISA_WA_ENABLE(pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaDstSubRegNumNotAllowedWithLowPrecPacked);
    }

    if ((platform == GENX_SKL && step < Step_C))
    {
        VISA_WA_ENABLE(pWaTable, WaDisableMixedModeLog);
        VISA_WA_ENABLE(pWaTable, WaDisableMixedModeFdiv);
        VISA_WA_ENABLE(pWaTable, WaDisableMixedModePow);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaDisableMixedModeLog);
        VISA_WA_DISABLE(pWaTable, WaDisableMixedModeFdiv);
        VISA_WA_DISABLE(pWaTable, WaDisableMixedModePow);
    }


    if ((platform == GENX_SKL && step < Step_C) ||
        platform == GENX_CHV)
    {
        VISA_WA_ENABLE(pWaTable, WaFloatMixedModeSelNotAllowedWithPackedDestination);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaFloatMixedModeSelNotAllowedWithPackedDestination);
    }

    // always disable in offline mode
    VISA_WA_DISABLE(pWaTable, WADisableWriteCommitForPageFault);

    if ((platform == GENX_SKL && step < Step_D) ||
        (platform == GENX_BXT && step == Step_A))
    {
        VISA_WA_ENABLE(pWaTable, WaDisableSIMD16On3SrcInstr);
    }

    if (platform == GENX_SKL && (step == Step_C || step == Step_D))
    {
        VISA_WA_ENABLE(pWaTable, WaSendSEnableIndirectMsgDesc);
    }
    else
    {
        VISA_WA_DISABLE(pWaTable, WaSendSEnableIndirectMsgDesc);
    }

    if (platform == GENX_SKL || platform == GENX_BXT)
    {
        VISA_WA_ENABLE(pWaTable, WaClearArfDependenciesBeforeEot);
    }

    if (platform == GENX_SKL && step == Step_A)
    {
        VISA_WA_ENABLE(pWaTable, WaDisableSendsSrc0DstOverlap);
    }

    if (platform >= GENX_SKL)
    {
        VISA_WA_ENABLE(pWaTable, WaMixModeSelInstDstNotPacked);
    }

    if (platform == GENX_SKL || platform == GENX_BXT)
    {
        VISA_WA_ENABLE(pWaTable, WaResetN0BeforeGatewayMessage);
    }

    // WA for future platforms
    if (platform == GENX_ICLLP)
    {
        VISA_WA_ENABLE(pWaTable, Wa_1406306137);
    }
    if (platform == GENX_ICLLP && (step == Step_A || step == Step_B))
    {
        VISA_WA_ENABLE(pWaTable, Wa_2201674230);
    }
    switch (platform)
    {
        case GENX_ICLLP:
            VISA_WA_ENABLE(pWaTable, Wa_1406950495);
            break;
        case GENX_TGLLP:
            VISA_WA_ENABLE(pWaTable, Wa_1406950495);
            VISA_WA_ENABLE(pWaTable, Wa_16013338947);
            break;
        case Xe_XeHPSDV:
            VISA_WA_ENABLE(pWaTable, Wa_1406950495);
            VISA_WA_ENABLE(pWaTable, Wa_16013338947);
            break;
        case Xe_DG2:
            VISA_WA_ENABLE(pWaTable, Wa_16013338947);
            break;
        case Xe_PVC:
            VISA_WA_ENABLE(pWaTable, Wa_16013338947);
            break;
        case Xe_PVCXT:
            VISA_WA_ENABLE(pWaTable, Wa_16013338947);
            if (step == Step_A)
            {
                VISA_WA_ENABLE(pWaTable, Wa_16012725276);
            }
            break;
        default:
            break;
    }

    return pWaTable;
}

// Change default values of some options according to WA_TABLE
// The values are set before parsing any flags specified by client
// (either within CreateVISABuilder() call or via VISABuilder interface)
// and may be overriden by client flags
static void AddWAOptions(Options &options, const WA_TABLE &waTable)
{
    if (waTable.Wa_1808850743 || waTable.Wa_1409909237)
    {
        options.setOptionInternally(vISA_noMaskWA, 2u);
        // Turn off jmpi as there is no wa for jmpi
        if (!options.getOption(vISA_KeepScalarJmp))
        {
            options.setOptionInternally(vISA_EnableScalarJmp, false);
        }
    }
}

int CISA_IR_Builder::CreateBuilder(
    CISA_IR_Builder *&builder,
    vISABuilderMode mode,
    VISA_BUILDER_OPTION buildOption,
    TARGET_PLATFORM platform,
    int numArgs,
    const char* flags[],
    const WA_TABLE *pWaTable)
{

    initTimer();

    if (builder)
    {
        assert(0);
        return VISA_FAILURE;
    }

    startTimer(TimerID::TOTAL);
    startTimer(TimerID::BUILDER);  // builder time ends with we call compile (i.e., it covers the IR construction time)

    builder = new CISA_IR_Builder(platform, buildOption, mode, COMMON_ISA_MAJOR_VER, COMMON_ISA_MINOR_VER, pWaTable);

    if (pWaTable)
    {
        AddWAOptions(builder->m_options, *pWaTable);
    }

    if (!builder->m_options.parseOptions(numArgs, flags))
    {
        delete builder;
        assert(0);
        return VISA_FAILURE;
    }

    // Set visa platform in the internal option for the case that IR_Builder or
    // G4_Kernel object are not easily available. However, getting platform
    // through options probably would be slower as it requires a hash table
    // lookup, so it should be used with caution.
    // Check if the given platform from function argument is valid.
    assert(platform != GENX_NONE && platform < TARGET_PLATFORM::ALL);
    TARGET_PLATFORM platformSet =
        static_cast<TARGET_PLATFORM>(builder->m_options.getuInt32Option(vISA_PlatformSet));
    // If the platform is specified in both the function argument and the
    // cmdline argument, the 2 values probably should be same.
    assert(platformSet == GENX_NONE || platformSet == platform);
    if (platformSet == GENX_NONE)
        builder->m_options.setOptionInternally(vISA_PlatformSet, static_cast<uint32_t>(platform));

#if defined(_DEBUG) || defined(_INTERNAL)
    builder->m_options.getOptionsFromEV();
#endif

    // This should not matter anymore since each kernel should set its Target attribute to 3D/CM
    auto targetMode = VISA_3D;
    builder->m_options.setTarget(targetMode);
    builder->m_options.setOptionInternally(vISA_isParseMode, mode == vISA_ASM_READER);

#ifndef DLL_MODE
    if (mode == vISA_ASM_READER)
    {
        // For vISA text input we always want to dump out vISA
        builder->m_options.setOptionInternally(vISA_DumpvISA, true);
    }
#endif

    // emit location info always for these cases
    if (mode == vISABuilderMode::vISA_DEFAULT && builder->m_options.getOption(vISA_outputToFile))
    {
        builder->m_options.setOptionInternally(vISA_EmitLocation, true);
    }

    // driver WaTable is not available in offline vISA executable mode
    // We instead create and initialize some of the known ones here
    if (!pWaTable)
    {
        builder->m_pWaTable = CreateVisaWaTable(platform, builder->m_options.GetStepping());
        builder->needsToFreeWATable = true;
    }

    if (mode == vISA_ASM_WRITER)
    {
        // If writing asm text, clear the stream and print the build version
        builder->ClearAsmTextStreams();
        builder->m_ssIsaAsm << printBuildVersion(builder->m_header) << "\n";
    }

    return VISA_SUCCESS;
}

int CISA_IR_Builder::DestroyBuilder(CISA_IR_Builder *builder)
{

    if (builder == NULL)
    {
        assert(0);
        return VISA_FAILURE;
    }

    delete builder;

    return VISA_SUCCESS;
}

VISAKernel* CISA_IR_Builder::GetVISAKernel(const std::string& kernelName) const
{
    if (kernelName.empty())
    {
        if (m_builderMode == vISA_ASM_READER)
        {
            auto kernel = m_kernelsAndFunctions.front();
            assert(kernel->getIsKernel());
            return static_cast<VISAKernel*>(kernel);
        }
        return static_cast<VISAKernel*>(m_kernel);
    }
    return static_cast<VISAKernel*>(m_nameToKernel.at(kernelName));
}

int CISA_IR_Builder::ClearAsmTextStreams()
{
    if (m_builderMode == vISA_ASM_WRITER)
    {
        m_ssIsaAsm.str(std::string());
        m_ssIsaAsm.clear();
        return VISA_SUCCESS;
    }

    assert(0 && "Should clear streams only in asm text writer mode!");
    return VISA_FAILURE;
}

int CISA_IR_Builder::AddKernel(VISAKernel *& kernel, const char* kernelName)
{
    if (kernel)
    {
        assert(0);
        return VISA_FAILURE;
    }

    VISAKernelImpl * kerneltemp = new (m_mem) VISAKernelImpl(VISA_BUILD_TYPE::KERNEL, this, kernelName);
    kernel = static_cast<VISAKernel*>(kerneltemp);
    m_kernel = kerneltemp;

    m_kernelsAndFunctions.push_back(kerneltemp);
    this->m_kernel_count++;
    this->m_nameToKernel[kernelName] = m_kernel;

    if (m_builderMode == vISA_ASM_WRITER)
    {
        m_ssIsaAsm << "//// KERNEL: ////\n";
        VISAKernel_format_provider fmt(m_kernel);
        m_ssIsaAsm << printFunctionDecl(&fmt, true) << "\n";
    }
    return VISA_SUCCESS;
}

int CISA_IR_Builder::SetPrevKernel(VISAKernel *& prevKernel)
{
    if (prevKernel)
    {
        m_prevKernel = (VISAKernelImpl*)prevKernel;
    }
    return VISA_SUCCESS;
}

int CISA_IR_Builder::AddFunction(VISAFunction *& function, const char* functionName)
{
    if (function)
    {
        assert(0);
        return VISA_FAILURE;
    }

    VISAKernelImpl* kerneltemp = new (m_mem) VISAKernelImpl(VISA_BUILD_TYPE::FUNCTION, this, functionName);
    function = static_cast<VISAFunction*>(kerneltemp);
    m_kernel = kerneltemp;
    m_kernelsAndFunctions.push_back(kerneltemp);
    m_kernel->m_functionId = this->m_function_count++;
    this->m_nameToKernel[functionName] = m_kernel;

    if (m_builderMode == vISA_ASM_WRITER)
    {
        m_ssIsaAsm << "\n" << "//// FUNCTION: ////\n";
        VISAKernel_format_provider fmt(m_kernel);
        m_ssIsaAsm << printFunctionDecl(&fmt, false) << "\n";
    }
    return VISA_SUCCESS;
}

int CISA_IR_Builder::AddPayloadSection(VISAFunction *& function, const char* functionName)
{
    if (function != NULL)
    {
        assert(0);
        return VISA_FAILURE;
    }

    VISAKernelImpl* kerneltemp = new (m_mem) VISAKernelImpl(VISA_BUILD_TYPE::PAYLOAD, this, functionName);
    function = static_cast<VISAFunction*>(kerneltemp);
    m_kernel = kerneltemp;
    m_kernelsAndFunctions.push_back(kerneltemp);
    m_kernel->m_functionId = this->m_function_count++;
    this->m_nameToKernel[functionName] = m_kernel;

    return VISA_SUCCESS;
}

// default size of the physical reg pool mem manager in bytes
#define PHY_REG_MEM_SIZE   (16*1024)

void restoreFCallState(
    G4_Kernel* kernel, const std::map<G4_BB*, G4_INST*>& savedFCallState)
{
    // Iterate over all BBs in kernel and fix all fcalls converted
    // to calls by reconverting them to fcall. This is required
    // because we want to reuse IR of function for next kernel.

    for (auto&& iter : savedFCallState)
    {
        auto curBB = iter.first;
        auto genOffset = curBB->back()->getGenOffset();
        curBB->pop_back();
        auto origInst = iter.second;
        assert(origInst->isFCall() || origInst->isFReturn());
        curBB->push_back(origInst);
        // set the genOffset in case of GenOffset being used when creating symbol table
        origInst->setGenOffset(genOffset);

        if (origInst->isFCall() && !origInst->asCFInst()->isIndirectCall())
        {
            // curBB must have a physical successor as we don't allow calls that do not return
            G4_BB* retBlock = curBB->getPhysicalSucc();
            G4_BB* retbbToConvert = retBlock->Preds.back();
            kernel->fg.removePredSuccEdges(retbbToConvert, retBlock);
            // Remove edge between call and previously joined function
            while (curBB->Succs.size() > 0)
            {
                kernel->fg.removePredSuccEdges(curBB, curBB->Succs.front());
            }

            // Restore edge to retBlock
            kernel->fg.addPredSuccEdges(curBB, retBlock);
        }
    }

    // Remove all in-edges to stack call function. These may have been added
    // to connect earlier kernels with the function.
    while (kernel->fg.getEntryBB()->Preds.size() > 0)
    {
        kernel->fg.removePredSuccEdges(
            kernel->fg.getEntryBB()->Preds.front(), kernel->fg.getEntryBB());
    }
}

G4_Kernel* CISA_IR_Builder::GetCallerKernel(G4_INST* inst)
{
    return &inst->getBuilder().kernel;
}

G4_Kernel* CISA_IR_Builder::GetCalleeKernel(G4_INST* fcall)
{
    assert(fcall->opcode() == G4_pseudo_fcall);
    std::string funcName = fcall->getSrc(0)->asLabel()->getLabel();
    auto iter = functionsNameMap.find(funcName);
    assert(iter != functionsNameMap.end() && "can't find function with given name");
    return iter->second;
}

void CISA_IR_Builder::ResetHasStackCall(
    std::list<std::list<vISA::G4_INST*>::iterator>& sgInvokeList,
    std::unordered_map<G4_Kernel*, std::list<std::list<G4_INST*>::iterator>>& callSites)
{
    for (auto& [func, callsites] : callSites)
    {
        bool hasStackCall = false;
        for (auto& it : callsites)
        {
            G4_INST* fcall = *it;
            assert(fcall->opcode() == G4_pseudo_fcall);
            bool isInSgInvokeList = std::find(sgInvokeList.begin(), sgInvokeList.end(), it) != sgInvokeList.end();
            if (!isInSgInvokeList)
            {
                hasStackCall = true;
                break;
            }
        }
        if (!hasStackCall)
        {
            func->fg.resetHasStackCalls();
        }
    }
}


void CISA_IR_Builder::CheckHazardFeatures(
    std::list<std::list<vISA::G4_INST*>::iterator>& sgInvokeList,
    std::unordered_map<G4_Kernel*, std::list<std::list<G4_INST*>::iterator>>& callSites)
{
    std::function<void(G4_Kernel*, G4_Kernel*, std::set<G4_Kernel*>&)> traverse;
    traverse = [&](G4_Kernel* root, G4_Kernel* func, std::set<G4_Kernel*>& visited)
    {
        for (auto& it : callSites[func])
        {
            G4_INST* fcall = *it;
            assert(fcall->opcode() == G4_pseudo_fcall);
            assert(func == GetCallerKernel(fcall));
            G4_Kernel* callee = GetCalleeKernel(fcall);

            assert(root != callee && "Detected a recursion that is not allowed");

            if (visited.count(callee))
                continue;

            visited.insert(callee);
            traverse(root, callee, visited);
        }
    };

    for (auto& it : sgInvokeList)
    {
        G4_INST* fcall = *it;
        // Check if there is a indirect call
        assert(!fcall->asCFInst()->isIndirectCall() && "Not supported for indirect calls");

        // Check recursion
        std::set<G4_Kernel*> visited;
        G4_Kernel* root = GetCalleeKernel(fcall);
        visited.insert(root);
        traverse(root, root, visited);
    }

}

void CISA_IR_Builder::CollectCallSites(
    std::list<VISAKernelImpl *>& functions,
    std::unordered_map<G4_Kernel*, std::list<std::list<G4_INST*>::iterator>>& callSites,
    std::list<std::list<vISA::G4_INST*>::iterator>& sgInvokeList)
{
    auto IsFCall = [](G4_INST* inst)
    {
        return inst->opcode() == G4_pseudo_fcall;
    };

    for (auto func : functions)
    {
        functionsNameMap[std::string(func->getName())] = func->getKernel();
        auto& instList = func->getKernel()->fg.builder->instList;
        std::list<G4_INST*>::iterator it = instList.begin();
        while (it != instList.end())
        {
            if (!IsFCall(*it))
            {
                it++;
                continue;
            }
            callSites[func->getKernel()].push_back(it);
            it++;
        }
    }

    // get sgInvokeList
    for (auto& [func, callsites] : callSites)
    {
        for (auto& it : callsites)
        {
            G4_INST* fcall = *it;
            assert(fcall->opcode() == G4_pseudo_fcall);
            // When callee is a invoke_simd target
            if (GetCalleeKernel(fcall)->getBoolKernelAttr(Attributes::ATTR_LTOInvokeOptTarget))
            {
                sgInvokeList.push_back(it);
            }
        }
    }
}

void CISA_IR_Builder::RemoveOptimizingFunction(
    std::list<VISAKernelImpl *>& functions,
    const std::list<std::list<vISA::G4_INST*>::iterator>& sgInvokeList)
{
    std::set<G4_Kernel*> removeList;
    for (auto& it : sgInvokeList)
    {
        G4_INST* fcall = *it;
        removeList.insert(GetCalleeKernel(fcall));
    }
    std::list<VISAKernelImpl*>::iterator i = functions.begin();
    while (i != functions.end())
    {
        auto func = *i;
        if (!removeList.count(func->getKernel())) {
            ++i;
        } else {
            functions.erase(i++);
        }
    }
}

void CISA_IR_Builder::ProcessSgInvokeList(
    const std::list<std::list<vISA::G4_INST*>::iterator>& sgInvokeList,
    std::unordered_map<G4_Kernel*, std::list<std::list<vISA::G4_INST*>::iterator>>& callee2Callers)
{
    for (auto& it : sgInvokeList)
    {
        G4_INST* fcall = *it;
        G4_Kernel* callee = GetCalleeKernel(fcall);
        callee2Callers[callee].push_back(it);
    }
}

#define DEBUG_LTO
#ifdef DEBUG_LTO
#define DEBUG_PRINT(msg) \
    std::cerr << __LINE__ << " " << msg;
#define DEBUG_UTIL(stmt) \
    stmt;
#else
#define DEBUG_PRINT(msg)
#define DEBUG_UTIL(stmt)
#endif

// Perform LTO including transforming stack calls to subroutine calls, subroutine calls to jumps, and inlining
void CISA_IR_Builder::LinkTimeOptimization(
    std::unordered_map<G4_Kernel*, std::list<std::list<vISA::G4_INST*>::iterator>>& callee2Callers,
    uint32_t options)
{
    bool call2jump = options & (1U << Linker_Call2Jump);
    std::map<G4_INST*, std::list<G4_INST*>::iterator> callsite;
    std::map<G4_INST*, std::list<G4_INST*>> rets;
    std::set<G4_Kernel*> visited;
    std::list<G4_INST*> dummyContainer;
    unsigned int raUID = 0;

    // append instructions from callee to caller
    for (auto& [callee, sgInvokeList] : callee2Callers)
    {
        G4_Declare *replacedArgDcl = nullptr;
        G4_Declare *replacedRetDcl = nullptr;

        for (auto& it : sgInvokeList)
        {
            bool inlining =         ( options & (1U << Linker_Inline)           );
            bool removeArgRet =     ( options & (1U << Linker_RemoveArgRet)     );
            bool removeStackArg =   ( options & (1U << Linker_RemoveStackArg)   );
            bool removeStackFrame = ( options & (1U << Linker_RemoveStackFrame) );
            G4_INST* fcall = *it;
            assert(fcall->opcode() == G4_pseudo_fcall);

            G4_Kernel* caller = GetCallerKernel(fcall);
            G4_Kernel* callee = GetCalleeKernel(fcall);
            G4_INST* calleeLabel = *callee->fg.builder->instList.begin();
            ASSERT_USER(calleeLabel->isLabel() == true, "Entry inst is not a label");

            // Change fcall to call
            fcall->setOpcode(G4_call);
            fcall->setSrc(calleeLabel->getSrc(0), 0);
            // we only record a single callsite to the target in order to convert to jumps
            // note that we don't need call2jump when inlining kicks in
            if ((!inlining) && callsite.find(calleeLabel) == callsite.end())
            {
                callsite[calleeLabel] = it;
            }
            else
            {
                callsite[calleeLabel] = dummyContainer.end();
            }

            auto& callerInsts = caller->fg.builder->instList;
            auto calleeInsts = callee->fg.builder->instList;

            if (removeArgRet)
            {
                auto& calleeBuilder = callee->fg.builder;
                auto& callerBuilder = caller->fg.builder;
                const RegionDesc *rDesc = callerBuilder->getRegionStride1();
                replacedArgDcl = replacedArgDcl ?
                    replacedArgDcl :
                    callerBuilder->createDeclareNoLookup("newArg", G4_GRF, callerBuilder->numEltPerGRF<Type_UD>(), 32, Type_UD);
                replacedRetDcl = replacedRetDcl ?
                    replacedRetDcl :
                    callerBuilder->createDeclareNoLookup("newRet", G4_GRF, callerBuilder->numEltPerGRF<Type_UD>(), 12, Type_UD);

                for (G4_INST* inst : calleeInsts)
                {
                    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                    {
                        G4_Operand *src = inst->getSrc(i);
                        if (!src) continue;
                        G4_Declare* topDcl = src->getTopDcl();
                        if (!topDcl) continue;
                        G4_Declare* rootDcl = topDcl->getRootDeclare();
                        if (calleeBuilder->isPreDefArg(rootDcl))
                        {
                            G4_Operand *replacedArgSrc = callerBuilder->createSrc(
                                    replacedArgDcl->getRegVar(),
                                    src->asSrcRegRegion()->getRegOff(),
                                    src->asSrcRegRegion()->getSubRegOff(),
                                    rDesc,
                                    src->getType());
                            inst->setSrc(replacedArgSrc, i);
                        }
                    }

                    G4_Operand *dst = inst->getDst();
                    if (!dst) continue;
                    G4_Declare* topDcl = dst->getTopDcl();
                    if (!topDcl) continue;
                    G4_Declare* rootDcl = topDcl->getRootDeclare();
                    if (calleeBuilder->isPreDefRet(rootDcl))
                    {
                        G4_DstRegRegion *replacedRetDst = callerBuilder->createDst(
                                replacedRetDcl->getRegVar(),
                                dst->asDstRegRegion()->getRegOff(),
                                dst->asDstRegRegion()->getSubRegOff(),
                                dst->asDstRegRegion()->getHorzStride(),
                                dst->getType());
                        inst->setDest(replacedRetDst);
                    }

                }
                for (G4_INST* inst : callerInsts)
                {
                    G4_Operand *dst = inst->getDst();
                    if (!dst) continue;
                    G4_Declare* topDcl = dst->getTopDcl();
                    if (!topDcl) continue;
                    G4_Declare* rootDcl = topDcl->getRootDeclare();
                    if (callerBuilder->isPreDefArg(rootDcl))
                    {
                        G4_DstRegRegion *replacedArgDst = callerBuilder->createDst(
                                replacedArgDcl->getRegVar(),
                                dst->asDstRegRegion()->getRegOff(),
                                dst->asDstRegRegion()->getSubRegOff(),
                                dst->asDstRegRegion()->getHorzStride(),
                                dst->getType());
                        inst->setDest(replacedArgDst);
                    }

                    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                    {
                        G4_Operand *src = inst->getSrc(i);
                        if (!src) continue;
                        G4_Declare* topDcl = src->getTopDcl();
                        if (!topDcl) continue;
                        G4_Declare* rootDcl = topDcl->getRootDeclare();
                        if (callerBuilder->isPreDefRet(rootDcl))
                        {
                            G4_Operand *replacedRetSrc = callerBuilder->createSrc(
                                    replacedRetDcl->getRegVar(),
                                    src->asSrcRegRegion()->getRegOff(),
                                    src->asSrcRegRegion()->getSubRegOff(),
                                    rDesc,
                                    src->getType());
                            inst->setSrc(replacedRetSrc, i);
                        }
                    }
                }
            }

            // A hash map to record how SP is populated from caller to callee
            std::map<G4_Declare*, long long> stackPointers;
            // A hash map to record where the instruction is on defs
            std::map<G4_Declare*, std::list<vISA::G4_INST*>::iterator> defInst;

            if (removeStackArg)
            {
                // collect instructions which store args to stack
                auto& callerBuilder = caller->fg.builder;
                auto& calleeBuilder = callee->fg.builder;
                auto getPointerOffset = [&](G4_INST *inst, long long offset)
                {
                    auto execSize = static_cast<int>(inst->getExecSize());
                    assert(execSize == 1);
                    switch(inst->opcode())
                    {
                        case G4_mov:
                            {
                                return offset;
                            }
                        case G4_add:
                            {
                                assert(inst->getSrc(1)->isImm());
                                return offset + inst->getSrc(1)->asImm()->getImm();
                            }
                        default:
                            {
                                assert(0);
                                return 0LL;
                            }
                    }
                };

                auto getRootDeclare = [&](G4_Operand *opnd)
                {
                    if (!opnd)
                        return (G4_Declare*) nullptr;
                    G4_Declare* topDcl = opnd->getTopDcl();
                    if (!topDcl)
                        return (G4_Declare*) nullptr;
                    return topDcl->getRootDeclare();

                };

                auto getBeginIt = [&](std::list<vISA::G4_INST*>::iterator it)
                {
                    // Trace backward until it reaches an update for SP
                    // This is where we start to push spilled arguments onto stack
                    auto beginIt = it;
                    for (; beginIt != callerInsts.begin(); --beginIt)
                    {
                        G4_INST *inst = *beginIt;
                        for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                        {
                            G4_Declare* rootDcl = getRootDeclare(inst->getSrc(i));
                            if (!rootDcl) continue;
                            G4_Operand *dst = inst->getDst();
                            if (rootDcl == callerBuilder->getFE_SP())
                            {
                                // the dst is updating SP
                                if (dst->getTopDcl() == callerBuilder->getFE_SP())
                                {
                                    auto prevIt = beginIt;
                                    prevIt --;
                                    G4_INST *prevInst = *prevIt;
                                    // It reaches the begining of function where it pushes a new frame.
                                    // It is not where we are looking for.
                                    if (prevInst->getDst()->getTopDcl() == callerBuilder->getFE_FP())
                                    {
                                        return it;
                                    }
                                    else
                                    {
                                        return beginIt;
                                    }
                                }
                            }
                        }
                    }
                    return it;
                };

                // A list of store in order to perform store-to-load forwarding
                std::list<std::list<vISA::G4_INST*>::iterator> storeList;

                auto beginIt = getBeginIt(it);
                bool noArgOnStack = (beginIt == it);
                for (auto callerIt = beginIt; callerIt != it; callerIt ++)
                {
                    G4_INST *inst = *callerIt;
                    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                    {
                        G4_Declare* rootDcl = getRootDeclare(inst->getSrc(i));
                        if (!rootDcl) continue;
                        G4_Operand *dst = inst->getDst();
                        if (rootDcl == callerBuilder->getFE_SP())
                        {
                            stackPointers[dst->getTopDcl()] = getPointerOffset(inst, stackPointers[rootDcl]);
                            defInst[dst->getTopDcl()] = callerIt;
                            // beginIt is the update of SP before pushing arguments onto stack
                            // We do not remove it immediately since we don't know if all S2L can be perform at this stage
                            std::string prefix = (removeStackFrame && callerIt != beginIt) ? "removeFrame " : "";
                            DEBUG_PRINT(prefix << "(" << stackPointers[dst->getTopDcl()] << ") ");
                            DEBUG_UTIL(inst->dump());
                            if (removeStackFrame && callerIt != beginIt)
                            {
                                callerInsts.erase(callerIt);
                            }
                        }
                        else if (stackPointers.find(rootDcl) != stackPointers.end())
                        {
                            long long offset = stackPointers[rootDcl];
                            if (inst->opcode() == G4_mov ||
                                inst->opcode() == G4_add)
                            {
                                auto execSize = static_cast<int>(inst->getExecSize());
                                if (execSize != 1)
                                {
                                    // Currently only support scalar type of operations
                                    DEBUG_PRINT("skip nonaddress calc\n");
                                    DEBUG_UTIL(inst->dump());
                                    continue;
                                }
                                stackPointers[dst->getTopDcl()] = getPointerOffset(inst, offset);
                                defInst[dst->getTopDcl()] = callerIt;
                                DEBUG_PRINT("(" << stackPointers[dst->getTopDcl()] << ") ");
                                DEBUG_UTIL(inst->dump());
                            }
                            else if (inst->opcode() == G4_sends ||
                                    inst->opcode() == G4_send)
                            {
                                assert(i == 0);
                                // Start adding argument stores to the list
                                storeList.push_back(callerIt);
                                DEBUG_PRINT("[ ]");
                                DEBUG_UTIL(inst->dump());
                            }
                            else
                            {
                                assert(0 && "not implemented");
                            }
                        }
                    }
                }
                // passing SP offset from caller to callee
                stackPointers[calleeBuilder->getFE_SP()] = stackPointers[callerBuilder->getFE_SP()];
                stackPointers[calleeBuilder->getFE_FP()] = stackPointers[callerBuilder->getFE_FP()];

                for (auto calleeIt = calleeInsts.begin(); calleeIt != calleeInsts.end(); calleeIt++)
                {
                    G4_INST *inst = *calleeIt;
                    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                    {
                        G4_Declare* rootDcl = getRootDeclare(inst->getSrc(i));
                        if (!rootDcl) continue;
                        G4_Operand *dst = inst->getDst();
                        if (rootDcl == calleeBuilder->getFE_SP())
                        {
                            stackPointers[dst->getTopDcl()] = getPointerOffset(inst, stackPointers[rootDcl]);
                            defInst[dst->getTopDcl()] = calleeIt;
                            std::string prefix = removeStackFrame ? "removeFrame " : "";
                            DEBUG_PRINT(prefix << "(" << stackPointers[dst->getTopDcl()] << ") ");
                            DEBUG_UTIL(inst->dump());
                            if (removeStackFrame)
                            {
                                calleeInsts.erase(calleeIt);
                                break;
                            }
                        }
                        else if (stackPointers.find(rootDcl) != stackPointers.end())
                        {
                            long long offset = stackPointers[rootDcl];
                            if (inst->opcode() == G4_mov)
                            {
                                auto execSize = static_cast<int>(inst->getExecSize());
                                assert(execSize == 1);
                                stackPointers[dst->getTopDcl()] = getPointerOffset(inst, offset);
                                defInst[dst->getTopDcl()] = calleeIt;
                                std::string prefix = removeStackFrame ? "removeFrame " : "";
                                DEBUG_PRINT(prefix << "(" << stackPointers[dst->getTopDcl()] << ") ");
                                DEBUG_UTIL(inst->dump());
                                if (removeStackFrame)
                                {
                                    calleeInsts.erase(calleeIt);
                                    break;
                                }
                            }
                            else if (inst->opcode() == G4_sends ||
                                    inst->opcode() == G4_send)
                            {
                                if (storeList.empty())
                                {
                                    // store prevFP to the callee's frame
                                    if (stackPointers[callerBuilder->getFE_SP()] ==
                                        stackPointers[getRootDeclare(inst->getSrc(0))])
                                    {
                                        DEBUG_PRINT("remove prevFP on callee's frame:\n");
                                        DEBUG_UTIL(inst->dump());
                                        calleeInsts.erase(calleeIt);
                                        break;
                                    }
                                    DEBUG_PRINT("skip for now (private variable on the callee's frame):\n");
                                    DEBUG_UTIL(inst->dump());
                                    assert(0);
                                }
                                auto storeIt = storeList.front();
                                G4_INST *storeInst = *storeIt;
                                G4_INST  *loadInst = inst;
                                storeList.pop_front();
                                DEBUG_PRINT("store-to-load forwarding:\n");
                                DEBUG_PRINT("\tstore:\t");
                                DEBUG_UTIL(storeInst->dump());
                                DEBUG_PRINT("\tload :\t");
                                DEBUG_UTIL(loadInst->dump());
                                assert(stackPointers[getRootDeclare(storeInst->getSrc(0))] ==
                                       stackPointers[getRootDeclare( loadInst->getSrc(0))] &&
                                       "Store and load have different SP offset");
                                // promote the load into mov
                                inst->setOpcode(G4_mov);
                                loadInst->setSrc(storeInst->getSrc(1), 0);
                                DEBUG_PRINT("\tforwarded:");
                                DEBUG_UTIL(inst->dump());
                                // erase the store
                                callerInsts.erase(storeIt);
                            }
                            else
                            {
                                assert(0 && "not implemented");
                            }
                        }
                    }
                }

                // All args has been removed on the stack
                // Remove SP updating instruction
                if (storeList.empty() && !noArgOnStack)
                {
                    DEBUG_PRINT("removed:");
                    DEBUG_UTIL((*defInst[callerBuilder->getFE_SP()])->dump());
                    callerInsts.erase(defInst[callerBuilder->getFE_SP()]);
                }

            }

            if (inlining)
            {
                auto& builder = caller->fg.builder;
                std::string funcName = fcall->getSrc(0)->asLabel()->getLabel();
                G4_Label *raLabel = builder->createLabel(funcName + "_ret" + std::to_string(raUID++), LABEL_BLOCK);
                G4_INST* ra = caller->fg.createNewLabelInst(raLabel);
                // We don't need calleeLabel (first instruction) anymore after inlining
                calleeInsts.pop_front();
                std::map<G4_Declare*, G4_Declare*> newDclMap;
                for (G4_INST* fret : calleeInsts)
                {
                    G4_INST* inst = fret->cloneInst();
                    auto cloneDcl = [&](G4_Operand* opd)
                    {
                        if (opd)
                        {
                            G4_Declare* topDcl = opd->getTopDcl();
                            G4_RegVar* var =
                                opd->isAddrExp() ?
                                opd->asAddrExp()->getRegVar() :
                                opd->getBase() && opd->getBase()->isRegVar()?
                                    opd->getBase()->asRegVar() :
                                    nullptr;
                            if (!var)
                                return;
                            G4_Declare* dcl = var->getDeclare();
                            if (topDcl && topDcl == callee->fg.builder->getStackCallArg())
                            {
                                G4_Declare* newDcl = caller->fg.builder->cloneDeclare(newDclMap, dcl);
                                opd->setTopDcl(caller->fg.builder->getStackCallArg());
                                opd->setBase(newDcl->getRegVar());
                                newDcl->setAliasDeclare(caller->fg.builder->getStackCallArg(), newDcl->getAliasOffset());
                            }
                            else if (topDcl && topDcl == callee->fg.builder->getStackCallRet())
                            {
                                G4_Declare* newDcl = caller->fg.builder->cloneDeclare(newDclMap, dcl);
                                opd->setTopDcl(caller->fg.builder->getStackCallRet());
                                opd->setBase(newDcl->getRegVar());
                                newDcl->setAliasDeclare(caller->fg.builder->getStackCallRet(), newDcl->getAliasOffset());
                            }
                            else if (topDcl && (topDcl == replacedArgDcl || topDcl == replacedRetDcl))
                            {
                                G4_Declare* newDcl = caller->fg.builder->createTempVar(topDcl->getTotalElems(), topDcl->getElemType(), Any, topDcl->getName());
                                newDcl->setAliasDeclare(topDcl, 0);
                                caller->Declares.push_back(newDcl);
                            }
                            else
                            {
                                G4_Declare* newDcl = caller->fg.builder->cloneDeclare(newDclMap, dcl);
                                if (opd->isAddrExp())
                                {
                                    assert(topDcl == nullptr);
                                    opd->asAddrExp()->setRegVar(newDcl->getRegVar());
                                }
                                else
                                {
                                    opd->setTopDcl(newDcl->getAliasDeclare() ? newDcl->getAliasDeclare() : newDcl);
                                    opd->setBase(newDcl->getRegVar());
                                }
                            }
                        }
                    };
                    for (int i = 0, numSrc = inst->getNumSrc(); i < numSrc; ++i)
                    {
                        cloneDcl(inst->getSrc(i));
                    }
                    cloneDcl(inst->getDst());
                    // add predicate into declaration list
                    if (G4_VarBase* flag = inst->getCondModBase())
                    {
                        caller->Declares.push_back(flag->asRegVar()->getDeclare());
                    }
                    callerInsts.insert(it, inst);
                    if (inst->opcode() != G4_pseudo_fret)
                        continue;
                    // Change inst to goto
                    inst->setOpcode(G4_goto);
                    inst->asCFInst()->setUip(raLabel);
                }
                // insert return label for goto
                callerInsts.insert(it, ra);
                // remove the call
                callerInsts.erase(it);
            }
            else
            {
                // We only have to copy callee's instructions once for subrountine calls
                if (visited.find(callee) != visited.end())
                {
                    continue;
                }
                visited.insert(callee);
                for (G4_INST* fret : calleeInsts)
                {
                    if (fret->opcode() != G4_pseudo_fret)
                        continue;
                    // Change fret to ret
                    fret->setOpcode(G4_return);
                    rets[calleeLabel].push_back(fret);
                }
                callerInsts.insert(callerInsts.end(), calleeInsts.begin(), calleeInsts.end());
                // Append declarations from callee to caller
                auto callerDclCount = caller->Declares.size();
                for (auto curDcl : callee->Declares)
                {
                    curDcl->setDeclId(curDcl->getDeclId() + callerDclCount);
                    caller->Declares.push_back(curDcl);
                }
            }
        }
    }

    if (call2jump)
    {
        for(auto& [label, itCall]: callsite)
        {
            if (itCall == dummyContainer.end())
                continue;
            G4_INST* call = *itCall;
            G4_Kernel* caller = GetCallerKernel(call);
            auto& builder = caller->fg.builder;
            call->setOpcode(G4_goto);
            call->asCFInst()->setUip(label->getLabel());
            std::string funcName = call->getSrc(0)->asLabel()->getLabel();
            G4_Label *raLabel = builder->createLabel(funcName + "_ret", LABEL_BLOCK);
            G4_INST* ra = caller->fg.createNewLabelInst(raLabel);
            auto& callerInsts = caller->fg.builder->instList;
            callerInsts.insert(++itCall, ra);

            for (G4_INST* ret : rets[label])
            {
                ret->setOpcode(G4_goto);
                ret->asCFInst()->setUip(raLabel);
            }
        }
    }

}

// Stitch the FG of subFunctions to mainFunc
// mainFunc could be a kernel or a non-kernel function.
// It also modifies pseudo_fcall/fret in to call/ret opcodes.
// ToDo: may consider stitching only functions that may be called by this kernel/function
static void Stitch_Compiled_Units(
    G4_Kernel* mainFunc, std::map<std::string, G4_Kernel*>& subFuncs,
    std::map<G4_BB*, G4_INST*>& FCallRetMap)
{

    // Append subFunctions to mainFunc
    for (auto&& iter : subFuncs)
    {
        G4_Kernel* callee = iter.second;
        mainFunc->fg.append(callee->fg);

        // merge the relocation when append
        if (!callee->getRelocationTable().empty())
            mainFunc->getRelocationTable().insert(mainFunc->getRelocationTable().end(),
                callee->getRelocationTable().begin(), callee->getRelocationTable().end());

        ASSERT_USER(mainFunc->getNumRegTotal() == callee->getNumRegTotal(), "caller and callee cannot have different GRF modes");
    }

    mainFunc->fg.reassignBlockIDs();
    mainFunc->fg.setPhysicalPredSucc(); // this is to locate the next BB after an fcall

    auto builder = mainFunc->fg.builder;
    // Change fcall/fret to call/ret and setup caller/callee edges
    for (G4_BB* cur : mainFunc->fg)
    {
        if (cur->isEndWithFCall())
        {
            // Setup successor/predecessor
            G4_INST* fcall = cur->back();

            if (!fcall->asCFInst()->isIndirectCall())
            {
                // Setup caller/callee edges for direct call
                // ToDo: remove this once SWSB is moved before stithcing, as we would not need to maintain CFG otherwise
                std::string funcName = fcall->getSrc(0)->asLabel()->getLabel();

                auto iter = subFuncs.find(funcName);
                assert(iter != subFuncs.end() && "can't find function with given name");
                G4_Kernel* callee = iter->second;
                G4_BB* retBlock = cur->Succs.front();
                ASSERT_USER(cur->Succs.size() == 1, "fcall basic block cannot have more than 1 successor");
                ASSERT_USER(retBlock->Preds.size() == 1, "block after fcall cannot have more than 1 predecessor");

                // Remove old edge
                retBlock->Preds.erase(retBlock->Preds.begin());
                cur->Succs.erase(cur->Succs.begin());

                // Connect new fg
                mainFunc->fg.addPredSuccEdges(cur, callee->fg.getEntryBB());
                mainFunc->fg.addPredSuccEdges(callee->fg.getUniqueReturnBlock(), retBlock);

                // propagate properties of callee to mainFunc
                // usesBarrier property is propagated to IGC and onwards in to NEO patch
                // token. we need this logic here to propagate barrier usage to IGC and
                // further to NEO so it can setup WG size appropriately. without this
                // setting barrier would cause machine to hang.
                // TODO: How to set usesBarrier when callee is indirect?
                mainFunc->fg.builder->getJitInfo()->usesBarrier |= callee->fg.builder->getJitInfo()->usesBarrier;

                G4_INST* calleeLabel = callee->fg.getEntryBB()->front();
                ASSERT_USER(calleeLabel->isLabel() == true, "Entry inst is not label");

                auto callInst = builder->createInternalInst(
                    fcall->getPredicate(), G4_call, nullptr, g4::NOSAT, fcall->getExecSize(),
                    fcall->getDst(), calleeLabel->getSrc(0), fcall->getSrc(0), fcall->getOption());
                callInst->inheritDIFrom(fcall);
                callInst->inheritSWSBFrom(fcall);
                cur->pop_back();
                cur->push_back(callInst);
            }
            else
            {
                // src0 is dont care for indirect call as long it's not a label
                auto callInst = builder->createInternalInst(
                    fcall->getPredicate(), G4_call, nullptr, g4::NOSAT, fcall->getExecSize(),
                    fcall->getDst(), fcall->getSrc(0), fcall->getSrc(0), fcall->getOption());
                callInst->inheritDIFrom(fcall);
                callInst->inheritSWSBFrom(fcall);
                cur->pop_back();
                cur->push_back(callInst);
            }
            FCallRetMap[cur] = fcall;
        }
    }

    // Change fret to ret
    for (G4_BB* cur : mainFunc->fg)
    {
        if (cur->isEndWithFRet())
        {
            G4_INST* fret = cur->back();
            auto retInst = builder->createInternalInst(
                fret->getPredicate(), G4_return, nullptr, g4::NOSAT, fret->getExecSize(),
                builder->createNullDst(Type_UD), fret->getSrc(0), fret->getSrc(1), fret->getOption());
            retInst->inheritDIFrom(fret);
            retInst->inheritSWSBFrom(fret);
            cur->pop_back();
            cur->push_back(retInst);
            FCallRetMap[cur] = fret;
        }
    }

    // Append declarations and color attributes from all callees to mainFunc
    for (auto iter : subFuncs)
    {
        G4_Kernel* callee = iter.second;
        for (auto curDcl : callee->Declares)
        {
            mainFunc->Declares.push_back(curDcl);
        }
    }

    mainFunc->dumpToFile("after.stitched");
}


typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern int CISAparse(CISA_IR_Builder *builder);
extern YY_BUFFER_STATE CISA_scan_string(const char* yy_str);
extern void CISA_delete_buffer(YY_BUFFER_STATE buf);
static std::mutex mtx;

int CISA_IR_Builder::ParseVISAText(const std::string& visaText, const std::string& visaTextFile)
{
    const std::lock_guard<std::mutex> lock(mtx);
#if defined(__linux__) || defined(_WIN64) || defined(_WIN32)
    // Direct output of parser to null
#if defined(_WIN64) || defined(_WIN32)
    CISAout = fopen("nul", "w");
#else
    CISAout = fopen("/dev/null", "w");
#endif

    int status = VISA_SUCCESS;

    // Dump the visa text
    if (m_options.getOption(vISA_GenerateISAASM) && !visaTextFile.empty())
    {
        std::ofstream ofs(visaTextFile.c_str(), std::ofstream::out);
        if (ofs.good()) {
            ofs << visaText;
            ofs.close();
        }
    }

    YY_BUFFER_STATE visaBuf = CISA_scan_string(visaText.c_str());
    if (CISAparse(this) != 0)
    {
#ifndef DLL_MODE
        std::cerr << "Parsing visa text failed.";
        if (!visaTextFile.empty())
        {
            std::cerr << " Please examine " << visaTextFile << " and fix the error";
        }
        std::cerr << "\n" << criticalMsg.str();
#endif //DLL_MODE
        status = VISA_FAILURE;
    }
    CISA_delete_buffer(visaBuf);

    if (CISAout)
    {
        fclose(CISAout);
    }

    // run vISA verifier to cath any additional errors.
    // the subsequent vISABuilder::Compile() call is assumed to always succeed after verifier checks.
    if (status == VISA_SUCCESS)
    {
        status = verifyVISAIR();
    }

    return status;
#else
    assert(0 && "vISA asm parsing not supported on this platform");
    return VISA_FAILURE;
#endif
}

// Parses inline asm file from ShaderOverride
int CISA_IR_Builder::ParseVISAText(const std::string& visaFile)
{
#if defined(__linux__) || defined(_WIN64) || defined(_WIN32)
    // Direct output of parser to null
#if defined(_WIN64) || defined(_WIN32)
    CISAout = fopen("nul", "w");
#else
    CISAout = fopen("/dev/null", "w");
#endif
    CISAin = fopen(visaFile.c_str(), "r");
    if (!CISAin)
    {
        assert(0 && "Failed to open file");
        return VISA_FAILURE;
    }

    if (CISAparse(this) != 0)
    {
        assert(0 && "Parsing visa text failed");
        return VISA_FAILURE;
    }
    fclose(CISAin);

    if (CISAout)
    {
        fclose(CISAout);
    }
    return VISA_SUCCESS;
#else
    assert(0 && "Asm parsing not supported on this platform");
    return VISA_FAILURE;
#endif
}

// default size of the kernel mem manager in bytes
#define KERNEL_MEM_SIZE    (4*1024*1024)
int CISA_IR_Builder::Compile(const char* nameInput, std::ostream* os, bool emit_visa_only)
{
    stopTimer(TimerID::BUILDER);   // TIMER_BUILDER is started when builder is created
    int status = VISA_SUCCESS;
    std::string name = std::string(nameInput);

    if (IS_VISA_BOTH_PATH)
    {
        if (m_builderMode == vISA_ASM_WRITER)
        {
            assert(0 && "Should not be calling Compile() in asm text writer mode!");
            return VISA_FAILURE;
        }
        if (IS_BOTH_PATH)
        {
            m_options.setOptionInternally(vISA_NumGenBinariesWillBePatched, (uint32_t) 1);
        }
        m_cisaBinary->initCisaBinary(m_kernel_count, m_function_count);
        m_cisaBinary->setMajorVersion((unsigned char)this->m_header.major_version);
        m_cisaBinary->setMinorVersion((unsigned char)this->m_header.minor_version);
        m_cisaBinary->setMagicNumber(COMMON_ISA_MAGIC_NUM);

        CBinaryCISAEmitter cisaBinaryEmitter;
        int status = VISA_SUCCESS;
        int kernelIndex = 0;
        for (auto func : m_kernelsAndFunctions)
        {
            func->finalizeAttributes();
            unsigned int binarySize = 0;
            status = cisaBinaryEmitter.Emit(func, binarySize);
            m_cisaBinary->initKernel(kernelIndex, func);
            kernelIndex++;
        }
        m_cisaBinary->finalizeCisaBinary();

        if (status != VISA_SUCCESS)
        {
            return status;
        }

        if (m_options.getOption(vISA_GenerateISAASM))
        {
            status = m_cisaBinary->isaDump(m_kernelsAndFunctions, &m_options);
            if (status != VISA_SUCCESS)
            {
                return status;
            }
        }

        if (!m_options.getOption(vISA_NoVerifyvISA))
        {
            status = verifyVISAIR();
            if (status != VISA_SUCCESS)
            {
                return status;
            }
        }
    }

    /*
        In case there is an assert in compilation phase, at least vISA binary will be generated.
    */
    if (IS_VISA_BOTH_PATH && m_options.getOption(vISA_DumpvISA) && nameInput && !os)
    {
        if (CisaFramework::allowDump(m_options, name))
            status = m_cisaBinary->dumpToFile(name);
    }

    if (os && emit_visa_only)
    {
        return m_cisaBinary->dumpToStream(os);
    }

    if (m_options.getuInt32Option(vISA_Linker) & (1U << Linker_Subroutine))
    {
        std::map<std::string, G4_Kernel*> functionsNameMap;
        G4_Kernel* mainFunc = m_kernelsAndFunctions.front()->getKernel();
        assert(m_kernelsAndFunctions.front()->getIsKernel() && "mainFunc must be the kernel entry");
        std::unordered_map<G4_Kernel*, std::list<std::list<G4_INST*>::iterator>> callSites;
        std::list<std::list<G4_INST*>::iterator> sgInvokeList;
        CollectCallSites(m_kernelsAndFunctions, callSites, sgInvokeList);

        if (sgInvokeList.size())
        {
            assert(callSites.begin()->first == mainFunc);
            CheckHazardFeatures(sgInvokeList, callSites);

            ResetHasStackCall(sgInvokeList, callSites);

            RemoveOptimizingFunction(m_kernelsAndFunctions, sgInvokeList);

            std::unordered_map<G4_Kernel*, std::list<std::list<vISA::G4_INST*>::iterator>> callee2Callers;
            ProcessSgInvokeList(sgInvokeList, callee2Callers);

            // Copy callees' context to callers and convert to subroutine calls
            LinkTimeOptimization(callee2Callers,
                    m_options.getuInt32Option(vISA_Linker));
        }
    }



    VISAKernelImpl* oldMainKernel = nullptr;
    if (IS_GEN_BOTH_PATH)
    {
        Mem_Manager mem(4096);
        common_isa_header pseudoHeader;
        // m_kernels contains kernels and functions to compile.

        pseudoHeader.num_kernels = 0;
        pseudoHeader.num_functions = 0;
        for (const VISAKernelImpl* func : m_kernelsAndFunctions)
        {
            if (func->getIsKernel())
            {
                pseudoHeader.num_kernels++;
            }
            else
            {
                pseudoHeader.num_functions++;
            }
        }

        pseudoHeader.functions = (function_info_t*)mem.alloc(sizeof(function_info_t) * pseudoHeader.num_functions);

        int i;
        unsigned int k = 0;
        bool isInPatchingMode = m_options.getuInt32Option(vISA_CodePatch) >= CodePatch_Enable_NoLTO && m_prevKernel;
        uint32_t localScheduleStartKernelId = m_options.getuInt32Option(vISA_LocalScheduleingStartKernel);
        uint32_t localScheduleEndKernelId = m_options.getuInt32Option(vISA_LocalScheduleingEndKernel);
        VISAKernelImpl* mainKernel = nullptr;
        std::list<VISAKernelImpl*>::iterator iter = m_kernelsAndFunctions.begin();
        std::list<VISAKernelImpl*>::iterator end = m_kernelsAndFunctions.end();
        for (i = 0; iter != end; iter++, i++)
        {
            VISAKernelImpl* kernel = (*iter);
            if ((uint32_t)i < localScheduleStartKernelId || (uint32_t)i > localScheduleEndKernelId)
            {
                kernel->setLocalSheduleable(false);
            }

            mainKernel = (kernel->getIsKernel()) ? kernel : mainKernel;
            kernel->finalizeAttributes();
            kernel->getIRBuilder()->setType(kernel->getType());
            if (kernel->getIsKernel() == false)
            {
                if (kernel->getIRBuilder()->getArgSize() < kernel->getKernelFormat()->input_size)
                {
                    kernel->getIRBuilder()->setArgSize(kernel->getKernelFormat()->input_size);
                }
                if (kernel->getIRBuilder()->getRetVarSize() < kernel->getKernelFormat()->return_value_size)
                {
                    kernel->getIRBuilder()->setRetVarSize(kernel->getKernelFormat()->return_value_size);
                }

                auto nameLen = strlen((*iter)->getKernel()->getName()) + 1;
                pseudoHeader.functions[k].name = (char*)mem.alloc(nameLen);
                strcpy_s(pseudoHeader.functions[k].name, nameLen, (*iter)->getKernel()->getName());
                k++;
            }

            if (kernel->getIsPayload())
            {
                // Copy main kernel's declarations (shader body) into payload section
                kernel->CopyVars(mainKernel);
                kernel->getKernel()->Declares = mainKernel->getKernel()->Declares;
                kernel->getIRBuilder()->setInputR1(mainKernel->getIRBuilder()->getInputR1());
                kernel->getIRBuilder()->setRealR0(mainKernel->getIRBuilder()->getRealR0());
                kernel->getIRBuilder()->setBuiltInR0(mainKernel->getIRBuilder()->getBuiltinR0());
                // Set payload LiveOuts to be output
                uint32_t inputCount = mainKernel->getIRBuilder()->getInputCount();
                for (unsigned int id = 0; id < inputCount; id++)
                {
                    input_info_t* input_info = mainKernel->getIRBuilder()->getInputArg(id);
                    // skip pseudo input for register bindings.
                    if (input_info->isPseudoInput())
                    {
                        continue;
                    }
                    vISA::G4_Declare* dcl = input_info->dcl;
                    if (dcl->isPayloadLiveOut())
                    {
                        dcl->getRootDeclare()->setLiveOut();
                    }
                }
                mainKernel->getIRBuilder()->getRealR0()->setLiveOut();
            }

            if ((kernel->getIsKernel() && isInPatchingMode) ||
                (kernel->getvIsaInstCount() == 0 && kernel->getIsPayload()))
            {
                continue;
            }
            int status =  kernel->compileFastPath();
            if (status != VISA_SUCCESS)
            {
                stopTimer(TimerID::TOTAL);
                return status;
            }
            if (kernel->getIsPayload())
            {
                // Remove payload live-outs from the kernel after the compilation since
                // they will not be outputs anymore after stitching.
                mainKernel->getIRBuilder()->getRealR0()->resetLiveOut();
                const std::vector<input_info_t*>& inputs = mainKernel->getIRBuilder()->m_inputVect;
                for (const input_info_t* input_info : inputs)
                {
                    vISA::G4_Declare* dcl = input_info->dcl;
                    if (dcl->isPayloadLiveOut())
                    {
                        dcl->resetLiveOut();
                    }
                }
            }
        }
        // Here we change the payload section as the main kernel in m_kernelsAndFunctions
        // During stitching, all functions will be cloned and stitched to the main kernel.
        // Demoting the shader body to a function type makes it intact
        // so we can stitch it again to another SIMD size of payload section
        if (m_options.getuInt32Option(vISA_CodePatch))
        {
            assert(m_kernelsAndFunctions.front()->getIsKernel());
            for (auto func : m_kernelsAndFunctions)
            {
                if (func->getIsKernel())
                {
                    oldMainKernel = func;
                    func->setType(VISA_BUILD_TYPE::FUNCTION);
                }
                else if (func->getIsPayload())
                {
                    func->setType(VISA_BUILD_TYPE::KERNEL);
                }
            }
            m_kernelsAndFunctions.pop_front();
            if (isInPatchingMode)
            {
                m_kernelsAndFunctions.push_back(m_prevKernel);
            }
            else
            {
                m_kernelsAndFunctions.push_back(oldMainKernel);
            }
            // payloadSection is the main kernel at this point
            // We need to copy essential info from the shader body to the main kernel
            auto payloadSection = m_kernelsAndFunctions.front()->getIRBuilder();
            auto shaderBody = m_kernelsAndFunctions.back()->getIRBuilder();
            *payloadSection->getJitInfo() = *shaderBody->getJitInfo();

            payloadSection->m_inputVect = shaderBody->m_inputVect;
            if (shaderBody->kernel.hasKernelDebugInfo())
            {
                payloadSection->kernel.setKernelDebugInfo(
                    shaderBody->kernel.getKernelDebugInfo());
            }

            if (shaderBody->kernel.hasGTPinInit())
            {
                std::vector<unsigned> globalFreeRegs;
                unsigned int i = 0, j = 0;
                auto payloadSectionGTPin = payloadSection->kernel.getGTPinData();
                auto shaderBodyGTPin = shaderBody->kernel.getGTPinData();
                while (i < payloadSectionGTPin->getNumFreeGlobalRegs() &&
                       j < shaderBodyGTPin->getNumFreeGlobalRegs())
                {
                    unsigned int iFreeGRF = payloadSectionGTPin->getFreeGlobalReg(i);
                    unsigned int jFreeGRF =     shaderBodyGTPin->getFreeGlobalReg(j);
                    if (iFreeGRF < jFreeGRF)
                    {
                        i ++;
                    }
                    else if (iFreeGRF > jFreeGRF)
                    {
                        j ++;
                    }
                    else // iFreeGRF == jFreeGRF
                    {
                        globalFreeRegs.push_back(iFreeGRF);
                        i ++;
                        j ++;
                    }
                }
                payloadSection->kernel.setGTPinData(
                    shaderBody->kernel.getGTPinData());
                // If the number of free regs in payload section is 0,
                // it means the compilation is skipped and we don't have to do anything
                if (payloadSectionGTPin->getNumFreeGlobalRegs())
                {
                    payloadSection->kernel.getGTPinData()->setFreeGlobalRegs(globalFreeRegs);
                }
            }
        }

        // Preparing for stitching some functions to other functions
        // There are two stiching policies:
        // 1. vISA_noStitchExternFunc == false
        //    Stitch all non-kernel functions to all kernels
        // 2. vISA_noStitchExternFunc == true
        //    Stitch only non-external functions. Stich them to all kernels and external functions

        // mainFunctions: functions or kernels those will be stiched by others
        // Thses functions/kernels will be the unit of compilePostOptimize
        VISAKernelImpl::VISAKernelImplListTy mainFunctions;
        // subFunctions: functions those will stitch to others
        VISAKernelImpl::VISAKernelImplListTy subFunctions;
        std::map<std::string, G4_Kernel*> subFunctionsNameMap;
        // For functions those will be stitch to others, create table to map their name to G4_Kernel
        for (auto func : m_kernelsAndFunctions)
        {
            if (func->getIsKernel()) {
                // kernels must be stitched
                mainFunctions.push_back(func);
                continue;
            } else {
                if (!m_options.getOption(vISA_noStitchExternFunc)) {
                    // Policy 1: all functions will stitch to kernels
                    subFunctions.push_back(func);
                    subFunctionsNameMap[std::string(func->getName())] = func->getKernel();
                } else {
                    // Policy 2: external functions will be stitched, non-external functions will stitch to others
                    if (func->getKernel()->getBoolKernelAttr(Attributes::ATTR_Extern))
                    {
                        mainFunctions.push_back(func);
                    }
                    else
                    {
                        subFunctions.push_back(func);
                        subFunctionsNameMap[std::string(func->getName())] = func->getKernel();
                    }
                }
            }
        }

        // reset debug info offset of functionsToStitch
        for (auto func : subFunctions)
        {
            if (m_options.getOption(vISA_GenerateDebugInfo))
            {
                func->getKernel()->getKernelDebugInfo()->resetRelocOffset();
                resetGenOffsets(*func->getKernel());
            }
        }

        bool hasPayloadPrologue = m_options.getuInt32Option(vISA_CodePatch) >= CodePatch_Payload_Prologue;
        // stitch functions and compile to gen binary
        for (auto func : mainFunctions)
        {
            unsigned int genxBufferSize = 0;

            // store the BBs with FCall and FRet, which must terminate the BB
            std::map<G4_BB*, G4_INST*> origFCallFRet;
            if (!hasPayloadPrologue)
            {
                Stitch_Compiled_Units(func->getKernel(), subFunctionsNameMap, origFCallFRet);
            }

            func->compilePostOptimize();
            if (hasPayloadPrologue)
            {
                if (!isInPatchingMode)
                {
                    for (auto shaderBody : subFunctions)
                    {
                        shaderBody->getKernel()->fg.reassignBlockIDs();
                        shaderBody->getKernel()->fg.setPhysicalPredSucc();
                        shaderBody->compilePostOptimize();
                    }
                }
                // Append shader body to payload section
                for (auto&& iter : subFunctionsNameMap)
                {
                    G4_Kernel* callee = iter.second;
                    func->getKernel()->fg.append(callee->fg);
                }
            }
            void* genxBuffer = func->encodeAndEmit(genxBufferSize);
            func->setGenxBinaryBuffer(genxBuffer, genxBufferSize);
            if (m_options.getOption(vISA_GenerateDebugInfo))
            {
                func->computeAndEmitDebugInfo(subFunctions);
            }
            restoreFCallState(func->getKernel(), origFCallFRet);

        }


    }

    if (IS_VISA_BOTH_PATH && m_options.getOption(vISA_DumpvISA))
    {
        unsigned int numGenBinariesWillBePatched = m_options.getuInt32Option(vISA_NumGenBinariesWillBePatched);

        if (numGenBinariesWillBePatched)
        {
            //only patch for Both path; vISA path doesn't need this.
            int kernelCount = 0;
            int functionCount = 0;
            for (auto func : m_kernelsAndFunctions)
            {
                if (func->getIsKernel())
                {
                    m_cisaBinary->patchKernel(
                        kernelCount, func->getGenxBinarySize(), func->getGenxBinaryBuffer(),
                        m_platformInfo->encoding);
                    kernelCount++;
                } else {
                    // functions be treated as "mainFunctions" will have its own binary, will need to
                    // specify its binary buffer in m_cisaBinary
                    // FIXME: By this the external functions' gen-binary will be part of .isa output when
                    // calling CisaBinary::dumpToStream, and avoid the assert in dumpToStream. But when
                    // parsing the emited .isa file, our parser may not correctly support this case.
                    if (m_options.getOption(vISA_noStitchExternFunc) &&
                        func->getKernel()->getBoolKernelAttr(Attributes::ATTR_Extern)) {
                        m_cisaBinary->patchFunctionWithGenBinary(functionCount, func->getGenxBinarySize(),
                            func->getGenxBinaryBuffer());
                    } else {
                        m_cisaBinary->patchFunction(functionCount, func->getGenxBinarySize());
                    }
                    functionCount++;
                }
            }
        }

        if (os)
        {
            status = m_cisaBinary->dumpToStream(os);
        }
        else
        {
            if (CisaFramework::allowDump(m_options, name))
                status = m_cisaBinary->dumpToFile(name);
        }
    }

    stopTimer(TimerID::TOTAL); // have to record total time before dump the timer
    if (m_options.getOption(vISA_dumpTimer))
    {
        const char *asmName = nullptr;
        m_options.getOption(VISA_AsmFileName, asmName);
        dumpAllTimers(asmName, true);
    }

#ifndef DLL_MODE
    if (criticalMsg.str().length() > 0)
    {
        std::cerr << "[vISA Finalizer Messsages]\n" << criticalMsg.str();
    }
#endif //DLL_MODE

    if (m_options.getuInt32Option(vISA_CodePatch) && oldMainKernel)
    {
        m_kernelsAndFunctions.pop_back();
        m_kernelsAndFunctions.push_back(oldMainKernel);
    }

    return status;
}

int CISA_IR_Builder::verifyVISAIR()
{

#ifdef IS_RELEASE_DLL
    return VISA_SUCCESS;
#endif

    bool hasErrors = false;
    unsigned totalErrors = 0;
    std::string testName; // base kernel name saved for function's isaasm file name

    for (auto kTemp : m_kernelsAndFunctions)
    {
        if (kTemp->getIsKernel())
        {
            //if asmName is test9_genx_0.asm, the testName is test9_genx.
            std::string asmName = kTemp->getOutputAsmPath();
            std::string::size_type asmNameEnd = asmName.find_last_of('_');
            if (asmNameEnd != std::string::npos)
            {
                testName = asmName.substr(0, asmNameEnd);
            }
            else
            {
                testName = asmName;
            }
            break;
        }
    }

    std::vector<std::string> failedFiles;
    VISAKernelImpl* mainKernel = nullptr;
    for (auto kTemp : m_kernelsAndFunctions)
    {
        unsigned funcId = 0;
        if (kTemp->getIsKernel())
        {
            mainKernel = kTemp;
        }
        // Payload section is a mirror compilation to the main kernel
        // Load the main kernel to access its symbol table
        VISAKernelImpl* fmtKernel = kTemp->getIsPayload() ? mainKernel : kTemp;

        VISAKernel_format_provider fmt(fmtKernel);

        vISAVerifier verifier(m_header, &fmt, getOptions());
        verifier.run(kTemp);

        if (verifier.hasErrors())
        {
            std::stringstream verifierName;

            if (kTemp->getIsKernel())
            {
                verifierName << kTemp->getOutputAsmPath();
            }
            else
            {
                kTemp->GetFunctionId(funcId);
                verifierName << testName;
                verifierName << "_f";
                verifierName << funcId;
            }
            verifierName << ".errors.txt";
            verifier.writeReport(verifierName.str().c_str());
            failedFiles.push_back(verifierName.str());
            hasErrors = true;
            totalErrors += (uint32_t)verifier.getNumErrors();
        }
    }
    if (hasErrors)
    {
        std::stringstream ss;
        ss << "Found a total of " << totalErrors << " errors in vISA input.\n";
        ss << "Please check\n";
        for (auto&& name : failedFiles)
        {
            ss << "\t" << name << "\n";
        }
        ss << "for the exact error messages\n";
        criticalMsgStream() << ss.str();
        return VISA_FAILURE;
    }

    return VISA_SUCCESS;

}

bool CISA_IR_Builder::CISA_lookup_builtin_constant(int lineNum, const char *symbol, int64_t &val)
{
    std::string sym(symbol);
    if (sym == "%DispatchSimd") {
        if (m_dispatchSimdSize <= 0) {
            m_dispatchSimdSize = -1;
            RecordParseError(lineNum,
                "symbol cannot be used before .kernel_attr DispatchSimd=... is set");
            return false;
        }
        val = m_dispatchSimdSize;
        return true;
    } else {
        RecordParseError(lineNum, sym, ": invalid built-in symbol");
        val = -1;
        return false;
    }
}

bool CISA_IR_Builder::CISA_eval_sizeof_decl(int lineNum, const char *var, int64_t &val)
{
    auto *decl =  (VISA_GenVar*)m_kernel->getDeclFromName(var);
    if (!decl) {
        if (std::string(var) == "GRF") {
            val = m_kernel->getIRBuilder()->getGRFSize();
            return true;
        }
        RecordParseError(lineNum, var, ": unbound variable");
        return false;
    }
    switch (decl->type) {
    case GENERAL_VAR: val = (int64_t)decl->genVar.getSize(); break;
    case ADDRESS_VAR: val = (int64_t)decl->addrVar.num_elements * 2; break;
    default:
        RecordParseError(lineNum, var, ": unsupported operator on this variable kind");
        return false;
    }
    return true;
}

// Use in a function returning bool (returns false on failure)
// requires: int lineNum
//
// TODO: the long term goal is to have the vISA builder class store a
// "last error" of some sort and then we can just change this macro.
//
// Note: this is exactly what C++ exceptions are for.  This ugliness is the cost.
#define VISA_CALL_TO_BOOL(FUNC, ...) \
    do { \
        int __status = m_kernel->FUNC(__VA_ARGS__); \
        if (__status != VISA_SUCCESS) { \
            RecordParseError(lineNum, #FUNC, ": unknown error (internal line: ", __LINE__, ")"); \
            return false; \
        } \
    } while (0)
#define VISA_RESULT_CALL_TO_BOOL(FUNC_RESULT) \
    do { \
        int __status = FUNC_RESULT; \
        if (__status != VISA_SUCCESS) { \
            RecordParseError(lineNum, ""/*__FUNCTION__*/, ": unknown error (internal line: ", __LINE__, ")"); \
            return false; \
        } \
    } while (0)
// similar to above, but returns nullptr on failure.
#define VISA_CALL_TO_NULLPTR(FUNC, ...) \
    do { \
        int __status = m_kernel->FUNC(__VA_ARGS__); \
        if (__status != VISA_SUCCESS) { \
            RecordParseError(lineNum, #FUNC, ": unknown error (internal line: ", __LINE__, ")"); \
            return nullptr; \
        } \
    } while (0)

#define VISA_CALL_TO_BOOL_NOLINE(FUNC, ...) \
    do { \
        int lineNum = 0; \
        VISA_CALL_TO_BOOL(FUNC, __VA_ARGS__); \
    } while (0)


VISA_StateOpndHandle *CISA_IR_Builder::CISA_get_surface_variable(
    const char *varName, int lineNum)
{
    VISA_StateOpndHandle * surface = nullptr;
    VISA_SurfaceVar *surfaceVar = (VISA_SurfaceVar*)m_kernel->getDeclFromName(varName);
    if (!surfaceVar) {
        RecordParseError(lineNum, varName, ": undefined surface variable");
    } else if (surfaceVar->type != SURFACE_VAR && surfaceVar->type != SAMPLER_VAR) {
        RecordParseError(lineNum, varName, ": not a surface variable");
    } else {
        if (m_kernel->CreateVISAStateOperandHandle(surface, surfaceVar) != VISA_SUCCESS) {
            RecordParseError(lineNum, varName, ": internal error: creating surface variable");
            surface = nullptr;
        }
    }
    return surface;
}

VISA_StateOpndHandle *CISA_IR_Builder::CISA_get_sampler_variable(
    const char *varName, int lineNum)
{
    VISA_StateOpndHandle * surface = nullptr;
    VISA_SamplerVar *samplerVar = (VISA_SamplerVar*)m_kernel->getDeclFromName(varName);
    if (!samplerVar) {
        RecordParseError(lineNum, varName, ": undefined sampler variable");
    } else if (samplerVar->type != SURFACE_VAR && samplerVar->type != SAMPLER_VAR) {
        RecordParseError(lineNum, varName, ": not a sampler variable");
    } else {
        if (m_kernel->CreateVISAStateOperandHandle(surface, samplerVar) != VISA_SUCCESS) {
            RecordParseError(lineNum, varName, ": internal error: creating sampler variable");
            surface = nullptr;
        }
    }
    return surface;
}

bool CISA_IR_Builder::CISA_general_variable_decl(
    const char * var_name,
    unsigned int var_elemts_num,
    VISA_Type data_type,
    VISA_Align var_align,
    const char * var_alias_name,
    int var_alias_offset,
    std::vector<attr_gen_struct*>& scope,
    int lineNum)
{
    VISA_GenVar * genVar = NULL;

    VISA_GenVar *parentDecl = NULL;

    if (m_kernel->declExistsInCurrentScope(var_name)) {
        RecordParseError(lineNum, var_name, ": variable redeclaration");
        return false;
    }

    if (var_alias_name && strcmp(var_alias_name, "") != 0)
    {
        parentDecl = (VISA_GenVar *)m_kernel->getDeclFromName(var_alias_name);
        if (parentDecl == nullptr) {
            // alias=... points to a nonexistent variable
            RecordParseError(lineNum, var_alias_name, ": unbound alias referent");
            return false;
        }
    }

    m_kernel->CreateVISAGenVar(
        genVar, var_name, var_elemts_num, data_type, var_align,
        parentDecl, var_alias_offset);

    if (!addAllVarAttributes((CISA_GEN_VAR*)genVar, scope, lineNum))
    {
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_addr_variable_decl(
    const char *var_name, unsigned int var_elements,
    VISA_Type data_type, std::vector<attr_gen_struct *>& scope, int lineNum)
{
    if (m_kernel->declExistsInCurrentScope(var_name)) {
        RecordParseError(lineNum, var_name, ": variable redeclaration");
        return false;
    }

    VISA_AddrVar *decl = NULL;
    m_kernel->CreateVISAAddrVar(decl, var_name, var_elements);
    if (!addAllVarAttributes((CISA_GEN_VAR*)decl, scope, lineNum))
    {
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_predicate_variable_decl(
    const char *var_name, unsigned int var_elements, std::vector<attr_gen_struct*>& attrs, int lineNum)
{
    if (m_kernel->declExistsInCurrentScope(var_name)) {
        RecordParseError(lineNum, var_name, ": variable redeclaration");
        return false;
    }

    VISA_PredVar *decl = NULL;
    m_kernel->CreateVISAPredVar(decl, var_name, (unsigned short)var_elements);
    if (!addAllVarAttributes((CISA_GEN_VAR*)decl, attrs, lineNum))
    {
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_sampler_variable_decl(
    const char *var_name, int num_elts, const char* name, int lineNum)
{
    if (m_kernel->declExistsInCurrentScope(var_name)) {
        RecordParseError(lineNum, var_name, ": variable redeclaration");
        return false;
    }

    VISA_SamplerVar *decl = NULL;
    m_kernel->CreateVISASamplerVar(decl, var_name, num_elts);
    return true;
}

bool CISA_IR_Builder::CISA_surface_variable_decl(
    const char *var_name, int num_elts, const char* name,
    std::vector<attr_gen_struct*>& attrs, int lineNum)
{
    if (m_kernel->declExistsInCurrentScope(var_name)) {
        RecordParseError(lineNum, var_name, ": variable redeclaration");
        return false;
    }

    //int reg_id = attr_val.value;
    //char * value = (char *)m_mem.alloc(1);
    //*value = (char)reg_id;

    VISA_SurfaceVar *decl = NULL;
    m_kernel->CreateVISASurfaceVar(decl, var_name, num_elts);
    if (!addAllVarAttributes((CISA_GEN_VAR*)decl, attrs, lineNum))
    {
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_implicit_input_directive(
    const char * argName, const char *varName,
    short offset, unsigned short size, int lineNum)
{
    std::string implicitArgName = argName;
    auto pos = implicitArgName.find("UNDEFINED_");
    uint32_t numVal = 0;
    if (pos!= std::string::npos)
    {
        pos += strlen("UNDEFINED_");
        auto numValString = implicitArgName.substr(pos, implicitArgName.length());
        numVal = std::stoi(numValString);
    }
    else
    {
        auto implicitInputName = implicitArgName.substr(strlen(".implicit_"), implicitArgName.length());
        for (; numVal < IMPLICIT_INPUT_COUNT; ++numVal)
        {
            if (!implicitInputName.compare(input_info_t::getImplicitKindString(numVal)))
            {
                break;
            }
        }
    }

    int status = VISA_SUCCESS;
    CISA_GEN_VAR *temp = m_kernel->getDeclFromName(varName);
    if (!temp) {
        RecordParseError(lineNum, varName, ": undefined variable");
        return false;
    }
    status = m_kernel->CreateVISAImplicitInputVar((VISA_GenVar *)temp, offset, size, numVal);
    if (status != VISA_SUCCESS)
    {
        RecordParseError(lineNum, "failed to create input variable");
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_input_directive(
    const char* var_name, short offset, unsigned short size, int lineNum)
{
    int status = VISA_SUCCESS;
    CISA_GEN_VAR *var = m_kernel->getDeclFromName(var_name);
    if (var == nullptr) {
        RecordParseError(lineNum, var_name, ": unbound identifier");
        return false;
    }

    status = m_kernel->CreateVISAInputVar((VISA_GenVar *)var, offset, size);
    if (status != VISA_SUCCESS)
    {
        RecordParseError(lineNum, var_name, ": internal error: failed to create input variable");
        return false;
    }
    return true;
}

bool CISA_IR_Builder::CISA_attr_directive(
    const char* input_name, const char* input_var, int lineNum)
{
    Attributes::ID attrID = Attributes::getAttributeID(input_name);
    if (!m_options.getOption(vISA_AsmFileNameOverridden) &&
        attrID == Attributes::ATTR_OutputAsmPath)
    {
        if (strcmp(input_name, "AsmName") == 0) {
            RecordParseWarning(lineNum,
                "AsmName deprecated (replace with OutputAsmPath)");
        }
        input_name = "OutputAsmPath"; // normalize to new name

        char *asmFileName = strdup(input_var);
        char *pos = strstr(asmFileName, ".asm");
        if (pos != NULL)
        {
            *pos = '\0';
        }
        m_options.setOptionInternally(VISA_AsmFileName, asmFileName);
    }

    if (attrID == Attributes::ATTR_Target) {
        unsigned char visa_target;
        if (input_var == nullptr) {
            RecordParseError(lineNum,
                ".kernel_attr Target=.. must be \"cm\" or \"3d\"");
            return false;
        }
        if (strcmp(input_var, "cm") == 0) {
            visa_target = VISA_CM;
        } else if (strcmp(input_var, "3d") == 0) {
            visa_target = VISA_3D;
        } else {
            RecordParseError(lineNum, "invalid kernel target attribute");
            return false;
        }
        m_kernel->AddKernelAttribute(input_name, 1, &visa_target);
    }
    else
    {
        m_kernel->AddKernelAttribute(input_name,
          input_var == nullptr ? 0 : (int)strlen(input_var), input_var);
    }

    return true;
}

bool CISA_IR_Builder::CISA_attr_directiveNum(
    const char* input_name, uint32_t input_var, int lineNum)
{
    if (std::string(input_name) == "SimdSize" ||
        std::string(input_name) == "DispatchSimdSize")
    {
        m_dispatchSimdSize = (int)input_var;
    }
    VISA_CALL_TO_BOOL(AddKernelAttribute, input_name, sizeof(uint32_t), &input_var);
    return true;
}

bool CISA_IR_Builder::CISA_create_label(const char *label_name, int lineNum)
{
    VISA_LabelOpnd *opnd[1] = {nullptr};

    //when we print out ./function from isa we also print out label.
    //if we don't skip it during re-parsing then we will have duplicate labels
    if (!m_kernel->getLabelOperandFromFunctionName(std::string(label_name)))
    {
        opnd[0] = m_kernel->getLabelOpndFromLabelName(std::string(label_name));
        if (!opnd[0])
        {
            // forward jump
            VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[0], label_name, LABEL_BLOCK);
            if (!m_kernel->setLabelOpndNameMap(label_name, opnd[0], LABEL_BLOCK))
                return false;
        }
        VISA_CALL_TO_BOOL(AppendVISACFLabelInst, opnd[0]);
    }

    return true;
}


bool CISA_IR_Builder::CISA_function_directive(const char* func_name, int lineNum)
{
    VISA_LabelOpnd *opnd[1] = {nullptr};
    opnd[0] = m_kernel->getLabelOperandFromFunctionName(std::string(func_name));
    if (!opnd[0])
    {
        VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[0], func_name, LABEL_SUBROUTINE);
        if (!m_kernel->setLabelOpndNameMap(func_name, opnd[0], LABEL_SUBROUTINE))
            return false;
    }

    VISA_CALL_TO_BOOL(AppendVISACFLabelInst, opnd[0]);
    return true;
}


bool CISA_IR_Builder::CISA_create_arith_instruction(
    VISA_opnd * pred,
    ISA_Opcode opcode,
    bool  sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd * dst_cisa,
    VISA_opnd * src0_cisa,
    VISA_opnd * src1_cisa,
    VISA_opnd * src2_cisa,
    int lineNum)
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAArithmeticInst,
        opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst_cisa, (VISA_VectorOpnd *)src0_cisa,
        (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
    return true;
}

bool CISA_IR_Builder::CISA_create_arith_instruction2(
    VISA_opnd * pred,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd * dst_cisa,
    VISA_opnd * carry_borrow,
    VISA_opnd * src1_cisa,
    VISA_opnd * src2_cisa,
    int lineNum)
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISATwoDstArithmeticInst,
        opcode, (VISA_PredOpnd *)pred, emask, executionSize,
        (VISA_VectorOpnd *)dst_cisa, (VISA_VectorOpnd *)carry_borrow,
        (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
    return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    bool  sat,
    VISA_opnd *dst,
    VISA_opnd *src0,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADataMovementInst,
        opcode, (VISA_PredOpnd*) pred, sat, emask, executionSize, (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_mov_instruction(
    VISA_opnd* dst, CISA_GEN_VAR* src0, int lineNum)
{
    MUST_BE_TRUE1(src0 != NULL, lineNum, "The source operand of a move instruction was null");
    VISA_CALL_TO_BOOL(AppendVISAPredicateMove,
        (VISA_VectorOpnd*)dst, (VISA_PredVar*)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_movs_instruction(
    VISA_EMask_Ctrl emask,
    ISA_Opcode opcode,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADataMovementInst,
        ISA_MOVS, NULL, false, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_branch_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char *target_label,
    bool is_fccall,
    int lineNum)
{
    VISA_LabelOpnd * opnd[1];
    int i = 0;

    switch (opcode)
    {
    case ISA_CALL:
        {
            //need second path over instruction stream to
            //determine correct IDs since function directive might not have been
            //encountered yet
            opnd[i] = m_kernel->getLabelOperandFromFunctionName(std::string(target_label));

            VISA_Label_Kind lblKind = is_fccall ? LABEL_FC : LABEL_SUBROUTINE;
            if (!opnd[i])
            {
                VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, lblKind);
                if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], lblKind))
                    return false;
                opnd[i]->tag = ISA_SUBROUTINE;
            }
            VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
            VISA_CALL_TO_BOOL(AppendVISACFCallInst,
                (VISA_PredOpnd *)pred, emask, executionSize, opnd[i]);
            return true;
        }
    case ISA_JMP:
        {
            opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

            //forward jump label: create the label optimistically
            if (!opnd[i])
            {
                VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, LABEL_BLOCK);
                if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], LABEL_BLOCK))
                    return false;
            }

            VISA_CALL_TO_BOOL(AppendVISACFJmpInst, (VISA_PredOpnd *) pred, opnd[i]);
            return true;
        }
    case ISA_GOTO:
        {
            opnd[i] = m_kernel->getLabelOpndFromLabelName(std::string(target_label));

            //forward jump label: create the label optimistically
            if (!opnd[i])
            {
                VISA_CALL_TO_BOOL(CreateVISALabelVar, opnd[i], target_label, LABEL_BLOCK);
                if (!m_kernel->setLabelOpndNameMap(target_label, opnd[0], LABEL_BLOCK))
                    return false;
            }
            VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
            VISA_CALL_TO_BOOL(AppendVISACFGotoInst,
                (VISA_PredOpnd*)pred, emask, executionSize, opnd[i]);
            return true;
        }
    default:
        {
            MUST_BE_TRUE(0, "UNKNOWN Branch OP not supported.");
            return false;
        }
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    CISA_GEN_VAR *decl,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAComparisonInst,
        sub_op, emask, executionSize,
        (VISA_PredVar *)decl, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_cmp_instruction(
    VISA_Cond_Mod sub_op,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAComparisonInst,
        sub_op, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}


bool CISA_IR_Builder::CISA_create_media_instruction(
    ISA_Opcode opcode,
    MEDIA_LD_mod media_mod,
    int block_width,
    int block_height,
    unsigned int plane_ID,
    const char * surfaceName,
    VISA_opnd *xOffset,
    VISA_opnd *yOffset,
    VISA_opnd *raw_dst,
    int lineNum)
{
    unsigned char mod;
    mod = media_mod & 0x7;
    if (mod >= MEDIA_LD_Mod_NUM) {
        RecordParseError(lineNum, "ISA_MEDIA_LD uses illegal exec size");
        return false;
    }

    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (surface == nullptr)
        return false; // error already reported

    VISA_CALL_TO_BOOL(AppendVISASurfAccessMediaLoadStoreInst,
        opcode, media_mod, surface,
        (unsigned char)block_width, (unsigned char)block_height,
        (VISA_VectorOpnd *)xOffset, (VISA_VectorOpnd *)yOffset,
        (VISA_RawOpnd *)raw_dst, (CISA_PLANE_ID)plane_ID);

    return true;
}

/*
For both RET and FRET instructions
*/
bool CISA_IR_Builder::CISA_Create_Ret(
    VISA_opnd *pred_opnd,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned int exec_size,
    int lineNum)
{
    if (opcode == ISA_RET)
    {
        VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
        VISA_CALL_TO_BOOL(AppendVISACFRetInst,
            (VISA_PredOpnd *)pred_opnd, emask, executionSize);
    }
    else
    {
        VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
        VISA_CALL_TO_BOOL(AppendVISACFFunctionRetInst,
            (VISA_PredOpnd *)pred_opnd, emask, executionSize);
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_oword_instruction(
    ISA_Opcode opcode,
    bool media_mod,
    unsigned int size,
    const char *surfaceName,
    VISA_opnd *offset_opnd,
    VISA_opnd *raw_dst_src,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASurfAccessOwordLoadStoreInst,
        opcode, vISA_EMASK_M1, surface,
        Get_VISA_Oword_Num_From_Number(size),
        (VISA_VectorOpnd*)offset_opnd, (VISA_RawOpnd*)raw_dst_src);
    return true;
}

bool CISA_IR_Builder::CISA_create_svm_block_instruction(
    SVMSubOpcode  subopcode,
    unsigned      owords,
    bool          unaligned,
    VISA_opnd*    address,
    VISA_opnd*    srcDst,
    int           lineNum)
{
    switch (subopcode)
    {
    case SVM_BLOCK_LD:
        VISA_CALL_TO_BOOL(AppendVISASvmBlockLoadInst,
            Get_VISA_Oword_Num_From_Number(owords), unaligned,
            (VISA_VectorOpnd*)address, (VISA_RawOpnd*)srcDst);
        return true;
    case SVM_BLOCK_ST:
        VISA_CALL_TO_BOOL(AppendVISASvmBlockStoreInst,
            Get_VISA_Oword_Num_From_Number(owords), unaligned,
            (VISA_VectorOpnd*)address, (VISA_RawOpnd*)srcDst);
        return true;
    default:
        return false;
    }

    return false;
}

bool CISA_IR_Builder::CISA_create_svm_scatter_instruction(
    VISA_opnd*    pred,
    SVMSubOpcode  subopcode,
    VISA_EMask_Ctrl emask,
    unsigned      exec_size,
    unsigned      blockSize,
    unsigned      numBlocks,
    VISA_opnd*    addresses,
    VISA_opnd*    srcDst,
    int           lineNum)
{
    VISA_SVM_Block_Type blockType = valueToVISASVMBlockType(blockSize);
    VISA_SVM_Block_Num blockNum = valueToVISASVMBlockNum(numBlocks);
    switch (subopcode)
    {
    case SVM_SCATTER:
        VISA_CALL_TO_BOOL(AppendVISASvmScatterInst,
            (VISA_PredOpnd*)pred, emask, Get_VISA_Exec_Size_From_Raw_Size(exec_size),
            blockType, blockNum, (VISA_RawOpnd*)addresses, (VISA_RawOpnd*)srcDst);
        return true;
    case SVM_GATHER:
        VISA_CALL_TO_BOOL(AppendVISASvmGatherInst,
            (VISA_PredOpnd*)pred, emask, Get_VISA_Exec_Size_From_Raw_Size(exec_size),
            blockType, blockNum, (VISA_RawOpnd*)addresses, (VISA_RawOpnd*)srcDst);
        return true;
    default:
        return false;
    }


    return false;
}

bool
CISA_IR_Builder::CISA_create_svm_gather4_scaled(
    VISA_opnd               *pred,
    VISA_EMask_Ctrl         eMask,
    unsigned                execSize,
    ChannelMask             chMask,
    VISA_opnd               *address,
    VISA_opnd               *offsets,
    VISA_opnd               *dst,
    int                     lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASvmGather4ScaledInst,
        static_cast<VISA_PredOpnd *>(pred),
        eMask,
        Get_VISA_Exec_Size_From_Raw_Size(execSize),
        chMask.getAPI(),
        static_cast<VISA_VectorOpnd *>(address),
        static_cast<VISA_RawOpnd *>(offsets),
        static_cast<VISA_RawOpnd *>(dst));

    return true;
}

bool CISA_IR_Builder::CISA_create_svm_scatter4_scaled(
    VISA_opnd              *pred,
    VISA_EMask_Ctrl eMask,
    unsigned               execSize,
    ChannelMask            chMask,
    VISA_opnd              *address,
    VISA_opnd              *offsets,
    VISA_opnd              *src,
    int                    lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASvmScatter4ScaledInst,
        static_cast<VISA_PredOpnd *>(pred),
        eMask,
        Get_VISA_Exec_Size_From_Raw_Size(execSize),
        chMask.getAPI(),
        static_cast<VISA_VectorOpnd *>(address),
        static_cast<VISA_RawOpnd *>(offsets),
        static_cast<VISA_RawOpnd *>(src));

    return true;
}

bool CISA_IR_Builder::CISA_create_svm_atomic_instruction(
    VISA_opnd* pred,
    VISA_EMask_Ctrl emask,
    unsigned   exec_size,
    VISAAtomicOps op,
    unsigned short bitwidth,
    VISA_opnd* addresses,
    VISA_opnd* src0,
    VISA_opnd* src1,
    VISA_opnd* dst,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISASvmAtomicInst,
        (VISA_PredOpnd *)pred, emask, executionSize, op, bitwidth,
        (VISA_RawOpnd *)addresses, (VISA_RawOpnd *)src0, (VISA_RawOpnd *)src1,
        (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_address_instruction(ISA_Opcode opcode,
                                                      VISA_EMask_Ctrl emask,
                                                      unsigned exec_size,
                                                      VISA_opnd *dst,
                                                      VISA_opnd *src0,
                                                      VISA_opnd *src1,
                                                      int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAAddrAddInst,
        emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    bool sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *src2,
    VISA_opnd *src3,
    int lineNum)
{
    if (opcode != ISA_SHR &&
        opcode != ISA_SHL &&
        opcode != ISA_ASR)
    {
        if (sat) {
            RecordParseError(lineNum, "saturation is not supported on this op");
        }
        sat = false;
        // fallthrough
    }

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISALogicOrShiftInst,
        opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1,
        (VISA_VectorOpnd *)src2, (VISA_VectorOpnd *)src3);
    return true;
}

bool CISA_IR_Builder::CISA_create_logic_instruction(
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    CISA_GEN_VAR *dst,
    CISA_GEN_VAR *src0,
    CISA_GEN_VAR *src1,
    int lineNum)
{
    if (opcode != ISA_AND &&
        opcode != ISA_OR  &&
        opcode != ISA_NOT &&
        opcode != ISA_XOR)
    {
        RecordParseError(lineNum, "prediate variables are not supported for this op");
        return false;
    }
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    if (!dst) {
        RecordParseError(lineNum, "null dst in logic op");
    }
    if (!src0) {
        RecordParseError(lineNum, "null src0 in logic op");
    }
    if (opcode != ISA_NOT)
    {
        if (!src1) {
            RecordParseError(lineNum, "null src1 in logic op");
        }
    }
    VISA_CALL_TO_BOOL(AppendVISALogicOrShiftInst,
        opcode, emask, executionSize,
        (VISA_PredVar *)dst, (VISA_PredVar *)src0, (VISA_PredVar *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_math_instruction(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    bool  sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int lineNum)
{
    VISA_Exec_Size executionSize =  Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAArithmeticInst,
        opcode, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1, NULL);
    return true;
}

bool CISA_IR_Builder::CISA_create_setp_instruction(
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    CISA_GEN_VAR * dst,
    VISA_opnd *src0,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISASetP,
        emask, executionSize, (VISA_PredVar *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_sel_instruction(
    ISA_Opcode opcode,
    bool sat,
    VISA_opnd *pred,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADataMovementInst,
        opcode, (VISA_PredOpnd*)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_fminmax_instruction(
    bool minmax,
    ISA_Opcode opcode,
    bool sat,
    VISA_opnd *pred,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    VISA_opnd *src1,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAMinMaxInst,
        (minmax ? CISA_DM_FMAX : CISA_DM_FMIN), sat, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter_instruction(
    ISA_Opcode opcode,
    int elt_size,
    VISA_EMask_Ctrl emask,
    unsigned elemNum,
    bool modifier,
    const char *surfaceName,
    VISA_opnd *global_offset, //global_offset
    VISA_opnd *element_offset, //element_offset
    VISA_opnd *raw_dst_src, //dst/src
    int lineNum)
{
    // GATHER  0x39 (GATHER)  Elt_size   Is_modified Num_elts    Surface Global_Offset   Element_Offset  Dst
    // SCATTER 0x3A (SCATTER) Elt_size               Num_elts    Surface Global_Offset   Element_Offset  Src
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    if (elemNum != 16 && elemNum != 8 && elemNum != 1) {
        RecordParseError(lineNum,
            "unsupported number of elements for gather/scatter instruction.");
    }

    VISA_Exec_Size executionSize = EXEC_SIZE_16;
    if (elemNum == 16)
    {
        executionSize = EXEC_SIZE_16;
    }
    else if (elemNum == 8)
    {
        executionSize = EXEC_SIZE_8;
    }
    else if (elemNum == 1)
    {
        executionSize = EXEC_SIZE_1;
    }

    GATHER_SCATTER_ELEMENT_SIZE elementSize = GATHER_SCATTER_BYTE_UNDEF;
    if (elt_size == 1)
    {
        elementSize = GATHER_SCATTER_BYTE;
    }else if (elt_size == 2)
    {
        elementSize = GATHER_SCATTER_WORD;
    }else if (elt_size == 4)
    {
        elementSize = GATHER_SCATTER_DWORD;
    }

    VISA_CALL_TO_BOOL(AppendVISASurfAccessGatherScatterInst,
        opcode, emask, elementSize, executionSize, surface,
        (VISA_VectorOpnd *)global_offset, (VISA_RawOpnd *)element_offset,
        (VISA_RawOpnd *)raw_dst_src);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_typed_instruction(
    ISA_Opcode opcode,
    VISA_opnd *pred,
    ChannelMask ch_mask,
    VISA_EMask_Ctrl emask,
    unsigned execSize,
    const char* surfaceName,
    VISA_opnd *uOffset,
    VISA_opnd *vOffset,
    VISA_opnd *rOffset,
    VISA_opnd *lod,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(execSize);
    VISA_CALL_TO_BOOL(AppendVISASurfAccessGather4Scatter4TypedInst,
        opcode, (VISA_PredOpnd *)pred, ch_mask.getAPI(), emask, executionSize, surface,
        (VISA_RawOpnd *)uOffset, (VISA_RawOpnd *)vOffset, (VISA_RawOpnd *)rOffset,
        (VISA_RawOpnd *)lod, (VISA_RawOpnd*)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_scatter4_scaled_instruction(
    ISA_Opcode                opcode,
    VISA_opnd                 *pred,
    VISA_EMask_Ctrl           eMask,
    unsigned                  execSize,
    ChannelMask               chMask,
    const char                *surfaceName,
    VISA_opnd                 *globalOffset,
    VISA_opnd                 *offsets,
    VISA_opnd                 *dstSrc,
    int                       lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASurfAccessGather4Scatter4ScaledInst,
        opcode, static_cast<VISA_PredOpnd *>(pred),
        eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
        chMask.getAPI(),
        surface,
        static_cast<VISA_VectorOpnd *>(globalOffset),
        static_cast<VISA_RawOpnd *>(offsets),
        static_cast<VISA_RawOpnd *>(dstSrc));

    return true;
}

bool CISA_IR_Builder::CISA_create_scatter_scaled_instruction(
    ISA_Opcode             opcode,
    VISA_opnd              *pred,
    VISA_EMask_Ctrl        eMask,
    unsigned               execSize,
    unsigned               numBlocks,
    const char             *surfaceName,
    VISA_opnd              *globalOffset,
    VISA_opnd              *offsets,
    VISA_opnd              *dstSrc,
    int                    lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASurfAccessScatterScaledInst,
                opcode, static_cast<VISA_PredOpnd *>(pred),
                eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
                valueToVISASVMBlockNum(numBlocks),
                surface,
                static_cast<VISA_VectorOpnd *>(globalOffset),
                static_cast<VISA_RawOpnd *>(offsets),
                static_cast<VISA_RawOpnd *>(dstSrc));

    return true;
}

bool CISA_IR_Builder::CISA_create_sync_instruction(ISA_Opcode opcode, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
    return true;
}

bool CISA_IR_Builder::CISA_create_sbarrier_instruction(bool isSignal, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASplitBarrierInst, isSignal);
    return true;
}

bool CISA_IR_Builder::CISA_create_FILE_instruction(
    ISA_Opcode opcode, const char * file_name, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISAMiscFileInst, file_name);
    return true;
}

bool CISA_IR_Builder::CISA_create_LOC_instruction(
    ISA_Opcode opcode, unsigned int loc, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISAMiscLOC, loc);
    return true;
}

bool CISA_IR_Builder::CISA_create_invtri_inst(
    VISA_opnd *pred,
    ISA_Opcode opcode,
    bool  sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    int lineNum)
{
    int num_operands = 0;
    VISA_INST_Desc *inst_desc = NULL;
    VISA_opnd *opnd[4];
    inst_desc = &CISA_INST_table[opcode];
    VISA_Modifier mod = MODIFIER_NONE;

    if (sat)
        mod = MODIFIER_SAT;

    if (dst != NULL)
    {
        dst->_opnd.v_opnd.tag += mod<<3;
        opnd[num_operands] = dst;
        num_operands ++;
    }

    if (src0 != NULL)
    {
        opnd[num_operands] = src0;
        num_operands ++;
    }

    PredicateOpnd predOpnd = pred ? pred->convertToPred() : PredicateOpnd::getNullPred();
    CisaFramework::CisaInst * inst = new(m_mem)CisaFramework::CisaInst(m_mem);

    unsigned char size = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    size += emask << 4;
    inst->createCisaInstruction(opcode, size, 0, predOpnd, opnd, num_operands, inst_desc);
    m_kernel->addInstructionToEnd(inst);

    return true;
}

bool CISA_IR_Builder::CISA_create_dword_atomic_instruction(
    VISA_opnd *pred,
    VISAAtomicOps subOpc,
    bool is16Bit,
    VISA_EMask_Ctrl eMask,
    unsigned execSize,
    const char *surfaceName,
    VISA_opnd *offsets,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASurfAccessDwordAtomicInst,
        static_cast<VISA_PredOpnd *>(pred),
        subOpc,
        is16Bit,
        eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
        surface,
        static_cast<VISA_RawOpnd *>(offsets),
        static_cast<VISA_RawOpnd *>(src0),
        static_cast<VISA_RawOpnd *>(src1),
        static_cast<VISA_RawOpnd *>(dst));

    return true;
}

bool CISA_IR_Builder::CISA_create_typed_atomic_instruction(
    VISA_opnd *pred,
    VISAAtomicOps subOpc,
    bool is16Bit,
    VISA_EMask_Ctrl eMask,
    unsigned execSize,
    const char *surfaceName,
    VISA_opnd *u,
    VISA_opnd *v,
    VISA_opnd *r,
    VISA_opnd *lod,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISA3dTypedAtomic,
        subOpc,
        is16Bit,
        static_cast<VISA_PredOpnd *>(pred),
        eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
        surface,
        static_cast<VISA_RawOpnd *>(u),
        static_cast<VISA_RawOpnd *>(v),
        static_cast<VISA_RawOpnd *>(r),
        static_cast<VISA_RawOpnd *>(lod),
        static_cast<VISA_RawOpnd *>(src0),
        static_cast<VISA_RawOpnd *>(src1),
        static_cast<VISA_RawOpnd *>(dst));

    return true;
}

bool CISA_IR_Builder::CISA_create_avs_instruction(
    ChannelMask channel,
    const char* surfaceName,
    const char* samplerName,
    VISA_opnd *u_offset,
    VISA_opnd *v_offset,
    VISA_opnd *deltaU,
    VISA_opnd *deltaV,
    VISA_opnd *u2d,
    VISA_opnd *groupID,
    VISA_opnd *verticalBlockNumber,
    OutputFormatControl cntrl,
    VISA_opnd *v2d,
    AVSExecMode execMode,
    VISA_opnd *iefbypass,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_StateOpndHandle *sampler = CISA_get_sampler_variable(samplerName, lineNum);
    if (!sampler)
        return false; // error already reported

    VISA_CALL_TO_BOOL(AppendVISAMEAVS,
        surface, sampler, channel.getAPI(),
        (VISA_VectorOpnd *)u_offset, (VISA_VectorOpnd *)v_offset, (VISA_VectorOpnd *)deltaU,
        (VISA_VectorOpnd *)deltaV, (VISA_VectorOpnd *)u2d, (VISA_VectorOpnd *)v2d,
        (VISA_VectorOpnd *)groupID, (VISA_VectorOpnd *)verticalBlockNumber, cntrl,
        execMode, (VISA_VectorOpnd *)iefbypass, (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_urb_write_3d_instruction(
    VISA_opnd* pred,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    unsigned int num_out,
    unsigned int global_offset,
    VISA_opnd* channel_mask,
    VISA_opnd* urb_handle,
    VISA_opnd* per_slot_offset,
    VISA_opnd* vertex_data,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISA3dURBWrite,
        (VISA_PredOpnd*)pred, emask, executionSize, (unsigned char)num_out,
        (VISA_RawOpnd*) channel_mask, (unsigned short)global_offset,
        (VISA_RawOpnd*)urb_handle, (VISA_RawOpnd*)per_slot_offset, (VISA_RawOpnd*)vertex_data);
    return true;
}

bool CISA_IR_Builder::CISA_create_rtwrite_3d_instruction(
    VISA_opnd* pred,
    const char* mode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char* surfaceName,
    const std::vector<VISA_opnd*> &operands,
    int lineNum)
{
    vISA_RT_CONTROLS cntrls;

    memset(&cntrls, 0, sizeof(vISA_RT_CONTROLS));

    VISA_opnd* s0a              = NULL;
    VISA_opnd* oM               = NULL;
    VISA_opnd* R                = NULL;
    VISA_opnd* G                = NULL;
    VISA_opnd* B                = NULL;
    VISA_opnd* A                = NULL;
    VISA_opnd* Z                = NULL;
    VISA_opnd* Stencil          = NULL;
    VISA_opnd *CPSCounter =  NULL;
    VISA_opnd *SamplerIndex = NULL;
    VISA_opnd *r1Header = NULL;
    VISA_opnd *rti = NULL;
    uint8_t counter = 0;

    r1Header = operands[counter++];

    if (mode != NULL)
    {
        if (strstr(mode, "<SI>"))
        {
            SamplerIndex = operands[counter++];
        }

        if (strstr(mode, "<CPS>"))
        {
            CPSCounter = operands[counter++];
        }

        if (strstr(mode, "<RTI>"))
        {
            cntrls.RTIndexPresent = true;
            rti = operands[counter++];
        }

        if (strstr(mode, "<A>"))
        {
            cntrls.s0aPresent = true;
            s0a = operands[counter++];
        }

        if (strstr(mode, "<O>"))
        {
            cntrls.oMPresent = true;
            oM = operands[counter++];
        }
        R = operands[counter++];
        G = operands[counter++];
        B = operands[counter++];
        A = operands[counter++];

        if (strstr(mode, "<Z>"))
        {
            cntrls.zPresent = true;
            Z = operands[counter++];
        }

        if (strstr(mode, "<ST>"))
        {
            cntrls.isStencil = true;
            Stencil = operands[counter++];
        }

        if (strstr(mode, "<LRTW>"))
        {
            cntrls.isLastWrite = true;

        }

        if (strstr(mode, "<PS>"))
        {
            cntrls.isPerSample = true;
        }

        if (strstr(mode, "CM"))
        {
            cntrls.isCoarseMode = true;
        }

        if (strstr(mode, "NULLRT"))
        {
            cntrls.isNullRT = true;
        }
    }
    else
    {
        R = operands[counter++];
        G = operands[counter++];
        B = operands[counter++];
        A = operands[counter++];
    }

    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    uint8_t numMsgSpecificOpnd = 0;
    VISA_RawOpnd* rawOpnds[20];

#define APPEND_NON_NULL_RAW_OPND(opnd) \
    if (opnd != NULL)  \
    { \
    rawOpnds[numMsgSpecificOpnd++] = (VISA_RawOpnd*)opnd; \
    }

    APPEND_NON_NULL_RAW_OPND(s0a);
    APPEND_NON_NULL_RAW_OPND(oM);
    APPEND_NON_NULL_RAW_OPND(R);
    APPEND_NON_NULL_RAW_OPND(G);
    APPEND_NON_NULL_RAW_OPND(B);
    APPEND_NON_NULL_RAW_OPND(A);
    APPEND_NON_NULL_RAW_OPND(Z);
    APPEND_NON_NULL_RAW_OPND(Stencil);
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISA3dRTWriteCPS,
        (VISA_PredOpnd*)pred, emask, executionSize, (VISA_VectorOpnd*)rti,
        cntrls, surface, (VISA_RawOpnd*)r1Header, (VISA_VectorOpnd*)SamplerIndex,
        (VISA_VectorOpnd*)CPSCounter, numMsgSpecificOpnd, rawOpnds);

    return true;
}


bool CISA_IR_Builder::CISA_create_info_3d_instruction(
    VISASampler3DSubOpCode subOpcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    ChannelMask channel,
    const char* surfaceName,
    VISA_opnd* lod,
    VISA_opnd* dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISA3dInfo,
        subOpcode, emask, executionSize, channel.getAPI(), surface, (VISA_RawOpnd*)lod, (VISA_RawOpnd*)dst);
    return true;
}

bool CISA_IR_Builder::createSample4Instruction(
    VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    ChannelMask channel,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* aoffimmi,
    const char* samplerName,
    const char* surfaceName,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_StateOpndHandle *sampler = CISA_get_sampler_variable(samplerName, lineNum);
    if (!sampler)
        return false; // error already reported

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

    if (channel.getNumEnabledChannels() != 1) {
        RecordParseError(lineNum, "one one of R,G,B,A may be specified for sample4 instruction");
        return false;
    }
    int status = m_kernel->AppendVISA3dGather4(
        subOpcode, pixelNullMask, (VISA_PredOpnd*)pred, emask,
        executionSize, channel.getSingleChannel(), (VISA_VectorOpnd*) aoffimmi,
        sampler, surface,
        (VISA_RawOpnd*) dst, numParameters, params);
    VISA_RESULT_CALL_TO_BOOL(status);
    return true;
}


bool CISA_IR_Builder::create3DLoadInstruction(
    VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    ChannelMask channels,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *aoffimmi,
    const char* surfaceName,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    int status = m_kernel->AppendVISA3dLoad(
        subOpcode, pixelNullMask, (VISA_PredOpnd*)pred, emask,
        executionSize, channels.getAPI(), (VISA_VectorOpnd*) aoffimmi, surface,
        (VISA_RawOpnd*) dst, numParameters, params);
    VISA_RESULT_CALL_TO_BOOL(status);
    return true;
}

bool CISA_IR_Builder::create3DSampleInstruction(
    VISA_opnd* pred,
    VISASampler3DSubOpCode subOpcode,
    bool pixelNullMask,
    bool cpsEnable,
    bool uniformSampler,
    ChannelMask channels,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* aoffimmi,
    const char* samplerName,
    const char* surfaceName,
    VISA_opnd* dst,
    unsigned int numParameters,
    VISA_RawOpnd** params,
    int lineNum)
{
    VISA_StateOpndHandle *surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface) {
        return false; // error already reported
    }

    VISA_StateOpndHandle *sampler = CISA_get_sampler_variable(samplerName, lineNum);
    if (!sampler) {
        return false; // error already reported
    }

    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

    int status = m_kernel->AppendVISA3dSampler(
        subOpcode, pixelNullMask, cpsEnable, uniformSampler,
        (VISA_PredOpnd*)pred, emask, executionSize, channels.getAPI(),
        (VISA_VectorOpnd*)aoffimmi, sampler, surface,
        (VISA_RawOpnd*)dst, numParameters, params);
    VISA_RESULT_CALL_TO_BOOL(status);
    return true;
}

bool CISA_IR_Builder::CISA_create_sample_instruction(
    ISA_Opcode opcode,
    ChannelMask channel,
    int simd_mode,
    const char* samplerName,
    const char* surfaceName,
    VISA_opnd *u_opnd,
    VISA_opnd *v_opnd,
    VISA_opnd *r_opnd,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle* surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    if (opcode == ISA_SAMPLE)
    {
        VISA_StateOpndHandle* sampler = CISA_get_sampler_variable(samplerName, lineNum);
        if (!sampler)
            return false; // error recorded

        VISA_CALL_TO_BOOL(AppendVISASISample,
            vISA_EMASK_M1, surface, sampler, channel.getAPI(), simd_mode == 16,
            (VISA_RawOpnd*)u_opnd, (VISA_RawOpnd*)v_opnd, (VISA_RawOpnd*)r_opnd, (VISA_RawOpnd*)dst);

    } else if (opcode == ISA_LOAD) {
        VISA_CALL_TO_BOOL(AppendVISASILoad,
            surface, channel.getAPI(), simd_mode == 16,
            (VISA_RawOpnd*)u_opnd, (VISA_RawOpnd*)v_opnd,
            (VISA_RawOpnd*)r_opnd, (VISA_RawOpnd*)dst);
    } else {
        RecordParseError(lineNum, (int)opcode, ": unsupported sampler mnemonic");
        return false;
    }

    return true;
}

bool CISA_IR_Builder::CISA_create_sampleunorm_instruction(
    ISA_Opcode opcode,
    ChannelMask channel,
    CHANNEL_OUTPUT_FORMAT out,
    const char* samplerName,
    const char* surfaceName,
    VISA_opnd *src0,
    VISA_opnd *src1,
    VISA_opnd *src2,
    VISA_opnd *src3,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle* surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_StateOpndHandle* sampler = CISA_get_sampler_variable(samplerName, lineNum);
    if (!sampler)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISASISampleUnorm,
        surface, sampler, channel.getAPI(),
        (VISA_VectorOpnd *)src0, (VISA_VectorOpnd *)src1, (VISA_VectorOpnd *)src2,
        (VISA_VectorOpnd *)src3, (VISA_RawOpnd *)dst, out);

    return true;
}

bool CISA_IR_Builder::CISA_create_vme_ime_instruction(
    ISA_Opcode opcode,
    unsigned char stream_mode,
    unsigned char searchCtrl,
    VISA_opnd *input_opnd,
    VISA_opnd *ime_input_opnd,
    const char* surfaceName,
    VISA_opnd *ref0_opnd,
    VISA_opnd *ref1_opnd,
    VISA_opnd *costCenter_opnd,
    VISA_opnd *dst_opnd,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISAMiscVME_IME,
        surface, stream_mode, searchCtrl, (VISA_RawOpnd *)input_opnd,
        (VISA_RawOpnd *)ime_input_opnd, (VISA_RawOpnd *)ref0_opnd,
        (VISA_RawOpnd *)ref1_opnd, (VISA_RawOpnd *)costCenter_opnd,
        (VISA_RawOpnd *)dst_opnd);

    return true;
}

bool CISA_IR_Builder::CISA_create_vme_sic_instruction(
    ISA_Opcode opcode,
    VISA_opnd *input_opnd,
    VISA_opnd *sic_input_opnd,
    const char* surfaceName,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISAMiscVME_SIC,
        surface,
        (VISA_RawOpnd *)input_opnd,
        (VISA_RawOpnd *)sic_input_opnd,
        (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_vme_fbr_instruction(
    ISA_Opcode opcode,
    VISA_opnd *input_opnd,
    VISA_opnd *fbr_input_opnd,
    const char* surfaceName,
    VISA_opnd* fbrMbMode,
    VISA_opnd* fbrSubMbShape,
    VISA_opnd* fbrSubPredMode,
    VISA_opnd *dst,
    int lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    VISA_CALL_TO_BOOL(AppendVISAMiscVME_FBR,
        surface,
        (VISA_RawOpnd *)input_opnd, (VISA_RawOpnd *)fbr_input_opnd,
        (VISA_VectorOpnd *)fbrMbMode, (VISA_VectorOpnd *)fbrSubMbShape,
        (VISA_VectorOpnd *)fbrSubPredMode, (VISA_RawOpnd *)dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_NO_OPND_instruction(ISA_Opcode opcode, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
    return true;
}

bool CISA_IR_Builder::CISA_create_switch_instruction(
    ISA_Opcode opcode,
    unsigned exec_size,
    VISA_opnd *indexOpnd,
    const std::deque<const char*>& labels,
    int lineNum)
{
    int numLabels = (int) labels.size();
    std::vector<VISA_LabelOpnd*> jmpTargets(numLabels);
    for (int i = 0; i < numLabels; ++i)
    {
        auto labelOpnd = m_kernel->getLabelOpndFromLabelName(labels[i]);

        //forward jump label: create the label optimistically
        if (!labelOpnd)
        {
            VISA_CALL_TO_BOOL(CreateVISALabelVar, labelOpnd, labels[i], LABEL_BLOCK);
            if (!m_kernel->setLabelOpndNameMap(labels[i], labelOpnd, LABEL_BLOCK))
                return false;
        }
        jmpTargets[i] = labelOpnd;
    }

    VISA_CALL_TO_BOOL(AppendVISACFSwitchJMPInst,
        (VISA_VectorOpnd *)indexOpnd,
        (uint8_t) numLabels,
        jmpTargets.data());

    return true;
}

bool CISA_IR_Builder::CISA_create_fcall_instruction(
    VISA_opnd *pred_opnd,
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    const char* funcName,
    unsigned arg_size,
    unsigned return_size,
    int lineNum) //last index
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFFunctionCallInst,
        (VISA_PredOpnd *)pred_opnd,emask, executionSize, std::string(funcName),
        (unsigned char)arg_size, (unsigned char)return_size);
    return true;
}

bool CISA_IR_Builder::CISA_create_ifcall_instruction(VISA_opnd *pred_opnd,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd* funcAddr,
    unsigned arg_size,
    unsigned return_size,
    int lineNum) //last index
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISACFIndirectFuncCallInst,
        (VISA_PredOpnd *)pred_opnd, emask, executionSize,
        (VISA_VectorOpnd*) funcAddr, (uint8_t) arg_size, (uint8_t) return_size);
    return true;
}

bool CISA_IR_Builder::CISA_create_faddr_instruction(
    const char* sym_name, VISA_opnd* dst, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISACFSymbolInst, std::string(sym_name), (VISA_VectorOpnd*) dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_raw_send_instruction(
    ISA_Opcode opcode,
    unsigned char modifier,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *pred_opnd,
    unsigned int exMsgDesc,
    unsigned char srcSize,
    unsigned char dstSize,
    VISA_opnd *Desc,
    VISA_opnd *Src,
    VISA_opnd *Dst,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISAMiscRawSend,
        (VISA_PredOpnd *) pred_opnd, emask, executionSize, modifier, exMsgDesc, srcSize, dstSize,
        (VISA_VectorOpnd *)Desc, (VISA_RawOpnd *)Src, (VISA_RawOpnd *)Dst);
    return true;
}

bool CISA_IR_Builder::CISA_create_lifetime_inst(
    unsigned char startOrEnd, const char *src, int lineNum)
{
    // src is a string representation of variable.
    // Scan entire symbol table to find variable whose name
    // corresponds to src.
    CISA_GEN_VAR *cisaVar = m_kernel->getDeclFromName(src);
    if (!cisaVar) {
        RecordParseError(lineNum, "lifetime operand not found");
        return false;
    }

    VISA_opnd *var = NULL;
    if (cisaVar->type == GENERAL_VAR) {
        var = CISA_create_gen_src_operand(src, 0, 1, 0, 0, 0, MODIFIER_NONE, lineNum);
    }
    else if (cisaVar->type == ADDRESS_VAR) {
        var = CISA_set_address_operand(cisaVar, 0, 1, (startOrEnd == 0), lineNum);
    }
    else if (cisaVar->type == PREDICATE_VAR) {
        var = CISA_create_predicate_operand(cisaVar, PredState_NO_INVERSE, PRED_CTRL_NON, lineNum);
    } else {
        RecordParseError(lineNum, src, ": invalid variable type for lifetime");
        return false;
    }

    VISA_CALL_TO_BOOL(AppendVISALifetime,
        (VISAVarLifetime)startOrEnd, (VISA_VectorOpnd*)var);
    return true;
}

bool CISA_IR_Builder::CISA_create_raw_sends_instruction(
    ISA_Opcode opcode,
    unsigned char modifier,
    bool hasEOT,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *pred_opnd,
    VISA_opnd *exMsgDesc,
    unsigned char ffid,
    unsigned char src0Size,
    unsigned char src1Size,
    unsigned char dstSize,
    VISA_opnd *Desc,
    VISA_opnd *Src0,
    VISA_opnd *Src1,
    VISA_opnd *Dst,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);

    VISA_CALL_TO_BOOL(AppendVISAMiscRawSends,
        (VISA_PredOpnd *) pred_opnd, emask, executionSize, modifier, ffid,
        (VISA_VectorOpnd *)exMsgDesc, src0Size, src1Size, dstSize,
        (VISA_VectorOpnd *)Desc, (VISA_RawOpnd *)Src0, (VISA_RawOpnd *)Src1,
        (VISA_RawOpnd *)Dst, hasEOT);

    return true;
}
/*
Should be only called from CISA 2.4+
*/
bool CISA_IR_Builder::CISA_create_fence_instruction(ISA_Opcode opcode, unsigned char mode, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode, mode);
    return true;
}

bool CISA_IR_Builder::CISA_create_wait_instruction(VISA_opnd* mask, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISAWaitInst, (VISA_VectorOpnd*) mask);
    return true;
}


/*** CISA 3.0 and later ***/
bool CISA_IR_Builder::CISA_create_yield_instruction(ISA_Opcode opcode, int lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISASyncInst, opcode);
    return true;
}

VISA_opnd * CISA_IR_Builder::CISA_create_gen_src_operand(
    const char* var_name, short v_stride, short width, short h_stride,
    unsigned char row_offset, unsigned char col_offset, VISA_Modifier mod, int lineNum)
{
    auto *decl =  (VISA_GenVar*)m_kernel->getDeclFromName(var_name);
    if (!decl) {
        RecordParseError(lineNum, var_name, ": unbound identifier");
        return nullptr;
    } else if (decl->type != GENERAL_VAR) {
        RecordParseError(lineNum, var_name, ": not a general register variable");
        return nullptr;
    }

    VISA_VectorOpnd *cisa_opnd = nullptr;
    int status = m_kernel->CreateVISASrcOperand(cisa_opnd, decl, mod, v_stride, width, h_stride, row_offset, col_offset);
    if (status != VISA_SUCCESS)
        RecordParseError(lineNum, "unknown error creating src operand");
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_dst_general_operand(
    const char * var_name,
    unsigned char roff, unsigned char sroff,
    unsigned short hstride, int lineNum)
{
    auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
    if (!decl) {
        RecordParseError(lineNum, var_name, ": unbound identifier");
        return nullptr;
    } else if (decl->type != GENERAL_VAR) {
        RecordParseError(lineNum, var_name, ": not a general register variable");
        return nullptr;
    }

    VISA_VectorOpnd *cisa_opnd = nullptr;
    int status = m_kernel->CreateVISADstOperand(cisa_opnd, decl, hstride, roff, sroff);
    if (status != VISA_SUCCESS)
        RecordParseError(lineNum, "unknown error creating dst operand");
    return (VISA_opnd *)cisa_opnd;
}

attr_gen_struct* CISA_IR_Builder::CISA_Create_Attr(const char* AttrName, int64_t I64Val, const char* CStrVal)
{
    attr_gen_struct* newAttr = (attr_gen_struct*)m_mem.alloc(sizeof(attr_gen_struct));
    Attributes::ID aID = Attributes::getAttributeID(AttrName);
    MUST_BE_TRUE(Attributes::isValid(aID), "vISA: unknown attribute!");
    if (Attributes::isInt32(aID) || Attributes::isBool(aID))
    {
        newAttr->isInt = true;
        // No i64 attribute value yet
        newAttr->value = (int32_t)I64Val;
    }
    else if (Attributes::isCStr(aID))
    {
        newAttr->isInt = false;
        newAttr->string_val = CStrVal;
    }
    newAttr->name = AttrName;
    newAttr->attr_set = true;
    return newAttr;
}

VISA_opnd * CISA_IR_Builder::CISA_create_immed(uint64_t value, VISA_Type type, int lineNum)
{
    VISA_VectorOpnd *cisa_opnd = NULL;

    VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &value, type);
    if (type == ISA_TYPE_Q || type == ISA_TYPE_UQ)
    {
        cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.lval = value;
    }
    else
    {
        cisa_opnd->_opnd.v_opnd.opnd_val.const_opnd._val.ival = (uint32_t)value;
    }
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_float_immed(double value, VISA_Type type, int lineNum)
{
    VISA_VectorOpnd *cisa_opnd = nullptr;
    if (type == ISA_TYPE_F)
    {
        float temp = (float)value;
        VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &temp, type);
    }
    else
    {
        VISA_CALL_TO_NULLPTR(CreateVISAImmediate, cisa_opnd, &value, type);
    }

    return (VISA_opnd *)cisa_opnd;
}

CISA_GEN_VAR * CISA_IR_Builder::CISA_find_decl(const char *var_name)
{
    return m_kernel->getDeclFromName(var_name);
}

VISA_opnd * CISA_IR_Builder::CISA_set_address_operand(
    CISA_GEN_VAR * cisa_decl, unsigned char offset, short width, bool isDst, int lineNum)
{
    VISA_VectorOpnd *cisa_opnd = nullptr;
    VISA_CALL_TO_NULLPTR(CreateVISAAddressOperand,
        cisa_opnd, (VISA_AddrVar *)cisa_decl, offset, width, isDst);

    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_set_address_expression(
    CISA_GEN_VAR *cisa_decl, short offset, int lineNum)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    VISA_CALL_TO_NULLPTR(CreateVISAAddressOfOperand,
        cisa_opnd, (VISA_GenVar *)cisa_decl, offset);
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_indirect(
    CISA_GEN_VAR * cisa_decl, VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short vertical_stride, unsigned short width,
    unsigned short horizontal_stride, VISA_Type type, int lineNum)
{
    VISA_VectorOpnd *cisa_opnd = NULL;
    VISA_CALL_TO_NULLPTR(CreateVISAIndirectSrcOperand,
        cisa_opnd, (VISA_AddrVar*)cisa_decl, mod, col_offset,
        immedOffset, vertical_stride, width, horizontal_stride, type);
    return cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_indirect_dst(
    CISA_GEN_VAR * cisa_decl, VISA_Modifier mod, unsigned short row_offset,
    unsigned char col_offset, unsigned short immedOffset,
    unsigned short horizontal_stride, VISA_Type type, int lineNum)
{
    MUST_BE_TRUE(cisa_decl->type == ADDRESS_VAR, "predication variable type is wrong"); // grammar enforced
    VISA_VectorOpnd *cisa_opnd = nullptr;
    VISA_CALL_TO_NULLPTR(CreateVISAIndirectDstOperand,
        cisa_opnd, (VISA_AddrVar*)cisa_decl, col_offset, immedOffset, horizontal_stride, type);
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_state_operand(
    const char * var_name, unsigned char offset, int lineNum, bool isDst)
{
    CISA_GEN_VAR *decl = m_kernel->getDeclFromName(var_name);
    if (decl == nullptr) {
        RecordParseError(lineNum, var_name, ": undefined state operand");
        return nullptr;
    }

    VISA_VectorOpnd * cisa_opnd = nullptr;
    int status = VISA_SUCCESS;
    switch (decl->type)
    {
    case SURFACE_VAR:
        status = m_kernel->CreateVISAStateOperand(cisa_opnd, (VISA_SurfaceVar *)decl, offset, isDst);
        break;
    case SAMPLER_VAR:
        status = m_kernel->CreateVISAStateOperand(cisa_opnd, (VISA_SamplerVar *)decl, offset, isDst);
        break;
    default:
        RecordParseError(lineNum, var_name, ": invalid variable type for state operand");
        break;
    }

    if (status != VISA_SUCCESS) {
        RecordParseError(lineNum, "unknown error creating state operand");
    }

    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_predicate_operand(
    CISA_GEN_VAR *decl, VISA_PREDICATE_STATE state,
    VISA_PREDICATE_CONTROL control, int lineNum)
{
    MUST_BE_TRUE1(decl->type == PREDICATE_VAR, lineNum, "predication variable type is wrong"); // parser enforces type
    VISA_PredOpnd *cisa_opnd = nullptr;
    int status = m_kernel->CreateVISAPredicateOperand(cisa_opnd, (VISA_PredVar *)decl, state, control);
    MUST_BE_TRUE1((status == VISA_SUCCESS), lineNum, "Failed to create predicate operand.");
    if (status != VISA_SUCCESS) {
        RecordParseError(lineNum, "unknown error creating predicate operand");
    }
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_RAW_NULL_operand(int lineNum)
{
    VISA_RawOpnd *cisa_opnd = nullptr;
    int status = m_kernel->CreateVISANullRawOperand(cisa_opnd, true);
    MUST_BE_TRUE1(status == VISA_SUCCESS, lineNum, "Was not able to create NULL RAW operand.");
    if (status != VISA_SUCCESS) {
        RecordParseError(lineNum, "unknown error creating raw null operand");
    }
    return (VISA_opnd *)cisa_opnd;
}

VISA_opnd * CISA_IR_Builder::CISA_create_RAW_operand(
    const char * var_name, unsigned short offset, int lineNum)
{
    VISA_RawOpnd *cisa_opnd = NULL;
    auto *decl = (VISA_GenVar *)m_kernel->getDeclFromName(var_name);
    if (decl == nullptr) {
        RecordParseError(lineNum, var_name, ": undefined raw operand variable");
        return nullptr;
    }
    int status = m_kernel->CreateVISARawOperand(cisa_opnd, decl, offset);
    if (status != VISA_SUCCESS) {
        RecordParseError(lineNum, "unknown error creating raw operand");
    }
    return (VISA_opnd *)cisa_opnd; // delay the decision of src or dst until translate stage
}

void CISA_IR_Builder::CISA_push_decl_scope()
{
    m_kernel->pushIndexMapScopeLevel();
}
void CISA_IR_Builder::CISA_pop_decl_scope()
{
    m_kernel->popIndexMapScopeLevel();
}

unsigned short CISA_IR_Builder::get_hash_key(const char* str)
{
    const char *str_pt = str;
    unsigned short key=0;
    unsigned char c;
    while ((c = *str_pt++) != '\0') key = (key+c)<<1;

    return key % HASH_TABLE_SIZE;
}
string_pool_entry** CISA_IR_Builder::new_string_pool()
{
    string_pool_entry ** sp =
        (string_pool_entry**)m_mem.alloc(sizeof(string_pool_entry *) * HASH_TABLE_SIZE);
    memset(sp, 0, sizeof(string_pool_entry *) * HASH_TABLE_SIZE);

    return sp;
}

string_pool_entry * CISA_IR_Builder::string_pool_lookup(
    string_pool_entry **spool, const char *str)
{
    unsigned short key = 0;
    string_pool_entry* entry;
    char *s;

    key = get_hash_key(str);

    for (entry = spool[key]; entry != NULL; entry = entry->next) {
        s = (char *)entry->value;
        if (!strcmp(s, str))
            return entry;
    }

    return NULL;
}

bool CISA_IR_Builder::addAllVarAttributes(
    CISA_GEN_VAR* GenVar, std::vector<attr_gen_struct*>& Attrs, int lineNum)
{
    if (Attrs.size() > 0)
    {
        (void)m_kernel->resizeAttribute(GenVar, (uint32_t)Attrs.size());
    }

    for (int i = 0, e = (int)Attrs.size(); i < e; ++i)
    {
        attr_gen_struct* pAttr = Attrs[i];
        Attributes::ID aID = Attributes::getAttributeID(pAttr->name);
        if (Attributes::isBool(aID))
        {
            m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, 0, nullptr);
        }
        else if (Attributes::isInt32(aID))
        {
            m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, 4, &pAttr->value);
        }
        else if (Attributes::isCStr(aID))
        {
            unsigned int sz = (unsigned)strlen(pAttr->string_val);
            m_kernel->AddAttributeToVarGeneric(GenVar, pAttr->name, sz, &pAttr->string_val);
        }
        else
        {
            RecordParseError(lineNum, pAttr->name, ": unknown attribute");
            return false;
        }
    }
    return true;
}

bool CISA_IR_Builder::string_pool_lookup_and_insert(
    string_pool_entry **spool,
    const char *str,
    Common_ISA_Var_Class type,
    VISA_Type data_type)
{
    unsigned short key = 0;
    string_pool_entry* entry;
    char *s;
    int len = (int) strlen(str);

    key = get_hash_key(str);

    for (entry = spool[key]; entry != NULL; entry = entry->next) {
        s = (char *)entry->value;
        if (!strcmp(s, str))
            return false;
    }

    s = (char*)m_mem.alloc(len + 1);
    memcpy_s(s, len + 1, str, len+1);
    s[len] = '\0';

    entry = (string_pool_entry*)m_mem.alloc(sizeof(string_pool_entry));
    memset(entry, 0, sizeof(*entry));
    entry->value = s;
    entry->type = type;
    entry->data_type = data_type;

    entry->next = spool[key];
    spool[key] = entry;

    return true;
}

Common_ISA_Input_Class CISA_IR_Builder::get_input_class(Common_ISA_Var_Class var_class)
{
    if (var_class == GENERAL_VAR)
        return INPUT_GENERAL;

    if (var_class == SAMPLER_VAR)
        return INPUT_SAMPLER;

    if (var_class == SURFACE_VAR)
        return INPUT_SURFACE;

    return INPUT_UNKNOWN;
}
void CISA_IR_Builder::CISA_post_file_parse()
{
    return;
}

// place it here so that internal Gen_IR files don't have to include VISAKernel.h
std::stringstream& IR_Builder::criticalMsgStream()
{
    return const_cast<CISA_IR_Builder*>(parentBuilder)->criticalMsgStream();
}

bool CISA_IR_Builder::CISA_create_dpas_instruction(
    ISA_Opcode opcode,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd * dst_cisa,
    VISA_opnd * src0_cisa,
    VISA_opnd * src1_cisa,
    VISA_opnd * src2_cisa,
    GenPrecision  A,
    GenPrecision  W,
    uint8_t D,
    uint8_t C,
    int lineNum)
{
    // rcount !
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADpasInst,
        opcode, emask, executionSize,
        (VISA_RawOpnd *)dst_cisa, (VISA_RawOpnd *)src0_cisa,
        (VISA_RawOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa,
        A, W, D, C);
    return true;
}


bool CISA_IR_Builder::CISA_create_bfn_instruction(
    VISA_opnd * pred,
    uint8_t func_ctrl,
    bool  sat,
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd * dst_cisa,
    VISA_opnd * src0_cisa,
    VISA_opnd * src1_cisa,
    VISA_opnd * src2_cisa,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISABfnInst,
        func_ctrl, (VISA_PredOpnd *)pred, sat, emask, executionSize,
        (VISA_VectorOpnd *)dst_cisa, (VISA_VectorOpnd *)src0_cisa,
        (VISA_VectorOpnd *)src1_cisa, (VISA_VectorOpnd *)src2_cisa);
    return true;
}

bool CISA_IR_Builder::CISA_create_qword_scatter_instruction(
    ISA_Opcode             opcode,
    VISA_opnd             *pred,
    VISA_EMask_Ctrl        eMask,
    unsigned               execSize,
    unsigned               numBlocks,
    const char            *surfaceName,
    VISA_opnd             *offsets,
    VISA_opnd             *dstSrc,
    int                    lineNum)
{
    VISA_StateOpndHandle * surface = CISA_get_surface_variable(surfaceName, lineNum);
    if (!surface)
        return false; // error recorded

    if (opcode == ISA_QW_GATHER)
    {
        VISA_CALL_TO_BOOL(AppendVISAQwordGatherInst,
            static_cast<VISA_PredOpnd *>(pred),
            eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
            valueToVISASVMBlockNum(numBlocks),
            surface,
            static_cast<VISA_RawOpnd *>(offsets),
            static_cast<VISA_RawOpnd *>(dstSrc));
    }
    else
    {
        VISA_CALL_TO_BOOL(AppendVISAQwordScatterInst,
            static_cast<VISA_PredOpnd *>(pred),
            eMask, Get_VISA_Exec_Size_From_Raw_Size(execSize),
            valueToVISASVMBlockNum(numBlocks),
            surface,
            static_cast<VISA_RawOpnd *>(offsets),
            static_cast<VISA_RawOpnd *>(dstSrc));
    }
    return true;
}

bool CISA_IR_Builder::CISA_create_bf_cvt_instruction(
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADataMovementInst,
        ISA_BF_CVT, nullptr, false, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_fcvt_instruction(
    VISA_EMask_Ctrl emask,
    unsigned exec_size,
    VISA_opnd *dst,
    VISA_opnd *src0,
    int lineNum)
{
    VISA_Exec_Size executionSize = Get_VISA_Exec_Size_From_Raw_Size(exec_size);
    VISA_CALL_TO_BOOL(AppendVISADataMovementInst,
        ISA_FCVT, nullptr, false, emask, executionSize,
        (VISA_VectorOpnd *)dst, (VISA_VectorOpnd *)src0);
    return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_inst(
    VISA_opnd                *pred,
    LSC_OP                    opcode,
    LSC_SFID                  sfid,
    LSC_CACHE_OPTS            caching,
    VISA_Exec_Size            execSize,
    VISA_EMask_Ctrl           emask,
    LSC_ADDR                  addr,
    LSC_DATA_SHAPE            dataShape,
    VISA_opnd                *surface,
    VISA_opnd                *dstData,
    VISA_opnd                *src0Addr,
    VISA_opnd                *src1Data,
    VISA_opnd                *src2Data,
    int                       lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISALscUntypedInst,
        opcode,
        sfid,
        static_cast<VISA_PredOpnd *>(pred),
        execSize,
        emask,
        caching,
        addr,
        dataShape,
        static_cast<VISA_VectorOpnd *>(surface),
        static_cast<VISA_RawOpnd *>(dstData),
        static_cast<VISA_RawOpnd *>(src0Addr),
        static_cast<VISA_RawOpnd *>(src1Data),
        static_cast<VISA_RawOpnd *>(src2Data));
    return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_strided_inst(
    VISA_opnd                *pred,
    LSC_OP                    opcode,
    LSC_SFID                  sfid,
    LSC_CACHE_OPTS            caching,
    VISA_Exec_Size            execSize,
    VISA_EMask_Ctrl           emask,
    LSC_ADDR                  addr,
    LSC_DATA_SHAPE            dataShape,
    VISA_opnd                *surface,
    VISA_opnd                *dst,
    VISA_opnd                *src0Base,
    VISA_opnd                *src0Stride,
    VISA_opnd                *src1Data,
    int                       lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISALscUntypedStridedInst,
        opcode,
        sfid,
        static_cast<VISA_PredOpnd *>(pred),
        execSize,
        emask,
        caching,
        addr,
        dataShape,
        static_cast<VISA_VectorOpnd *>(surface),
        static_cast<VISA_RawOpnd *>(dst),
        static_cast<VISA_RawOpnd *>(src0Base),
        static_cast<VISA_VectorOpnd *>(src0Stride),
        static_cast<VISA_RawOpnd *>(src1Data));
    return true;
}

bool CISA_IR_Builder::CISA_create_lsc_untyped_block2d_inst(
    VISA_opnd               *pred,
    LSC_OP                   opcode,
    LSC_SFID                 sfid,
    LSC_CACHE_OPTS           caching,
    VISA_Exec_Size           execSize,
    VISA_EMask_Ctrl          emask,
    LSC_DATA_SHAPE_BLOCK2D   dataShape2d,
    VISA_opnd               *dstData,
    VISA_opnd               *src0AddrsOps[LSC_BLOCK2D_ADDR_PARAMS],
    VISA_opnd               *src1Data,
    int                      lineNum)
{
    VISA_VectorOpnd *src0Addrs[LSC_BLOCK2D_ADDR_PARAMS] {
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[0]),
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[1]),
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[2]),
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[3]),
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[4]),
        static_cast<VISA_VectorOpnd *>(src0AddrsOps[5]),
    };
    VISA_CALL_TO_BOOL(AppendVISALscUntypedBlock2DInst,
        opcode,
        sfid,
        static_cast<VISA_PredOpnd *>(pred),
        execSize,
        emask,
        caching,
        dataShape2d,
        static_cast<VISA_RawOpnd *>(dstData),
        src0Addrs,
        static_cast<VISA_RawOpnd *>(src1Data));
    return true;
}

bool CISA_IR_Builder::CISA_create_lsc_typed_inst(
        VISA_opnd               *pred,
        LSC_OP                   opcode,
        LSC_SFID                 sfid,
        LSC_CACHE_OPTS           caching,
        VISA_Exec_Size           execSize,
        VISA_EMask_Ctrl          emask,
        LSC_ADDR_TYPE            addrModel,
        LSC_ADDR_SIZE            addrSize,
        LSC_DATA_SHAPE           dataShape,
        VISA_opnd               *surface,
        VISA_opnd               *dst_data,
        VISA_opnd               *src0_Us,
        VISA_opnd               *src0_Vs,
        VISA_opnd               *src0_Rs,
        VISA_opnd               *src0_LODs,
        VISA_opnd               *src1_data,
        VISA_opnd               *src2_data,
        int                      lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISALscTypedInst,
        opcode,
        static_cast<VISA_PredOpnd *>(pred),
        execSize,
        emask,
        caching,
        addrModel,
        addrSize,
        dataShape,
        static_cast<VISA_VectorOpnd *>(surface),
        static_cast<VISA_RawOpnd *>(dst_data),
        static_cast<VISA_RawOpnd *>(src0_Us),
        static_cast<VISA_RawOpnd *>(src0_Vs),
        static_cast<VISA_RawOpnd *>(src0_Rs),
        static_cast<VISA_RawOpnd *>(src0_LODs),
        static_cast<VISA_RawOpnd *>(src1_data),
        static_cast<VISA_RawOpnd *>(src2_data));
    return true;
}

bool CISA_IR_Builder::CISA_create_lsc_fence(
    LSC_SFID                 sfid,
    LSC_FENCE_OP             fence,
    LSC_SCOPE                scope,
    int                      lineNum)
{
    VISA_CALL_TO_BOOL(AppendVISALscFence,
        sfid, fence, scope);

    return true;
}

bool CISA_IR_Builder::CISA_create_nbarrier(
    bool isWait,
    VISA_opnd *barrierId,
    VISA_opnd *threadCount,
    int lineNum)
{
    if (isWait) {
        VISA_CALL_TO_BOOL(AppendVISANamedBarrierWait,
            static_cast<VISA_VectorOpnd*>(barrierId));
    } else {
        VISA_CALL_TO_BOOL(AppendVISANamedBarrierSignal,
            static_cast<VISA_VectorOpnd*>(barrierId),
            static_cast<VISA_VectorOpnd*>(threadCount));
    }
    return true;
}


const VISAKernelImpl* CISA_IR_Builder::getKernel(const std::string& name) const
{
    auto it = m_nameToKernel.find(name);
    if (it == m_nameToKernel.end())
        return nullptr;
    return it->second;
}
