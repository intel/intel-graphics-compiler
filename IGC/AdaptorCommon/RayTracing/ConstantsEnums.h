/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

// THIS FILE IS USED BY MDFrameWork AUTOGENERATION WHICH
// DOES NOT ACCEPT LLVM CODE FORMATTING
//
// DOT NOT CHANGE THE FORMATTING!!!

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
        ANY_HIT              = 0b0000,
        CLOSEST_HIT          = 0b0001,
        MISS                 = 0b0010,
        INTERSECTION         = 0b0011,
        RESERVED             = 0b0100
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
