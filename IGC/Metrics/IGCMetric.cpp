/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCMetric.h"
#include "IGCMetricImpl.h"

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

    void IGCMetric::FinalizeStats()
    {
        get(igcMetric)->FinalizeStats();
    }
}