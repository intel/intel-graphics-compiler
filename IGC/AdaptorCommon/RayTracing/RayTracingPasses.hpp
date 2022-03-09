/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
    class Pass;
}

namespace IGC
{
llvm::Pass* createRayTracingIntrinsicLoweringPass();
llvm::Pass* createEarlyRematPass();
llvm::Pass* createLateRematPass();
llvm::Pass* CreateBindlessKernelArgLoweringPass();
llvm::Pass* CreateBindlessInlineDataPass();
llvm::Pass* createRayTracingConstantCoalescingPass();
llvm::Pass* createStackIDSchedulingPass();
llvm::Pass* CreateStackIDRetirement();
llvm::Pass* createRayTracingIntrinsicAnalysisPass();
llvm::Pass* createRayTracingIntrinsicResolutionPass();
llvm::Pass* createTraceRayInlinePrepPass();
llvm::Pass* createTraceRayInlineLatencySchedulerPass();
llvm::Pass* CreateTraceRayInlineLoweringPass();
llvm::Pass* CreateRTGlobalsPointerLoweringPass();
llvm::Pass* createPrivateToGlobalPointerPass();
llvm::Pass* createLowerIntersectionAnyHitPass();
llvm::Pass* createSplitPreparePass();
llvm::Pass* createSplitAsyncPass();
llvm::Pass* createRayTracingFinalizePass();
llvm::Pass* createInlineMergeCallsPass();
llvm::Pass* createPromoteToScratchPass();
llvm::Pass* createRayInfoCSEPass();
llvm::Pass* createRayTracingPrintfPostProcessPass();
llvm::Pass* createPayloadSinkingAnalysisPass();
llvm::Pass* createPayloadSinkingPass();
llvm::Pass* createLowerGlobalRootSignaturePass();
llvm::Pass* createRayTracingMemDSEPass();
llvm::Pass* createDeadPayloadStoreEliminationPass();
}
