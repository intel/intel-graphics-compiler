/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGCBiF_Intrinsics_Lsc.cl"

#define LSC_PREFETCH(p, num_elements) __lsc_prefetch(p, sizeof(*p), num_elements)

// Mapping from OpenCL prefetch to LSC prefetch. OpenCL prefetch doesn't have
// cache control options; default cache control options are used.
INLINE void __lsc_prefetch(global void* p, int element_size, int num_elements)
{
    enum LSC_LDCC cacheOpt = BIF_FLAG_CTRL_GET(ForceL1Prefetch) ? LSC_LDCC_L1C_L3C : LSC_LDCC_L1UC_L3C;

    int size = element_size * num_elements;

    // If pointer is aligned, vector data type can be used and up to 32 bytes per work
    // item can be fetched in single load message in SIMD16 mode. If pointer is not
    // aligned, only scalar type can be used, limiting fetch to 8 bytes per work item
    // in one load message.
    bool aligned = (ulong) p % 4 == 0;

    // Warning: out of bound L1 prefetch will generate page fault. When L1 prefetch is
    // enabled, round down to not overfetch.
    if (BIF_FLAG_CTRL_GET(ForceL1Prefetch))
    {
        // overfetch unsafe, round down
        if (aligned)
        {
            // aligned, can use vectors
            if (size >= 32)
            {
                __builtin_IB_lsc_prefetch_global_uint8(p, 0, cacheOpt);
            }
            else if (size >= 16)
            {
                __builtin_IB_lsc_prefetch_global_uint4(p, 0, cacheOpt);
            }
            else if (size >= 12)
            {
                __builtin_IB_lsc_prefetch_global_uint3(p, 0, cacheOpt);
            }
            else if (size >= 8)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
            }
            else if (size >= 4)
            {
                __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);
            }
            else if (size >= 2)
            {
                __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);
            }
            else if (size >= 1)
            {
                __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);
            }
        }
        else
        {
            // unaligned, don't use vectors
            if (size >= 16)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
                __builtin_IB_lsc_prefetch_global_ulong((global ulong*)p + 1, 0, cacheOpt);
            }
            else if (size >= 8)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
            }
            else if (size >= 4)
            {
                __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);
            }
            else if (size >= 2)
            {
                __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);
            }
            else if (size >= 1)
            {
                __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);
            }
        }
    }
    else
    {
        // overfetch safe, round up
        if (aligned)
        {
            // aligned, can use vectors
            if (size > 16)
            {
                __builtin_IB_lsc_prefetch_global_uint8(p, 0, cacheOpt);
            }
            else if (size > 12)
            {
                __builtin_IB_lsc_prefetch_global_uint4(p, 0, cacheOpt);
            }
            else if (size > 8)
            {
                __builtin_IB_lsc_prefetch_global_uint3(p, 0, cacheOpt);
            }
            else if (size > 4)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
            }
            else if (size > 2)
            {
                __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);
            }
            else if (size > 1)
            {
                __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);
            }
            else if (size == 1)
            {
                __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);
            }
        }
        else
        {
            // unaligned, don't use vectors
            if (size > 8)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
                __builtin_IB_lsc_prefetch_global_ulong((global ulong*)p + 1, 0, cacheOpt);
            }
            else if (size > 4)
            {
                __builtin_IB_lsc_prefetch_global_ulong(p, 0, cacheOpt);
            }
            else if (size > 2)
            {
                __builtin_IB_lsc_prefetch_global_uint(p, 0, cacheOpt);
            }
            else if (size > 1)
            {
                __builtin_IB_lsc_prefetch_global_ushort(p, 0, cacheOpt);
            }
            else if (size == 1)
            {
                __builtin_IB_lsc_prefetch_global_uchar(p, 0, cacheOpt);
            }
        }
    }
}

// Mapping from OpenCL prefetch to LSC prefetch with exposed cache controls.
INLINE void __lsc_prefetch_cache_controls(global void* p, int element_size, int num_elements, enum LSC_LDCC cache_opt)
{
    int size = element_size * num_elements;

    // Assumptions:
    // 1. Vector data type can be used only for i32/i64 types and only if
    //    pointer is aligned. OpenCL defines alignment to the size of the
    //    data type in bytes. Instead of checking alignment at runtime:
    //      a. Assume i32/i64 types are aligned to 4 bytes.
    //      b. Assume i8/i16 types are not aligned.
    // 2. Assume overfetch is safe. For platforms generating page faults
    //    on out of bounds prefetch, cache controls must be corrected
    //    before calling builtin.

    if (element_size % 4)
    {
        // unaligned, don't use vectors
        if (size > 8)
        {
            __builtin_IB_lsc_prefetch_global_ulong(p, 0, cache_opt);
            __builtin_IB_lsc_prefetch_global_ulong((global ulong*)p + 1, 0, cache_opt);
        }
        else if (size > 4)
        {
            __builtin_IB_lsc_prefetch_global_ulong(p, 0, cache_opt);
        }
        else if (size > 2)
        {
            __builtin_IB_lsc_prefetch_global_uint(p, 0, cache_opt);
        }
        else if (size > 1)
        {
            __builtin_IB_lsc_prefetch_global_ushort(p, 0, cache_opt);
        }
        else if (size == 1)
        {
            __builtin_IB_lsc_prefetch_global_uchar(p, 0, cache_opt);
        }
    }
    else
    {
        // aligned, can use vectors
        if (size > 16)
        {
            __builtin_IB_lsc_prefetch_global_uint8(p, 0, cache_opt);
        }
        else if (size > 12)
        {
            __builtin_IB_lsc_prefetch_global_uint4(p, 0, cache_opt);
        }
        else if (size > 8)
        {
            __builtin_IB_lsc_prefetch_global_uint3(p, 0, cache_opt);
        }
        else if (size > 4)
        {
            __builtin_IB_lsc_prefetch_global_ulong(p, 0, cache_opt);
        }
        else if (size > 2)
        {
            __builtin_IB_lsc_prefetch_global_uint(p, 0, cache_opt);
        }
        else if (size > 1)
        {
            __builtin_IB_lsc_prefetch_global_ushort(p, 0, cache_opt);
        }
        else if (size == 1)
        {
            __builtin_IB_lsc_prefetch_global_uchar(p, 0, cache_opt);
        }
    }
}

//Prefetch function

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


#if defined(cl_khr_fp64)
void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double2* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double3* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double4* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double8* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double16* p, int num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}
#endif // defined(cl_khr_fp64)


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global char16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global short16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global int16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global long16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global float16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 3 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global half16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}


#if defined(cl_khr_fp64)
void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double2* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double3* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double4* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double8* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}

void __attribute__((overloadable)) OPTNONE __spirv_ocl_prefetch( global double16* p, long num_elements)
{
#ifdef cl_intel_pvc_lsc_validation
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        LSC_PREFETCH(p, 2 * num_elements);
    }
#endif // cl_intel_pvc_lsc_validation
}
#endif // defined(cl_khr_fp64)

