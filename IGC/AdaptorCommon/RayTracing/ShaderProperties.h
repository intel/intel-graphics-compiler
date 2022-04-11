/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/MDFrameWork.h"

namespace ShaderProperties {
    bool shaderReturnsToContinuation(IGC::CallableShaderTypeMD ShaderTy);
    bool isPrimaryShaderIdentifier(IGC::CallableShaderTypeMD ShaderTy);
    bool canPromoteContinuations(IGC::CallableShaderTypeMD ShaderTy);
} // namespace ShaderProperties

