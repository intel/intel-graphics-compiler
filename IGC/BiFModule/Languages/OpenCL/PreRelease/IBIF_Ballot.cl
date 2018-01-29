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

#include "spirv.h"

uint intel_sub_group_ballot(bool p)
{
    return __builtin_IB_WaveBallot(p);
}

uint4 __builtin_spirv_BuiltInSubgroupEqMaskKHR()
{
    uint id = __builtin_spirv_BuiltInSubgroupLocalInvocationId();
    uint4 v = 0;

    v.x = 1 << id;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupGeMaskKHR()
{
    uint id = __builtin_spirv_BuiltInSubgroupLocalInvocationId();
    uint4 v = 0;

    v.x = as_uint(as_int(1 << 31) >> (31 - id));

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupLeMaskKHR()
{
    uint id = __builtin_spirv_BuiltInSubgroupLocalInvocationId();
    uint4 v = 0;

    uint bitIdx = 1 << id;

    v.x = (bitIdx - 1) | bitIdx;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupGtMaskKHR()
{
    uint4 v = 0;

    v.x = ~__builtin_spirv_BuiltInSubgroupLeMaskKHR().x;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupLtMaskKHR()
{
    uint id = __builtin_spirv_BuiltInSubgroupLocalInvocationId();
    uint4 v = 0;

    v.x = (1 << id) - 1;

    return v;
}

uint4 __builtin_spirv_OpSubgroupBallotKHR_i1(bool Predicate)
{
    uint4 v = 0;
    v.x = __builtin_IB_WaveBallot(Predicate);
    return v;
}

uint __builtin_spirv_OpSubgroupFirstInvocationKHR_i32(uint Value)
{
    uint chanEnable = __builtin_IB_WaveBallot(true);
    uint firstActive = __builtin_spirv_OpenCL_ctz_i32(chanEnable);
    uint3 id = (uint3)(firstActive, 0, 0);
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(Subgroup, Value, id);
}

float __builtin_spirv_OpSubgroupFirstInvocationKHR_f32(float Value)
{
    return as_float(__builtin_spirv_OpSubgroupFirstInvocationKHR_i32(as_uint(Value)));
}


