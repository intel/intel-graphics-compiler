/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

#include <3d/common/iStdLib/types.h>
#include <common/shaderHash.hpp>
#include "KernelInfo.h"
#include "IGCMetric.h"

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
#include <google/protobuf/util/json_util.h>
#include <Metrics/proto_schema/igc_metrics.pb.h>
#include <Metrics/proto_schema/instruction_stats.pb.h>

#endif // IGC_METRICS

#include "Compiler/CISACodeGen/CVariable.hpp"
#include <Compiler/CISACodeGen/DebugInfoData.hpp>
#include <DebugInfo/VISADebugDecoder.hpp>

#pragma once

namespace IGCMetrics
{
    const char* const funcTrackValue = "llvm.igc.metric.trackValue";

    class IGCMetricImpl
    {
    private:
        friend class CollectDataMetrics;

        bool isEnabled;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Program oclProgram;
        IGCLLVM::Module* pModule;
        // On end of collecting metrics it will contain binary format of protobuf
        void* pMetricData;

        // Helpers
        // Map user-variables
        std::map<llvm::MetadataAsValue*, IGC_METRICS::VarInfo*> map_Var;
        // Map Function debuginfo to Function metrics
        std::map<llvm::DISubprogram*, IGC_METRICS::Function*> map_Func;
        // helpers for emulated calls
        std::map<llvm::DILocation*, IGC_METRICS::FuncEmuCalls*> map_EmuCalls;
        // helpers for loops
        std::map<llvm::DILocalScope*, IGC_METRICS::CFGStats_Loops*> map_Loops;
        // Current count of instruction in function
        int countInstInFunc;

        int CountInstInFunc(llvm::Function* pFunc);

        void GetFunctionData(IGC_METRICS::Function* func_m, llvm::Function& func);
        void UpdateFunctionArgumentsList();

        inline IGC_METRICS::VarInfo* GetVarMetric(llvm::Value* pValue);
        inline IGC_METRICS::VarInfo* AddVarMetric(llvm::DbgVariableIntrinsic* pInstr);

        inline IGC_METRICS::Function* GetFuncMetric(const llvm::Instruction* const pInstr);
        inline IGC_METRICS::Function* GetFuncMetric(llvm::Instruction* pInstr);
        inline IGC_METRICS::Function* GetFuncMetric(llvm::Loop* pLoop);
        inline IGC_METRICS::Function* GetFuncMetric(llvm::DISubprogram* pFunc);
        inline IGC_METRICS::Function* GetFuncMetric(const llvm::DebugLoc* pLoc);
        inline IGC_METRICS::Function* GetFuncMetric(const llvm::DebugLoc& pLoc);
        inline IGC_METRICS::Function* GetFuncMetric(llvm::Function* pFunc);

        void UpdateLoopsInfo();
        void UpdateModelCost();
        void CollectLoop(llvm::Loop* loop);

        inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DILexicalBlock* Loc);
        inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DISubprogram* Loc);
        inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DILocation* Loc);
        inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DIVariable* Var);
        static inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, const std::string& filePathName, int line);

        static inline const std::string GetFullPath(const char* dir, const char* fileName);
        static inline const std::string GetFullPath(const std::string& dir, const std::string& fileName);

#endif
    public:
        IGCMetricImpl();
        ~IGCMetricImpl();
        bool Enable();

        size_t getMetricDataSize();
        const void* const getMetricData();

        void Init(ShaderHash* Hash, bool isDebugInfo);

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

        void CollectDataFromDebugInfo(
            IGC::DebugInfoData *pDebugInfo,
            const IGC::VISADebugInfo *pVisaDbgInfo);

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

        void CollectIsSubGroupFuncIn(
            llvm::Function* pFunc,
            bool flag);

        void CollectGen9Gen10WithIEEESqrtDivFunc(
            llvm::Function* pFunc,
            bool flag);

        void CollectNonUniformLoop(
            llvm::Function* pFunc,
            short LoopCount, llvm::Loop* problematicLoop);

        void FinalizeStats();

        void OutputMetrics();
    };
}
