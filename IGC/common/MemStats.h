/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace IGC
{
enum SHADER_MEMORY_ALLOCS_TYPE
{
    SMAT_TINY = 0,
    SMAT_SMALL,
    SMAT_MEDIUM,
    SMAT_BIG,
    SMAT_LARGE,
    SMAT_NUM_OF_TYPES
};

struct SShaderMemoryAllocsType
{
    const char* Name;
    size_t max_size;
};

static const SShaderMemoryAllocsType g_cShaderMemoryAllocsType[] =
{
    {"Size < 4B", 4},
    {"Size < 32B", 32},
    {"Size < 256B", 256},
    {"Size < 1kB", 1024},
    {"Size > 1kB", size_t(~0)}
};

enum SHADER_MEMORY_SNAPSHOT
{
    SMS_COMPILE_START = 0,
    SMS_AFTER_ASMTOLLVMIR,
    SMS_AFTER_UNIFICATION,
    SMS_AFTER_OPTIMIZER,

    SMS_AFTER_LAYOUTPASS, // end of analysis passes
    SMS_AFTER_CISACreateDestroy_SIMD8,
    SMS_AFTER_vISACompile_SIMD8,

    SMS_AFTER_CISACreateDestroy_SIMD16,
    SMS_AFTER_vISACompile_SIMD16,

    SMS_AFTER_CISACreateDestroy_SIMD32,
    SMS_AFTER_vISACompile_SIMD32,

    SMS_AFTER_CODEGEN,
    SMS_COMPILE_END,
    MAX_SHADER_MEMORY_SNAPSHOT,
};

struct SShaderMemorySnapshotInfo
{
    const char* Name;
    bool IsMilestone;
};

/*****************************************************************************\
g_cShaderMemorySnapshot
Memory snapshot information.
\*****************************************************************************/
static const SShaderMemorySnapshotInfo g_cShaderMemorySnapshot[] =
{
    { "COMPILE_START", true },
    { "AFTER_ASMTOLLVMIR", true },
    { "AFTER_UNIFICATION", true },
    { "AFTER_OPTIMIZER", true },
    { "AFTER_LAYOUT_PASS", true },
    { "AFTER_CISACreateDestroy_SIMD8", true },
    { "AFTER_vISACompile_SIMD8", true },
    { "AFTER_CISACreateDestroy_SIMD16", true },
    { "AFTER_vISACompile_SIMD16", true },
    { "AFTER_CISACreateDestroy_SIMD32", true },
    { "AFTER_vISACompile_SIMD32", true },
    { "AFTER_CODEGEN", true },
    { "COMPILE_END", true },
};

static_assert((sizeof(g_cShaderMemorySnapshot) / sizeof(*g_cShaderMemorySnapshot)) == MAX_SHADER_MEMORY_SNAPSHOT);

}
