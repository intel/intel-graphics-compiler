/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

#include <3d/common/iStdLib/types.h>
#include <common/shaderHash.hpp>
#include "KernelInfo.h"

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
#include <google/protobuf/util/json_util.h>
#include <Metrics/proto_schema/igc_metrics.pb.h>
#include <Metrics/proto_schema/instruction_stats.pb.h>
#endif // IGC_METRICS

#pragma once

namespace IGCMetrics
{
    class IGCMetric
    {
    private:
        bool isEnabled;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Program oclProgram;

        // Helpers
        // Map all instruction debuginfo to line number
        std::map<int, llvm::DILocation*> map_Instr;
        // Map Function debuginfo to Function metrics
        std::map<llvm::DIScope*, IGC_METRICS::Function*> map_Func;
        // helpers for emulated calls
        std::map<llvm::DILocation*, IGC_METRICS::FuncEmuCalls*> map_EmuCalls;
        // helpers for loops
        std::map<llvm::DILocalScope*, IGC_METRICS::CFGStats_Loops*> map_Loops;
        // Current count of instruction in function
        int countInstInFunc;

        int CountInstInFunc(llvm::Function* pFunc);

        void GetFunctionCalls(IGC_METRICS::Function* func_m, llvm::Function& func);

        inline IGC_METRICS::Function* GetFuncMetric(llvm::Instruction* pInstr);
        inline IGC_METRICS::Function* GetFuncMetric(llvm::Loop* pLoop);

        void CollectInstructions(llvm::Module* pModule);

        void UpdateLoopsInfo();
        void CollectLoop(llvm::Loop* loop);
#endif
    public:
        IGCMetric();
        ~IGCMetric();
        bool Enable();
        void Init(ShaderHash* Hash, bool isDebugInfo);

        void CollectLoops(llvm::LoopInfo* loopInfo);
        void CollectLoops(llvm::Loop* loop);

        void CollectFunctions(llvm::Module* pModule);

        void StatBeginEmuFunc(llvm::Instruction* instruction);
        void StatEndEmuFunc(llvm::Instruction* emulatedInstruction);

        void StatIncCoalesced(llvm::Instruction* coalescedAccess);

        void CollectRegStats(KERNEL_INFO* vISAstats);

        void FinalizeStats();

        void OutputMetrics();
    };
}