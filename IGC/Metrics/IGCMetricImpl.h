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

#define HashKey size_t
#define HashKey_NULL 0
#endif // IGC_METRICS

#pragma once

namespace IGCMetrics
{
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
    typedef std::vector<IGC_METRICS::Function*> MFuncList;
#endif

    class IGCMetricImpl
    {
    private:
        bool isEnabled;
#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        IGC_METRICS::Program oclProgram;

        // Helpers
        // Map all instruction line number, filepath to Function
        // { Key: Hash(fullPathFile+LineNb), Value:Function list containing this instruction }
        std::map<HashKey, MFuncList*> map_InstrLoc2Func;
        // Map Function debuginfo to Function metrics
        std::map<llvm::DISubprogram*, IGC_METRICS::Function*> map_Func;
        // helpers for emulated calls
        std::map<llvm::DILocation*, IGC_METRICS::FuncEmuCalls*> map_EmuCalls;
        // helpers for loops
        std::map<llvm::DILocalScope*, IGC_METRICS::CFGStats_Loops*> map_Loops;
        // Current count of instruction in function
        int countInstInFunc;

        int CountInstInFunc(llvm::Function* pFunc);

        void GetFunctionCalls(IGC_METRICS::Function* func_m, llvm::Function& func);

        inline MFuncList* GetFuncMetric(llvm::Instruction* pInstr);
        inline MFuncList* GetFuncMetric(llvm::Loop* pLoop);
        inline MFuncList* GetFuncMetric(HashKey Key);

        void CollectInstructions(llvm::Module* pModule);

        void UpdateLoopsInfo();
        void CollectLoop(llvm::Loop* loop);

        static HashKey GetHash(const char* dir, const char* fileName, int line);
        static HashKey GetHash(const char* filePathName, int line);
        static HashKey GetHash(const std::string& dir, const std::string& fileName, int line);
        static HashKey GetHash(const std::string& filePathName, int line);
        static HashKey GetHash(llvm::DILocation* Loc);

        static inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DISubprogram* Loc);
        static inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, llvm::DILocation* Loc);
        static inline void FillCodeRef(IGC_METRICS::CodeRef* codeRef, const std::string& filePathName, int line);

        static inline const std::string GetFullPath(const char* dir, const char* fileName);
        static inline const std::string GetFullPath(const std::string& dir, const std::string& fileName);

        void UpdateCollectInstructions(llvm::Function* func);
#endif
    public:
        IGCMetricImpl();
        ~IGCMetricImpl();
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