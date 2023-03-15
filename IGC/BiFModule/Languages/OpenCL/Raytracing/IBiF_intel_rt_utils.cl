/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "raytracing/constants.h"
#include "IBiF_intel_rt_utils.h"

#define sizeofbits(val) (8 * sizeof(val))

// === --------------------------------------------------------------------===
// === Bitfield accessors
// === --------------------------------------------------------------------===

ushort __getBits16(ushort value, uint startBit, uint width)
{
    ushort value_aligned = value >> startBit;
    ushort mask = USHRT_MAX >> (sizeofbits(ushort) - width);
    return value_aligned & mask;
}

uint __getBits32(uint value, uint startBit, uint width)
{
    uint value_aligned = value >> startBit;
    uint mask = UINT_MAX >> (sizeofbits(uint) - width);
    return value_aligned & mask;
}

ulong __getBits64(ulong value, uint startBit, uint width)
{
    ulong value_aligned = value >> startBit;
    ulong mask = ULONG_MAX >> (sizeofbits(ulong) - width);
    return value_aligned & mask;
}


// === --------------------------------------------------------------------===
// === Bitfield setters
// === --------------------------------------------------------------------===
//
// Example for startBit=2, width=3
//   slotWidthMask = 00000111
//   valueSlitMask = 11100011

ushort __setBits16(ushort value, ushort slot, uint startBit, uint width)
{
    ushort slotWidthMask = USHRT_MAX >> (sizeofbits(ushort) - width);
    ushort valueSlitMask = ~(slotWidthMask << startBit);
    return (value & valueSlitMask) | ((slot & slotWidthMask) << startBit);
}

uint __setBits32(uint value, uint slot, uint startBit, uint width)
{
    uint slotWidthMask = UINT_MAX >> (sizeofbits(uint) - width);
    uint valueSlitMask = ~(slotWidthMask << startBit);
    return (value & valueSlitMask) | ((slot & slotWidthMask) << startBit);
}

ulong __setBits64(ulong value, ulong slot, uint startBit, uint width)
{
    ulong slotWidthMask = ULONG_MAX >> (sizeofbits(ulong) - width);
    ulong valueSlitMask = ~(slotWidthMask << startBit);
    return (value & valueSlitMask) | ((slot & slotWidthMask) << startBit);
}

// === --------------------------------------------------------------------===
// === Helper functions
// === --------------------------------------------------------------------===
global void* __getImplicitDispatchGlobals()
{
    global char* globalBuffer = __builtin_IB_intel_get_rt_global_buffer();
    int subDeviceID = SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )();
    return globalBuffer + subDeviceID * DISPATCH_GLOBALS_STRIDE;
}