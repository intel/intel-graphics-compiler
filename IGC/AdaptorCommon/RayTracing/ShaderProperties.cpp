/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A collection of simple predicates to be shared between raytracing passes
/// that describe properties of each of the shader types.
///
//===----------------------------------------------------------------------===//

#include "ShaderProperties.h"
#include "MDFrameWork.h"

using namespace llvm;
using namespace IGC;

namespace ShaderProperties {

    // If we hit a return in a raygen, we just exit.
    // miss and closesthit shaders are the only shaders that can exit
    // the flow of a TraceRay().  Callable shaders exit the flow of
    // CallShader(). No others need to invoke a continuation.
    bool shaderReturnsToContinuation(CallableShaderTypeMD ShaderTy)
    {
        switch (ShaderTy)
        {
        case ClosestHit:
        case Miss:
        case Callable:
            return true;
        default:
            return false;
        }
    }

    // These are the shaders that are non-hitgroup and not the callstack
    // handler.
    bool isPrimaryShaderIdentifier(IGC::CallableShaderTypeMD ShaderTy)
    {
        switch (ShaderTy)
        {
        case RayGen:
        case Miss:
        case Callable:
            return true;
        default:
            return false;
        }
    }

    bool canPromoteContinuations(IGC::CallableShaderTypeMD ShaderTy)
    {
        return ShaderTy == RayGen;
    }

} // namespace ShaderProperties

