/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file defines helper builtins of OpenCL VA (Video Analytics) extension functions.
#include "../../../Implementation/IGCBiF_Intrinsics.cl"
#include "../../../Headers/spirv.h"
/*****************************************************************************/
/*                       VA (Video Analytics)                                */
/*****************************************************************************/

INLINE void OVERLOADABLE intel_work_group_va_boolcentroid(
    __local void* dst,
    int2 i_coord,
    int2 size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_boolsum(
    __local void* dst,
    int2 i_coord,
    int2 size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_centroid(
    __local void* dst,
    int2 i_coord,
    int size,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_convolve_16x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_dilate_64x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_erode_64x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_minmax(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_minmaxfilter_16x4(
    __local void* dst,
    int2 i_coord,
    image2d_t image,
    sampler_t a )
{
    // placeholder for future implementation
}

INLINE void OVERLOADABLE intel_work_group_va_boolcentroid(
    __local void* registers,
    float2 coordsNorm,
    int2 size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_boolcentroid(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_boolsum(
    __local void* registers,
    float2 coordsNorm,
    int2 size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_boolsum(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_centroid(
    __local void* registers,
    float2 coordsNorm,
    int size,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_centroid(
        registers,
        coordsNorm,
        size,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_convolve_16x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    __builtin_IB_va_convolve_16x4_SLM(
        registers,
        coordsNorm,
        (long)__builtin_IB_cast_object_to_generic_ptr(srcImg),
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_dilate_64x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_dilate_64x4(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_erode_64x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_erode_64x4(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_minmax(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_minmax(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE void OVERLOADABLE intel_work_group_va_minmaxfilter_16x4(
    __local void* registers,
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    long i_image = (long)__builtin_IB_cast_object_to_generic_ptr( srcImg );
    __builtin_IB_va_minmaxfilter_16x4_SLM(
        registers,
        coordsNorm,
        i_image,
        __builtin_IB_convert_sampler_to_int(accelerator) );
}

INLINE short OVERLOADABLE intel_work_group_va_convolve_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE short4 OVERLOADABLE intel_work_group_va_convolve_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar OVERLOADABLE intel_work_group_va_minfilter_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar4 OVERLOADABLE intel_work_group_va_minfilter_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar OVERLOADABLE intel_work_group_va_maxfilter_16x1(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

INLINE uchar4 OVERLOADABLE intel_work_group_va_maxfilter_16x4(
    float2 coordsNorm,
    image2d_t srcImg,
    sampler_t accelerator )
{
    // placeholder for future implementation
}

