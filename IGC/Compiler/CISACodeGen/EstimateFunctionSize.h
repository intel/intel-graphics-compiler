/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Pass.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/StringRef.h>
#include "llvm/Support/ScaledNumber.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <cstddef>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace llvm {
class Loop;
class LoopInfo;
}
namespace IGC {

    /// \brief Estimate function size after complete inlining.
    ///
    /// This pass visits the call graph and estimates the number of llvm IR
    /// instructions after complete inlining.
    class EstimateFunctionSize : public llvm::ModulePass, public llvm::InstVisitor<EstimateFunctionSize>
    {
    public:
        static char ID;

        enum AnalysisLevel {
            AL_Module,
            AL_Kernel
        };

        explicit EstimateFunctionSize(AnalysisLevel = AL_Module, bool = false);
        ~EstimateFunctionSize();
        EstimateFunctionSize(const EstimateFunctionSize&) = delete;
        EstimateFunctionSize& operator=(const EstimateFunctionSize&) = delete;
        llvm::StringRef getPassName() const  override { return "Estimate Function Sizes"; }
        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
        bool runOnModule(llvm::Module& M) override;

        /// \brief Return the estimated maximal function size after complete inlining.
        std::size_t getMaxExpandedSize() const;

        /// \brief Return the estimated function size after complete inlining.
        std::size_t getExpandedSize(const llvm::Function* F) const;

        bool onlyCalledOnce(const llvm::Function* F, const llvm::Function* CallerF);

        bool hasRecursion() const { return HasRecursion; }

        bool shouldEnableSubroutine() const { return EnableSubroutine; }

        bool isTrimmedFunction( llvm::Function* F);

        void visitCallInst( llvm::CallInst& CI );

        bool isStackCallAssigned(llvm::Function* F);


    private:
        void analyze();
        void checkSubroutine();
        void clear();
        void reduceKernelSize();

        /// \brief Return the associated opaque data.
        template <typename T> T* get(llvm::Function* F) {
            IGC_ASSERT(ECG.count(F));
            return static_cast<T*>(ECG[F]);
        }
        void performImplArgsAnalysis();
        void initializeTopologicalVisit(llvm::Function* root, std::unordered_map<void*, uint32_t>& FunctionsInKernel, std::deque<void*>& BottomUpQueue, bool ignoreStackCallBoundary);
        uint32_t updateExpandedUnitSize(llvm::Function* F, bool ignoreStackCallBoundary);
        void UpdateSizeAfterCollapsing(std::deque<void*>& leafNodes, std::unordered_set<void*> &funcsInKernel);
        void updateInlineCnt(llvm::Function* root);
        llvm::ScaledNumber<uint64_t> calculateTotalWeight(llvm::Function* root);
        uint32_t bottomUpHeuristic(llvm::Function* F, uint32_t& stackCall_cnt);
        void partitionKernel();
        void runStaticAnalysis();
        void reduceCompilationUnitSize();
        void trimCompilationUnit(llvm::SmallVector<void*, 64> &unitHeads, uint32_t threshold, bool ignoreStackCallBoundary);
        void performTrimming(llvm::Function *head, llvm::SmallVector<void*, 64>& functions_to_trim, uint32_t threshold, bool ignoreStackCallBoundary);
        void performGreedyTrimming(llvm::Function* head, llvm::SmallVector<void*, 64>& functions_to_trim, uint32_t threshold, bool ignoreStackCallBoundary);
        uint32_t getMaxUnitSize();
        void getFunctionsToTrim(llvm::Function* root, llvm::SmallVector<void*, 64> &trimming_pool, bool ignoreStackCallBoundary, uint32_t& func_cnt);
        void updateStaticFuncFreq();
        void estimateTotalLoopIteration(llvm::Function &F, llvm::LoopInfo *LI);

        /// \brief The module being analyzed.
        llvm::Module* M;

        /// \brief The analysis level to be performed.
        AnalysisLevel AL;

        bool tmpHasImplicitArg;
        bool matchImplicitArg( llvm::CallInst& CI );

        const llvm::StringRef GET_LOCAL_ID_X = "__builtin_IB_get_local_id_x";
        const llvm::StringRef GET_LOCAL_ID_Y = "__builtin_IB_get_local_id_y";
        const llvm::StringRef GET_LOCAL_ID_Z = "__builtin_IB_get_local_id_z";
        const llvm::StringRef GET_GROUP_ID = "__builtin_IB_get_group_id";
        const llvm::StringRef GET_LOCAL_THREAD_ID = "__builtin_IB_get_local_thread_id";
        const llvm::StringRef GET_GLOBAL_SIZE = "__builtin_IB_get_global_size";
        const llvm::StringRef GET_LOCAL_SIZE = "__builtin_IB_get_local_size";
        const llvm::StringRef GET_GLOBAL_OFFSET = "__builtin_IB_get_global_offset";
        const llvm::StringRef GET_WORK_DIM = "__builtin_IB_get_work_dim";
        const llvm::StringRef GET_NUM_GROUPS = "__builtin_IB_get_num_groups";
        const llvm::StringRef GET_ENQUEUED_LOCAL_SIZE = "__builtin_IB_get_enqueued_local_size";
        const llvm::StringRef GET_STAGE_IN_GRID_ORIGIN = "__builtin_IB_get_stage_in_grid_origin";
        const llvm::StringRef GET_STAGE_IN_GRID_SIZE = "__builtin_IB_get_stage_in_grid_size";
        const llvm::StringRef GET_SYNC_BUFFER = "__builtin_IB_get_sync_buffer";
        const llvm::StringRef GET_ASSERT_BUFFER = "__builtin_IB_get_assert_buffer";
        bool HasRecursion;
        bool EnableSubroutine;

        /// Internal data structure for the analysis which is approximately an
        /// extended call graph.
        llvm::SmallDenseMap<llvm::Function*, void*> ECG;
        //Kernel entries
        llvm::SmallVector<void*, 64> kernelEntries;
        //Functions that are assigned stackcalls
        llvm::SmallVector<void*, 64> stackCallFuncs;
        llvm::SmallVector<void*, 64> addressTakenFuncs;
        llvm::ScaledNumber<uint64_t> threshold_func_freq;
        llvm::ScaledNumber<uint64_t> thresholdForTrimming;
        std::unordered_map<llvm::Loop *, llvm::ScaledNumber<uint64_t>>
            LoopIterCnts;

        // Flags for Kernel trimming
        bool ControlKernelTotalSize;
        bool ControlUnitSize;
        unsigned ControlInlineTinySize;
        unsigned UnitSizeThreshold;

        // Flags for Static Profile-guided trimming
        bool StaticProfileGuidedTrimming;
        bool UseFrequencyInfoForSPGT;
        bool BlockFrequencySampling;
        bool EnableLeafCollapsing;
        bool EnableSizeContributionOptimization;
        bool LoopCountAwareTrimming;
        bool EnableGreedyTrimming;
        unsigned SizeWeightForSPGT;
        unsigned FrequencyWeightForSPGT;
        unsigned MetricForKernelSizeReduction;
        unsigned ParameterForColdFuncThreshold;
        unsigned ControlInlineTinySizeForSPGT;
        unsigned MaxUnrollCountForFunctionSizeAnalysis;
        unsigned SkipTrimmingOneCopyFunction;
        std::string SelectiveTrimming;

        // Flags for Partitioning
        bool PartitionUnit;
        bool StaticProfileGuidedPartitioning;

        // Flags for implcit arguments and external functions
        bool ForceInlineExternalFunctions;
        bool ForceInlineStackCallWithImplArg;
        bool ControlInlineImplicitArgs;
        unsigned SubroutineThreshold;
        unsigned KernelTotalSizeThreshold;
        unsigned ExpandedUnitSizeThreshold;
    };

    llvm::ModulePass* createEstimateFunctionSizePass();
    llvm::ModulePass *createEstimateFunctionSizePass(bool);
    llvm::ModulePass* createEstimateFunctionSizePass(EstimateFunctionSize::AnalysisLevel);

} // namespace IGC
