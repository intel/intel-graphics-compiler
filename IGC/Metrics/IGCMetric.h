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

#pragma once

namespace IGC
{
    class DebugInfoData;
    class DbgDecoder;
}

namespace IGCMetrics
{
    class IGCMetric
    {
    private:
        void* igcMetric;
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

        void CollectLoopCyclomaticComplexity(
            llvm::Function* pFunc,
            int LoopCyclomaticComplexity,
            int LoopCyclomaticComplexity_Max);

        void CollectNestedLoopsWithMultipleExits(
            llvm::Function* pFunc,
            float NestedLoopsWithMultipleExitsRatio,
            float NestedLoopsWithMultipleExitsRatio_Max);

        void CollectLongStridedLdStInLoop(
            llvm::Function* pFunc,
            llvm::Loop* pProblematicLoop,
            int LongStridedLdStInLoop_LdCnt,
            int LongStridedLdStInLoop_StCnt,
            int LongStridedLdStInLoop_MaxCntLdOrSt);

        void CollectIsGeminiLakeWithDoubles(
            llvm::Function* pFunc,
            bool IsGeminiLakeWithDoubles);

        void CollectDataFromDebugInfo(IGC::DebugInfoData* pDebugInfo, IGC::DbgDecoder* pDebugDecoder);

        void FinalizeStats();

        void OutputMetrics();
    };
}
