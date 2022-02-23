/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This file should contain constants and enums that need to be shared with
/// a UMD.
///
//===----------------------------------------------------------------------===//

#pragma once

#include <stdint.h>

namespace RTStackFormat
{
    enum class HIT_GROUP_TYPE
    {
        TRIANGLES = 0x0,
        PROCEDURAL_PRIMITIVE = 0x1,
    };

    enum struct RayTracingShaderType
    {
        ANY_HIT = 0b000,
        CLOSEST_HIT = 0b001,
        MISS = 0b010,
        INTERSECTION = 0b011,
        RESERVED = 0b100
    };

    enum HIT_KIND
    {
        HIT_KIND_TRIANGLE_FRONT_FACE = 254,
        HIT_KIND_TRIANGLE_BACK_FACE = 255
    };

    constexpr static uint32_t RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH = 31;
    constexpr static uint32_t RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES = 32;
    constexpr static uint32_t RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT = 32;
    constexpr static uint32_t RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT = 64;

    constexpr static uint32_t SHADER_IDENTIFIER_SIZE_IN_BYTES = 32;
}

namespace IGC
{
    // These options tell the UMD how IGC has codegen'd accesses to raytracing
    // memory
    enum class RTMemoryAccessMode
    {
        // IGC accesses the Surface State Heap (SSH) via the binding table
        BTI,
        // IGC accesses the Surface State Heap (SSH) via bindless offset. Note that
        // this is *not* the bindless surface state heap.
        Bindless,
        // IGC does stateless accesses and requires no further setup from the UMD
        Stateless
    };
}
