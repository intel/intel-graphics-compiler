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

#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
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
    const char* const funcMetrics = "llvm.igc.metric";

    class IGCMetric
    {
    private:
        void* igcMetric;
    public:
        IGCMetric();
        ~IGCMetric();
        bool Enable();
        void Init(ShaderHash* Hash, bool isDebugInfo);

        size_t getMetricDataSize();
        const void* const getMetricData();

        void CollectLoops(llvm::LoopInfo* loopInfo);
        void CollectLoops(llvm::Loop* loop);

        void CollectFunctions(llvm::Module* pModule);

        void StatBeginEmuFunc(llvm::Instruction* instruction);
        void StatEndEmuFunc(llvm::Instruction* emulatedInstruction);

        void StatIncCoalesced(llvm::Instruction* coalescedAccess);

        void CollectRegStats(KERNEL_INFO* vISAstats, llvm::Function* pFunc);

        void CollectMem2Reg(llvm::AllocaInst* pAllocaInst, IGC::StatusPrivArr2Reg status);

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

        void CollectInstructionCnt(
            llvm::Function* pFunc,
            int InstCnt,
            int InstCntMax);

        void CollectThreadGroupSize(
            llvm::Function* pFunc,
            int ThreadGroupSize,
            int ThreadGroupSizeMax);

        void CollectThreadGroupSizeHint(
            llvm::Function* pFunc,
            int ThreadGroupSizeHint,
            int ThreadGroupSizeHintMax);

        void CollectIsSubGroupFuncIn(llvm::Function* pFunc, bool flag);

        void CollectGen9Gen10WithIEEESqrtDivFunc(llvm::Function* pFunc, bool flag);

        void CollectNonUniformLoop(llvm::Function* pFunc, short LoopCount, llvm::Loop* problematicLoop);

        void CollectDataFromDebugInfo(IGC::DebugInfoData* pDebugInfo, IGC::DbgDecoder* pDebugDecoder);

        void FinalizeStats();

        void OutputMetrics();

        static bool isMetricFuncCall(llvm::CallInst* pCallInst);
    };
}
