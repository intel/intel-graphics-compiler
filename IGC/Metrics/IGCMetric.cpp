/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCMetric.h"
#include "IGCMetricImpl.h"

class VISAModule;

namespace IGCMetrics
{
    inline IGCMetricImpl* get(void* pIGCMetric)
    {
        return (IGCMetricImpl*)(pIGCMetric);
    }

    IGCMetric::IGCMetric()
    {
        this->igcMetric = (void*)new IGCMetricImpl();
    }
    IGCMetric::~IGCMetric()
    {
        delete get(igcMetric);
    }
    bool IGCMetric::Enable()
    {
        return get(igcMetric)->Enable();
    }

    void IGCMetric::Init(ShaderHash* Hash, bool isEnabled)
    {
        get(igcMetric)->Init(Hash, isEnabled);
    }

    void IGCMetric::OutputMetrics()
    {
        get(igcMetric)->OutputMetrics();
    }

    void IGCMetric::StatBeginEmuFunc(llvm::Instruction* instruction)
    {
        get(igcMetric)->StatBeginEmuFunc(instruction);
    }

    void IGCMetric::StatEndEmuFunc(llvm::Instruction* emulatedInstruction)
    {
        get(igcMetric)->StatEndEmuFunc(emulatedInstruction);
    }

    void IGCMetric::StatIncCoalesced(llvm::Instruction* coalescedAccess)
    {
        get(igcMetric)->StatIncCoalesced(coalescedAccess);
    }

    void IGCMetric::CollectRegStats(KERNEL_INFO* kernelInfo)
    {
        get(igcMetric)->CollectRegStats(kernelInfo);
    }

    void IGCMetric::CollectFunctions(llvm::Module* pModule)
    {
        get(igcMetric)->CollectFunctions(pModule);
    }

    void IGCMetric::CollectLoops(llvm::Loop* loop)
    {
        get(igcMetric)->CollectLoops(loop);
    }

    void IGCMetric::CollectLoops(llvm::LoopInfo* loopInfo)
    {
        get(igcMetric)->CollectLoops(loopInfo);
    }

    void IGCMetric::CollectLoopCyclomaticComplexity(
        llvm::Function* pFunc,
        int LoopCyclomaticComplexity,
        int LoopCyclomaticComplexity_Max)
    {
        get(igcMetric)->CollectLoopCyclomaticComplexity(
            pFunc,
            LoopCyclomaticComplexity,
            LoopCyclomaticComplexity_Max);
    }

    void IGCMetric::CollectNestedLoopsWithMultipleExits(
        llvm::Function* pFunc,
        float NestedLoopsWithMultipleExitsRatio,
        float NestedLoopsWithMultipleExitsRatio_Max)
    {
        get(igcMetric)->CollectNestedLoopsWithMultipleExits(
            pFunc,
            NestedLoopsWithMultipleExitsRatio,
            NestedLoopsWithMultipleExitsRatio_Max);
    }

    void IGCMetric::CollectLongStridedLdStInLoop(
        llvm::Function* pFunc,
        llvm::Loop* pProblematicLoop,
        int LongStridedLdStInLoop_LdCnt,
        int LongStridedLdStInLoop_StCnt,
        int LongStridedLdStInLoop_MaxCntLdOrSt)
    {
        get(igcMetric)->CollectLongStridedLdStInLoop(
            pFunc,
            pProblematicLoop,
            LongStridedLdStInLoop_LdCnt,
            LongStridedLdStInLoop_StCnt,
            LongStridedLdStInLoop_MaxCntLdOrSt);
    }

    void IGCMetric::CollectIsGeminiLakeWithDoubles(
        llvm::Function* pFunc,
        bool IsGeminiLakeWithDoubles)
    {
        get(igcMetric)->CollectIsGeminiLakeWithDoubles(pFunc, IsGeminiLakeWithDoubles);
    }

    void IGCMetric::FinalizeStats()
    {
        get(igcMetric)->FinalizeStats();
    }

    void IGCMetric::CollectDataFromDebugInfo(IGC::DebugInfoData* pDebugInfo, IGC::DbgDecoder* pDebugDecoder)
    {
        get(igcMetric)->CollectDataFromDebugInfo(pDebugInfo, pDebugDecoder);
    }
}