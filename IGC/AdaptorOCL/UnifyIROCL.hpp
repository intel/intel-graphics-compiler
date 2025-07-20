/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenPublic.h"

namespace IGC {
void UnifyIROCL(OpenCLProgramContext *pContext);

void UnifyIRSPIR(OpenCLProgramContext *pContext);
} // namespace IGC
