/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Intel extension buffer structure, generic interface for
//   0-operand extensions (e.g. sync opcodes)
//   1-operand unary operations (e.g. render target writes)
//   2-operand binary operations (future extensions)
//   3-operand ternary operations (future extensions)
//
struct IntelExtensionStruct
{
    uint   opcode;  // opcode to execute
    uint   rid; // resource ID
    uint   sid; // sampler ID

    float4 src0f;   // float source operand  0
    float4 src1f;   // float source operand  0
    float4 src2f;   // float source operand  0
    float4 dst0f;   // float destination operand

    uint4  src0u;
    uint4  src1u;
    uint4  src2u;
    uint4  dst0u;

    float  pad[181]; // total length 864
};

//
// extension opcodes
//

// Define RW buffer for Intel extensions.
// Application should bind null resource, operations will be ignored.
// If application needs to use slot other than u63, it needs to
// define INTEL_SHADER_EXT_UAV_SLOT as a unused slot. This should be
// defined before including this file in shader as:
// #define INTEL_SHADER_EXT_UAV_SLOT u8

#ifdef INTEL_SHADER_EXT_UAV_SLOT
RWStructuredBuffer<IntelExtensionStruct> g_IntelExt : register( INTEL_SHADER_EXT_UAV_SLOT );
#else
RWStructuredBuffer<IntelExtensionStruct> g_IntelExt : register( u63 );
#endif

#define INTEL_EXT_UINT64_ATOMIC           24

#define INTEL_EXT_ATOMIC_ADD          0
#define INTEL_EXT_ATOMIC_MIN          1
#define INTEL_EXT_ATOMIC_MAX          2
#define INTEL_EXT_ATOMIC_CMPXCHG      3
#define INTEL_EXT_ATOMIC_XCHG         4
#define INTEL_EXT_ATOMIC_AND          5
#define INTEL_EXT_ATOMIC_OR           6
#define INTEL_EXT_ATOMIC_XOR          7


//
// Initialize Intel HLSL Extensions
// This method should be called before any other extension function
//
void IntelExt_Init()
{
    uint4 init = { 0x63746e69, 0x6c736c68, 0x6e747865, 0x32313030 }; // intc hlsl extn 0012
    g_IntelExt[0].src0u = init;
}


// uint64 typed atomics
// Interlocked max
uint2 IntelExt_InterlockedMaxUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_MAX;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked min
uint2 IntelExt_InterlockedMinUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_MIN;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked and
uint2 IntelExt_InterlockedAndUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_AND;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked or
uint2 IntelExt_InterlockedOrUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_OR;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked add
uint2 IntelExt_InterlockedAddUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_ADD;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked xor
uint2 IntelExt_InterlockedXorUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_XOR;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked xor
uint2 IntelExt_InterlockedExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_XCHG;

    return g_IntelExt[opcode].dst0u.xy;
}

// Interlocked compare exchange
uint2 IntelExt_InterlockedCompareExchangeUint64(RWTexture2D<uint2> uav, uint2 address, uint2 cmp_value, uint2 xchg_value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    uav[uint2(opcode, opcode)] = uint2(0, 0); //dummy instruction to get the resource handle
    g_IntelExt[opcode].opcode = INTEL_EXT_UINT64_ATOMIC;
    g_IntelExt[opcode].src0u.xy = address;
    g_IntelExt[opcode].src1u.xy = cmp_value;
    g_IntelExt[opcode].src1u.zw = xchg_value;
    g_IntelExt[opcode].src2u.x = INTEL_EXT_ATOMIC_CMPXCHG;

    return g_IntelExt[opcode].dst0u.xy;
}