/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "EmUtils.h"

enum CallableShaderTypeMD : uint32_t
{
    AnyHit,
    ClosestHit,
    Intersection,
    Miss,
    Callable,
    CallStackHandler,
    NumberOfCallableShaderTypes,
    // raygen shaders are not callable shaders as they aren't invoked
    // by any RTUnit or BTD machinery.  It is useful, though, to have all
    // shader types available for reference in this enum.
    RayGen,
};

