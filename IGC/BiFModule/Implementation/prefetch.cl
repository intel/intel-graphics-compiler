/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCBiF_Intrinsics_Lsc.cl"

extern __constant int __UseLSC;
extern __constant int __ForceL1Prefetch;

// Mapping from OpenCL prefetch to LSC prefetch. Uniform immediate offset can
// be used to save on base pointer arithmetics, but offset can't be variable.
#define LSC_PREFETCH(type, p, num_elements)                                            \
    /* Warning: out of bound L1 prefetch will generate page fault. When L1 prefetch is \
       enabled, round down to not overfetch. Limit prefetch size to 32 bytes per work  \
       item (max size for single load message in SIMD16). */                           \
    enum LSC_LDCC cacheOpt = __ForceL1Prefetch ? LSC_LDCC_L1C_L3C : LSC_LDCC_L1UC_L3C; \
    int size = sizeof(type) * num_elements;                                            \
    if (__ForceL1Prefetch)                                                             \
    {                                                                                  \
        /* overfetch unsafe, round down */                                             \
        if (size >= 32)                                                                \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint8(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size >= 16)                                                           \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint4(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size >= 12)                                                           \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint3(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size >= 8)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint2(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size >= 4)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);                     \
        }                                                                              \
        else if (size >= 2)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);                   \
        }                                                                              \
        else if (size >= 1)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);                    \
        }                                                                              \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        /* overfetch safe, round up */                                                 \
        if (size > 16)                                                                 \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint8(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size > 12)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint4(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size > 8)                                                             \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint3(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size > 4)                                                             \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint2(p, 0, cacheOpt);                    \
        }                                                                              \
        else if (size > 2)                                                             \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);                     \
        }                                                                              \
        else if (size > 1)                                                             \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);                   \
        }                                                                              \
        else if (size == 1)                                                            \
        {                                                                              \
            __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);                    \
        }                                                                              \
    }                                                                                  \

//Prefetch function

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i8_i32, )( global char* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uchar, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i32, )( global char2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i32, )( global char3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uchar, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i32, )( global char4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i32, )( global char8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i32, )( global char16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i16_i32, )( global short* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i32, )( global short2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i32, )( global short3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i32, )( global short4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i32, )( global short8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i32, )( global short16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i32_i32, )( global int* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i32, )( global int2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i32, )( global int3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i32, )( global int4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i32, )( global int8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i32, )( global int16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i64_i32, )( global long* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i32, )( global long2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i32, )( global long3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i32, )( global long4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i32, )( global long8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i32, )( global long16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f32_i32, )( global float* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i32, )( global float2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i32, )( global float3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i32, )( global float4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i32, )( global float8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i32, )( global float16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f16_i32, )( global half* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i32, )( global half2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i32, )( global half3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i32, )( global half4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i32, )( global half8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i32, )( global half16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f64_i32, )( global double* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i32, )( global double2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i32, )( global double3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i32, )( global double4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i32, )( global double8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i32, )( global double16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}
#endif // defined(cl_khr_fp64)


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i8_i64, )( global char* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uchar, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i64, )( global char2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i64, )( global char3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uchar, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i64, )( global char4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i64, )( global char8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i64, )( global char16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i16_i64, )( global short* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i64, )( global short2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i64, )( global short3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i64, )( global short4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i64, )( global short8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i64, )( global short16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i32_i64, )( global int* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i64, )( global int2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i64, )( global int3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i64, )( global int4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i64, )( global int8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i64, )( global int16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1i64_i64, )( global long* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i64, )( global long2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i64, )( global long3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i64, )( global long4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i64, )( global long8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i64, )( global long16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f32_i64, )( global float* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i64, )( global float2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i64, )( global float3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i64, )( global float4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i64, )( global float8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i64, )( global float16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f16_i64, )( global half* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i64, )( global half2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i64, )( global half3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ushort, p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i64, )( global half4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i64, )( global half8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i64, )( global half16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(uint8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1f64_i64, )( global double* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i64, )( global double2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong2, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i64, )( global double3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong3, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i64, )( global double4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong4, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i64, )( global double8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void SPIRV_OVERLOADABLE OPTNONE SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i64, )( global double16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (__UseLSC)
    {
        LSC_PREFETCH(ulong8, p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}
#endif // defined(cl_khr_fp64)

