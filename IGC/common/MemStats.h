/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
