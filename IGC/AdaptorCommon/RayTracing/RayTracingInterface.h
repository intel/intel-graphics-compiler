/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Functions that are called for raytracing lowering after a frontend has
/// done appropriate setup.  There are three main things to be done before
/// calling these:
///
/// 1) Lower the IR to use the expected intrinsics
/// 2) RayDispatchShaderContext should be set up with appropriate state (e.g.,
///    hitgroup information, whether we are an RTPSO or not, etc.)
/// 3) Any MDFrameWork.h metadata that needs to be used
///
//===----------------------------------------------------------------------===//
#pragma once

#include "Compiler/CodeGenPublic.h"
#include <vector>

namespace IGC
{
void RayTracingLowering(RayDispatchShaderContext* pContext);
void RayTracingInlineLowering(CodeGenContext* pContext);
}
