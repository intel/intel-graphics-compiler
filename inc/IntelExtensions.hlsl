/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

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
    uint   rid;     // resource ID
    uint   sid;     // sampler ID

    float4 src0f;   // float source operand  0
    float4 src1f;   // float source operand  0
    float4 src2f;   // float source operand  0
    float4 dst0f;   // float destination operand

    uint4  src0u;
    uint4  src1u;
    uint4  src2u;
    uint4  dst0u;

    float  pad[181]; // total lenght 864
};

//
// extension opcodes
//
#define INTEL_EXT_WAVE_SHUFFLE              17
#define INTEL_EXT_WAVE_LANEINDEX            18
#define INTEL_EXT_WAVE_WAVEACTIVEBALLOT     19
#define INTEL_EXT_WAVE_WAVEPREFIXOP         20
#define INTEL_EXT_WAVE_WAVEALL              21
#define INTEL_EXT_SIMDSIZE                  22


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


//
// Initialize Intel HSLS Extensions
// This method should be called before any other extension function
//
void IntelExt_Init()
{
    uint4 init = { 0x63746e69, 0x6c736c68, 0x6e747865, 0x0 }; // intc hlsl extn
    g_IntelExt[0].src0u = init;
}

// Extension matching DirectX12 Wave instructions
// DX12 behavior is described here:
// https://github.com/Microsoft/DirectXShaderCompiler/wiki/Wave-Intrinsics
#define INTEL_EXT_WAVEOPS_SUM     0
#define INTEL_EXT_WAVEOPS_PROD    1
#define INTEL_EXT_WAVEOPS_UMIN    2
#define INTEL_EXT_WAVEOPS_UMAX    3
#define INTEL_EXT_WAVEOPS_IMIN    4
#define INTEL_EXT_WAVEOPS_IMAX    5
#define INTEL_EXT_WAVEOPS_OR      6
#define INTEL_EXT_WAVEOPS_XOR     7
#define INTEL_EXT_WAVEOPS_AND     8
#define INTEL_EXT_WAVEOPS_FSUM    9
#define INTEL_EXT_WAVEOPS_FPROD   10
#define INTEL_EXT_WAVEOPS_FMIN    11
#define INTEL_EXT_WAVEOPS_FMAX    12

#define INTEL_EXT_UINT64_ATOMIC      24

#define INTEL_EXT_ATOMIC_ADD          0
#define INTEL_EXT_ATOMIC_MIN          1
#define INTEL_EXT_ATOMIC_MAX          2
#define INTEL_EXT_ATOMIC_CMPXCHG      3
#define INTEL_EXT_ATOMIC_XCHG         4
#define INTEL_EXT_ATOMIC_AND          5
#define INTEL_EXT_ATOMIC_OR           6
#define INTEL_EXT_ATOMIC_XOR          7

float IntelExt_WaveReadLaneAt(float input, uint lane)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_SHUFFLE;
    g_IntelExt[opcode].src0f.x = input;
    g_IntelExt[opcode].src1u.x = lane;
    return g_IntelExt[opcode].dst0f.x;
}

int IntelExt_WaveReadLaneAt(int input, uint lane)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_SHUFFLE;
    g_IntelExt[opcode].src0u.x = input;
    g_IntelExt[opcode].src1u.x = lane;
    return g_IntelExt[opcode].dst0u.x;
}

uint IntelExt_WaveGetLaneIndex()
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_LANEINDEX;
    return g_IntelExt[opcode].dst0u.x;
}

float IntelExt_QuadReadAcrossDiagonal(float localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 3));
}

int IntelExt_QuadReadAcrossDiagonal(int localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 3));
}

float IntelExt_QuadReadLaneAt(float sourceValue, uint quadLaneID)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(sourceValue, (laneID & 0xFC) + quadLaneID);
}

int IntelExt_QuadReadLaneAt(int sourceValue, uint quadLaneID)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(sourceValue, (laneID & 0xFC) + quadLaneID);
}

float IntelExt_QuadReadAcrossX(float localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 1));
}

int IntelExt_QuadReadAcrossX(int localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 1));
}

float IntelExt_QuadReadAcrossY(float localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 2));
}

int IntelExt_QuadReadAcrossY(int localValue)
{
    uint laneID = IntelExt_WaveGetLaneIndex();
    return IntelExt_WaveReadLaneAt(localValue, (laneID & 0xFC) + ((laneID & 3) ^ 2));
}

uint4 IntelExt_WaveActiveBallot(bool localValue)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEACTIVEBALLOT;
    g_IntelExt[opcode].src1u.x = localValue;
    return uint4(g_IntelExt[opcode].dst0u.x, 0, 0, 0);
}

float IntelExt_WaveReadLaneFirst(float input)
{
    uint firstLaneID = firstbitlow(IntelExt_WaveActiveBallot(true).x);
    return IntelExt_WaveReadLaneAt(input, firstLaneID);
}

int IntelExt_WaveReadLaneFirst(int input)
{
    uint firstLaneID = firstbitlow(IntelExt_WaveActiveBallot(true).x);
    return IntelExt_WaveReadLaneAt(input, firstLaneID);
}

bool IntelExt_WaveActiveAllTrue(bool expr)
{
    return (IntelExt_WaveActiveBallot(expr).x == IntelExt_WaveActiveBallot(true).x);
}

bool IntelExt_WaveActiveAllEqual(float localValue)
{
    return IntelExt_WaveActiveAllTrue(IntelExt_WaveReadLaneFirst(localValue) == localValue);
}

bool IntelExt_WaveActiveAllEqual(int localValue)
{
    return IntelExt_WaveActiveAllTrue(IntelExt_WaveReadLaneFirst(localValue) == localValue);
}

float IntelExt_WaveAll(float localValue, uint opType)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEALL;
    g_IntelExt[opcode].src0f.x = localValue;
    g_IntelExt[opcode].src1u.x = opType;
    return g_IntelExt[opcode].dst0f.x;
}

int IntelExt_WaveAll(int localValue, uint opType)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEALL;
    g_IntelExt[opcode].src0u.x = localValue;
    g_IntelExt[opcode].src1u.x = opType;
    return g_IntelExt[opcode].dst0u.x;
}

int IntelExt_WaveActiveBitAnd(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_AND);
}

int IntelExt_WaveActiveBitOr(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_OR);
}

int IntelExt_WaveActiveBitXor(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_XOR);
}

uint IntelExt_WaveActiveCountBits(bool bBit)
{
    return countbits(IntelExt_WaveActiveBallot(bBit).x);
}

float IntelExt_WaveActiveMax(float localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_FMAX);
}

int IntelExt_WaveActiveMax(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_IMAX);
}

uint IntelExt_WaveActiveMax(uint localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_UMAX);
}

float IntelExt_WaveActiveMin(float localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_FMIN);
}

int IntelExt_WaveActiveMin(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_IMIN);
}

uint IntelExt_WaveActiveMin(uint localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_UMIN);
}

float IntelExt_WaveActiveProduct(float localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_FPROD);
}

int IntelExt_WaveActiveProduct(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_PROD);
}

float IntelExt_WaveActiveSum(float localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_FSUM);
}

int IntelExt_WaveActiveSum(int localValue)
{
    return IntelExt_WaveAll(localValue, INTEL_EXT_WAVEOPS_SUM);
}

bool IntelExt_WaveActiveAnyTrue(bool expr)
{
    return (IntelExt_WaveActiveBallot(expr).x != 0);
}

uint IntelExt_WaveGetLaneCount()
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_SIMDSIZE;
    return g_IntelExt[opcode].dst0u.x;
}

bool IntelExt_WaveIsFirstLane()
{
    uint firstLaneID = firstbitlow(IntelExt_WaveActiveBallot(true).x);
    uint index = IntelExt_WaveGetLaneIndex();
    return (firstLaneID == index);
}

uint IntelExt_WavePrefixCountBits(bool bBit)
{
    uint ballot = IntelExt_WaveActiveBallot(bBit).x;
    uint index = IntelExt_WaveGetLaneIndex();
    return countbits(ballot & ((1 << index) - 1));
}

float IntelExt_WavePrefixProduct(float value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEPREFIXOP;
    g_IntelExt[opcode].src0f.x = value;
    g_IntelExt[opcode].src1u.x = INTEL_EXT_WAVEOPS_FPROD;
    return g_IntelExt[opcode].dst0f.x;
}

int IntelExt_WavePrefixProduct(int value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEPREFIXOP;
    g_IntelExt[opcode].src0u.x = value;
    g_IntelExt[opcode].src1u.x = INTEL_EXT_WAVEOPS_PROD;
    return g_IntelExt[opcode].dst0u.x;
}

float IntelExt_WavePrefixSum(float value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEPREFIXOP;
    g_IntelExt[opcode].src0f.x = value;
    g_IntelExt[opcode].src1u.x = INTEL_EXT_WAVEOPS_FSUM;
    return g_IntelExt[opcode].dst0f.x;
}

int IntelExt_WavePrefixSum(int value)
{
    uint opcode = g_IntelExt.IncrementCounter();
    g_IntelExt[opcode].opcode = INTEL_EXT_WAVE_WAVEPREFIXOP;
    g_IntelExt[opcode].src0u.x = value;
    g_IntelExt[opcode].src1u.x = INTEL_EXT_WAVEOPS_SUM;
    return g_IntelExt[opcode].dst0u.x;
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

// Interlocked Min
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

// Interlocked exchange
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