/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
    class Pass;
}

namespace IGC
{
llvm::Pass* createRayTracingIntrinsicAnalysisPass();
llvm::Pass* createRayTracingIntrinsicResolutionPass();
llvm::Pass* createTraceRayInlinePrepPass();
llvm::Pass* createTraceRayInlineLatencySchedulerPass();
llvm::Pass* CreateTraceRayInlineLoweringPass();
llvm::Pass* createInlineRaytracing();
llvm::Pass* CreateDynamicRayManagementPass();
llvm::Pass* CreateRTGlobalsPointerLoweringPass();
llvm::Pass* createOverrideTMaxPass(unsigned OverrideValue);
}
