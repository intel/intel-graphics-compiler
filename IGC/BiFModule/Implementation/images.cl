/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Built-in image functions
// These values need to match the runtime equivalent
#define CLK_ADDRESS_NONE              0x00
#define CLK_ADDRESS_CLAMP             0x01
#define CLK_ADDRESS_CLAMP_TO_EDGE     0x02
#define CLK_ADDRESS_REPEAT            0x03
#define CLK_ADDRESS_MIRRORED_REPEAT   0x04

#define CLK_NORMALIZED_COORDS_FALSE   0x00
#define CLK_NORMALIZED_COORDS_TRUE    0x08

#define CLK_FILTER_NEAREST            0x00
#define CLK_FILTER_LINEAR             0x10

#include "IBiF_Header.cl"

__spirv_Sampler __bindless_sampler_initializer(uint sampler);

__spirv_Sampler __translate_sampler_initializer(uint sampler)
{
    if(BIF_FLAG_CTRL_GET(UseBindlessImage))
    {
        return __bindless_sampler_initializer(sampler);
    }
    return __builtin_IB_convert_object_type_to_spirv_sampler((ulong*)sampler);
}

#ifdef __IGC_BUILD__
__attribute__((inline)) float4 __flush_denormals(float4 in)
{
    //temporary fix, since SKL+ support denormal values we need to flush those values to zero after LOAD instructions.
    //AND r6, r8, 0x807fffff;
    //UCMP.eq p0, r6, r8;
    //(p0) AND r8, r8, 0x80000000;
    int4 floatDenorm = (int4)0x807fffff;
    int4 signBit = (int4)0x80000000;
    float4 temp = as_float4(as_int4(in) & floatDenorm);
    return (as_int4(temp) == as_int4(in)) ? as_float4(as_int4(in) & signBit) : in;
}
#endif

// Image Instructions

// ImageSampleExplicitLod samples an image using an explicit level of detail.

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    float2 floatCoords = convert_float2(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if (Lod == 0.0f)
    {
        // Even though this is a SPIRV builtin, the runtime still patches OCL C enum values for the addressing mode.
        if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
        {
            float2 coords = Coordinate;
            if (CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
            {
                float2 dim = __spirv_ConvertUToF_Rfloat2(
                    (uint2)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id)));
                coords = coords * dim;
            }
            int2 intCoords = __spirv_ConvertFToS_Rint2(__spirv_ocl_floor(coords));
            return __builtin_IB_OCL_2d_ldui(image_id, intCoords, 0);
        }
        else
        {
            return as_uint4(__builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate, 0.0f));
        }
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate, Lod));
    }
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    float float_lod = __spirv_ConvertFToS_Rint(Lod);
    return __builtin_IB_OCL_2d_ldui(image_id, Coordinate, float_lod);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}
#endif // cl_khr_fp16

float4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_3D SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

float4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_3D SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    float3 floatCoords = convert_float3(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if (Lod == 0.0f)
    {
        if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
        {
            if (CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
            {
                float3 dim = (float3)((float)__builtin_IB_get_image_width(image_id), (float)__builtin_IB_get_image_height(image_id), (float)__builtin_IB_get_image_depth(image_id));
                Coordinate = Coordinate * dim;
            }
            int3 intCoords = __spirv_ConvertFToS_Rint3(__spirv_ocl_floor(Coordinate));
            return __builtin_IB_OCL_3d_ldui(image_id, intCoords, 0);
        }
        else
        {
            return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate, 0.0f));
        }
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate, Lod));
    }
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_3D SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

int4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_3D SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
    return as_int4(res);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_3D SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float3 floatCoords = convert_float3(Coordinate);
        return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, floatCoords, Lod));
    }
    else
    {
        int int_lod = __spirv_ConvertFToS_Rint(Lod);
        return __builtin_IB_OCL_3d_ldui(image_id, Coordinate, int_lod);
    }
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

int4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_3D SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

#ifdef cl_khr_fp16
half4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_3D SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rhalf4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

half4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_3D SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rhalf4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}
#endif // cl_khr_fp16

float4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, Coordinate, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

float4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    float3 floatCoords = convert_float3(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D_array SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if (Lod == 0.0f)
    {
        int dt = __builtin_IB_get_image2d_array_size(image_id);
        float layer = __spirv_ocl_fclamp(__spirv_ocl_rint(Coordinate.z), 0.0f, (float)(dt - 1));
        if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
        {
            float2 tmpCoords = Coordinate.xy;
            if (CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
            {
                float2 dim = __spirv_ConvertUToF_Rfloat2((uint2)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id)));
                tmpCoords = Coordinate.xy * dim;
            }
            int3 intCoords = __spirv_ConvertFToS_Rint3((float3)(__spirv_ocl_floor(tmpCoords), layer));
            return __builtin_IB_OCL_2darr_ldui(image_id, intCoords, 0);
        }
        else
        {
            return as_uint4(__builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, Coordinate, 0.0f));
        }
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, Coordinate, Lod));
    }
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D_array SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

int4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D_array SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D_array SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D_array SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    float float_lod = __spirv_ConvertFToS_Rint(Lod);
    int dt = __builtin_IB_get_image2d_array_size(image_id);
    float layer = __spirv_ocl_fclamp(__spirv_ocl_rint((float)Coordinate.z), 0.0f, (float)(dt - 1));
    return __builtin_IB_OCL_2darr_ldui(image_id, (int3)(Coordinate.xy, (int)layer), float_lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_2D_array SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

int4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D_array SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_2D_array SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

#ifdef cl_khr_fp16
half4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D_array SampledImage, float3 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D_array SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rhalf4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}

half4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D_array SampledImage, int3 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_2D_array SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    return __spirv_ImageSampleExplicitLod_Rhalf4(SampledImage, Coordinate.xyz, ImageOperands, Lod);
}
#endif // cl_khr_fp16

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_1d_sample_l(image_id, sampler_id, Coordinate, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);
    if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float floatCoords = convert_float((Coordinate));
        return __builtin_IB_OCL_1d_sample_l(image_id, sampler_id, floatCoords, Lod);
    }
    else
    {
        float4 res = __builtin_IB_OCL_1d_ld(image_id, Coordinate, 0);
        return __flush_denormals(res);
    }
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    // TODO: Why do we go through 3D builtins for 1D image?

    float4 Coordinate4 = (float4)(Coordinate, 0, 0, 0);
    if (Lod == 0.0f)
    {
        if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
        {
            if (CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
            {
                float4 dim = __spirv_ConvertUToF_Rfloat4(
                    (uint4)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id), __builtin_IB_get_image_depth(image_id), 0));
                Coordinate4 = Coordinate4 * dim;
            }
            int4 intCoords = __spirv_ConvertFToS_Rint4(__spirv_ocl_floor(Coordinate4));
            return __builtin_IB_OCL_3d_ldui(image_id, intCoords.xyz, 0);
        }
        else
        {
            return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate4.xyz, 0.0f));
        }
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate4.xyz, Lod));
    }
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float floatCoords = convert_float((Coordinate));
        return as_uint4(__builtin_IB_OCL_1d_sample_l(image_id, sampler_id, floatCoords, Lod));
    }
    else
    {
        float float_lod = __spirv_ConvertFToS_Rint(Lod);
        return __builtin_IB_OCL_1d_ldui(image_id, Coordinate, float_lod);
    }
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}
#endif // cl_khr_fp16

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_1D_array SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, (float2)(Coordinate.x, Coordinate.y), Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_1D_array SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    float2 floatCoords = convert_float2(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_1D_array SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    if (Lod == 0.0f)
    {
        int dt = __builtin_IB_get_image1d_array_size(image_id);
        float layer = __spirv_ocl_fclamp(__spirv_ocl_rint(Coordinate.y), 0.0f, (float)(dt - 1));
        if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
        {
            float tmpCoords = Coordinate.x;
            if (CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
            {
                float width = (float)(__builtin_IB_get_image_width(image_id));
                tmpCoords = (float)(width * Coordinate.x);
            }
            int2 intCoords = __spirv_ConvertFToS_Rint2((float2)(__spirv_ocl_floor(tmpCoords), layer));
            return __builtin_IB_OCL_1darr_ldui(image_id, intCoords, 0);
        }
        else
        {
            return as_uint4(__builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, Coordinate.xy, 0.0f));
        }
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, Coordinate.xy, Lod));
    }
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_1D_array SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(__spirv_SampledImage_1D_array SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    float float_lod = __spirv_ConvertFToS_Rint(Lod);
    int dt = __builtin_IB_get_image1d_array_size(image_id);
    float layer = __spirv_ocl_fclamp(__spirv_ocl_rint((float)Coordinate.y), 0.0f, (float)(dt - 1));
    return __builtin_IB_OCL_1darr_ldui(image_id, (int2)(Coordinate.x, (int)layer), float_lod);
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(__spirv_SampledImage_1D_array SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    uint4 res = __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, Lod);
    return as_int4(res);
}

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_1D_array SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(__spirv_SampledImage_1D_array SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    float4 res = __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, Lod);
    return __spirv_FConvert_Rhalf4(res);
}
#endif // cl_khr_fp16

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_depth SampledImage, float2 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_depth SampledImage, int2 Coordinate, int ImageOperands, float Lod)
{
    float2 floatCoords = convert_float2(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array_depth SampledImage, float4 Coordinate, int ImageOperands, float Lod)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, (float3)(Coordinate.xy, Coordinate.z), Lod);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(__spirv_SampledImage_2D_array_depth SampledImage, int4 Coordinate, int ImageOperands, float Lod)
{
    float4 floatCoords = convert_float4(Coordinate);
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, floatCoords, ImageOperands, Lod);
}

// Gradient overloads

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2d_sample_d(image_id, sampler_id, Coordinate.xy, dx, dy);
}

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_2D SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    return convert_half4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}
#endif // cl_khr_fp16

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_2D SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    return as_uint4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    return as_int4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

float4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_3D SampledImage,
    float3 Coordinate,
    int ImageOperands,
    float3 dx,
    float3 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_3d_sample_d(image_id, sampler_id, Coordinate, dx, dy);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_3D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float4 dx,
    float4 dy)
{
    return __spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate.xyz, ImageOperands, dx.xyz, dy.xyz);
}

#ifdef cl_khr_fp16
half4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_3D SampledImage,
    float3 Coordinate,
    int ImageOperands,
    float3 dx,
    float3 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    float4 res = __builtin_IB_OCL_3d_sample_d(image_id, sampler_id, Coordinate, dx, dy);
    return __spirv_FConvert_Rhalf4(res);
}

half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_3D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float4 dx,
    float4 dy)
{
    return __spirv_ImageSampleExplicitLod_Rhalf4(SampledImage, Coordinate.xyz, ImageOperands, dx.xyz, dy.xyz);
}
#endif // cl_khr_fp16

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_3D SampledImage,
    float3 Coordinate,
    int ImageOperands,
    float3 dx,
    float3 dy)
{
    return as_uint4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_3D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float4 dx,
    float4 dy)
{
    return __spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate.xyz, ImageOperands, dx.xyz, dy.xyz);
}

int4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_3D SampledImage,
    float3 Coordinate,
    int ImageOperands,
    float3 dx,
    float3 dy)
{
    return as_int4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_3D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float4 dx,
    float4 dy)
{
    return __spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate.xyz, ImageOperands, dx.xyz, dy.xyz);
}

ushort4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rushort4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float3 dx, float3 dy)
{
    return convert_ushort4(__spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

short4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rshort4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float3 dx, float3 dy)
{
    return convert_short4(__spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

uchar4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruchar4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float3 dx, float3 dy)
{
    return convert_uchar4(__spirv_ImageSampleExplicitLod_Ruint4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

char4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Rchar4(__spirv_SampledImage_3D SampledImage, float3 Coordinate, int ImageOperands, float3 dx, float3 dy)
{
    return convert_char4(__spirv_ImageSampleExplicitLod_Rint4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2darr_sample_d(image_id, sampler_id, Coordinate.xyz, dx, dy);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_2D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    return as_uint4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    return as_int4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D SampledImage,
    float Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_1d_sample_d(image_id, sampler_id, Coordinate, dx, dy);
}

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_1D SampledImage,
    float Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    return convert_half4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}
#endif // cl_khr_fp16

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_1D SampledImage,
    float Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    return as_uint4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D SampledImage,
    float Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    return as_int4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D_array SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_1darr_sample_d(image_id, sampler_id, Coordinate, dx, dy);
}

uint4 OVERLOADABLE __spirv_ImageSampleExplicitLod_Ruint4(
    __spirv_SampledImage_1D_array SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    return as_uint4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D_array SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float dx,
    float dy)
{
    return as_int4(__spirv_ImageSampleExplicitLod_Rfloat4(SampledImage, Coordinate, ImageOperands, dx, dy));
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_depth SampledImage,
    float2 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2d_sample_d(image_id, sampler_id, Coordinate, dx, dy);
}

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array_depth SampledImage,
    float4 Coordinate,
    int ImageOperands,
    float2 dx,
    float2 dy)
{
    long image_id = (long)__builtin_IB_get_image(SampledImage);
    long sampler_id = (long)__builtin_IB_get_sampler(SampledImage);

    return __builtin_IB_OCL_2darr_sample_d(image_id, sampler_id, Coordinate.xyz, dx, dy);
}

// Image SampleExplicitLod SYCL Bindless

#define DefaultImageOperands 2
#define DefaultLod 0.f

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C_TY(DIM, ARRAY, COORD_DIM, RET_TYPE, LOAD_TYPE)                                                                       \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                                             \
    return convert_##RET_TYPE##4(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod));                                                         \
}                                                                                                                                                                             \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands, float Lod)   \
{                                                                                                                                                                             \
    return convert_##RET_TYPE##4(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod));                                                         \
}

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(DIM, ARRAY, COORD_DIM)           \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C_TY(DIM, ARRAY, COORD_DIM, char, int4)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C_TY(DIM, ARRAY, COORD_DIM, uchar, uint4)  \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C_TY(DIM, ARRAY, COORD_DIM, short, int4)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C_TY(DIM, ARRAY, COORD_DIM, ushort, uint4)

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C_TY(DIM, RET_TYPE, LOAD_TYPE, COORD_DIM)                                                                                                \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D Image, float##COORD_DIM Coordinate, int ImageOperands, float##COORD_DIM dx, float##COORD_DIM dy) \
{                                                                                                                                                                                                     \
    return convert_##RET_TYPE##4(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, dx, dy));                                                                              \
}

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C(DIM, COORD_DIM)           \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C_TY(DIM, char, int4, COORD_DIM)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C_TY(DIM, short, int4, COORD_DIM)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C_TY(DIM, uchar, uint4, COORD_DIM)  \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C_TY(DIM, ushort, uint4, COORD_DIM)

DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(3,       , 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(2,       , 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(2, _array, 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(1,       ,  )
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_S_C(1, _array, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C(2, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_S_C(1,  )

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, RET_TYPE, LOAD_TYPE, COORD_DIM)                                                                \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands)   \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate)             \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE##4 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate)               \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##4(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE##3(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).xyz);                                   \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE##3(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).xyz);                                   \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands)   \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate)             \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate)               \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE##2(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).xy);                                    \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE##2(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).xy);                                    \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands)   \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(Image, Coordinate, ImageOperands, DefaultLod);                                                     \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate)             \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate)               \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(Image, Coordinate, DefaultImageOperands, DefaultLod);                                              \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).x);                                        \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands, float Lod) \
{                                                                                                                                                           \
    return convert_##RET_TYPE(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, Lod).x);                                        \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE(Image, Coordinate, ImageOperands, DefaultLod);                                                        \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate, int ImageOperands)  \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE(Image, Coordinate, ImageOperands, DefaultLod);                                                        \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, float##COORD_DIM Coordinate)                   \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE(Image, Coordinate, DefaultImageOperands, DefaultLod);                                                 \
}                                                                                                                                                           \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D##ARRAY Image, int##COORD_DIM Coordinate)                     \
{                                                                                                                                                           \
    return __spirv_ImageSampleExplicitLod_R##RET_TYPE(Image, Coordinate, DefaultImageOperands, DefaultLod);                                                 \
}

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, RET_TYPE, LOAD_TYPE, COORD_DIM)                                                                                       \
RET_TYPE##3 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##3(__spirv_SampledImage_##DIM##D Image, float##COORD_DIM Coordinate, int ImageOperands, float##COORD_DIM dx, float##COORD_DIM dy) \
{                                                                                                                                                                                        \
    return convert_##RET_TYPE##3(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, dx, dy).xyz);                                                             \
}                                                                                                                                                                                        \
RET_TYPE##2 OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE##2(__spirv_SampledImage_##DIM##D Image, float##COORD_DIM Coordinate, int ImageOperands, float##COORD_DIM dx, float##COORD_DIM dy) \
{                                                                                                                                                                                        \
    return convert_##RET_TYPE##2(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, dx, dy).xy);                                                              \
}                                                                                                                                                                                        \
RET_TYPE OVERLOADABLE __spirv_ImageSampleExplicitLod_R##RET_TYPE(__spirv_SampledImage_##DIM##D Image, float##COORD_DIM Coordinate, int ImageOperands, float##COORD_DIM dx, float##COORD_DIM dy) \
{                                                                                                                                                                                        \
    return convert_##RET_TYPE(__spirv_ImageSampleExplicitLod_R##LOAD_TYPE(Image, Coordinate, ImageOperands, dx, dy).x);                                                                  \
}

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(DIM, ARRAY, COORD_DIM)           \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, char, int4, COORD_DIM)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, short, int4, COORD_DIM)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, int, int4, COORD_DIM)     \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, uchar, uint4, COORD_DIM)  \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, ushort, uint4, COORD_DIM) \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, uint, uint4, COORD_DIM)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, float, float4, COORD_DIM)

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY(DIM, COORD_DIM)           \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, char, int4, COORD_DIM)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, short, int4, COORD_DIM)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, int, int4, COORD_DIM)     \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, uchar, uint4, COORD_DIM)  \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, ushort, uint4, COORD_DIM) \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, uint, uint4, COORD_DIM)   \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, float, float4, COORD_DIM)

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(DIM, ARRAY, COORD_DIM)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_TY(DIM, ARRAY, half, half4, COORD_DIM)

#define DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_HALF(DIM, COORD_DIM)    \
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_TY(DIM, half, half4, COORD_DIM)

DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(3,       , 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(2,       , 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(2, _array, 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(1,       ,  )
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD(1, _array, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY(2, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY(1,  )

#ifdef cl_khr_fp16
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(3,       , 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(2,       , 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(2, _array, 3)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(1,       ,  )
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_HALF(1, _array, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_HALF(2, 2)
DEF_SYCL_BINDLESS_SAMPLED_IMAGE_EXPLICIT_LOD_DX_DY_HALF(1,  )
#endif // cl_khr_fp16

#undef DefaultImageOperands
#undef DefaultLod

// Image Read

#define DEF_IMAGE_READ_2D(ACC_QUAL)                                                                                 \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_##ACC_QUAL* Image, int2 Coordinate)                        \
{                                                                                                                   \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                    \
    return __builtin_IB_OCL_2d_ldui_##ACC_QUAL(id, Coordinate, 0);                                                  \
}                                                                                                                   \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_##ACC_QUAL* Image, int2 Coordinate, int ImageOperands)     \
{                                                                                                                   \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                             \
}                                                                                                                   \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_##ACC_QUAL* Image, int2 Coordinate)   \
{                                                                                                                   \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                        \
    return as_int4(res);                                                                                            \
}                                                                                                                   \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_##ACC_QUAL* Image, int2 Coordinate, int ImageOperands)   \
{                                                                                                                   \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                    \
}                                                                                                                   \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_##ACC_QUAL* Image, int2 Coordinate)  \
{                                                                                                                   \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                 \
    float4 res = __builtin_IB_OCL_2d_ld_##ACC_QUAL(id, Coordinate, 0);                                              \
    return __flush_denormals(res);                                                                                  \
}                                                                                                                   \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_##ACC_QUAL* Image, int2 Coordinate, int ImageOperands)  \
{                                                                                                                   \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                  \
}

#define DEF_IMAGE_READ_3D(ACC_QUAL)                                                                                 \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img3d_##ACC_QUAL* Image, int3 Coordinate)                        \
{                                                                                                                   \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                 \
    return __builtin_IB_OCL_3d_ldui_##ACC_QUAL( id, Coordinate, 0);                                                 \
}                                                                                                                   \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img3d_##ACC_QUAL* Image, int4 Coordinate)                        \
{                                                                                                                   \
    return __spirv_ImageRead_Ruint4(Image, Coordinate.xyz);                                                         \
}                                                                                                                   \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img3d_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)     \
{                                                                                                                   \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                             \
}                                                                                                                   \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img3d_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands)     \
{                                                                                                                   \
    return __spirv_ImageRead_Ruint4(Image, Coordinate.xyz);                                                         \
}                                                                                                                   \
int4 OVERLOADABLE __spirv_ImageRead_Rint4(global Img3d_##ACC_QUAL* Image, int3 Coordinate)                          \
{                                                                                                                   \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                        \
    return as_int4(res);                                                                                            \
}                                                                                                                   \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img3d_##ACC_QUAL* Image, int4 Coordinate)   \
{                                                                                                                   \
    return __spirv_ImageRead_Rint4(Image, Coordinate.xyz);                                                          \
}                                                                                                                   \
int4 OVERLOADABLE __spirv_ImageRead_Rint4(global Img3d_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)       \
{                                                                                                                   \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                                                              \
}                                                                                                                   \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img3d_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands)   \
{                                                                                                                   \
    return __spirv_ImageRead_Rint4(Image, Coordinate.xyz);                                                          \
}                                                                                                                   \
float4 OVERLOADABLE __spirv_ImageRead_Rfloat4(global Img3d_##ACC_QUAL* Image, int3 Coordinate)                      \
{                                                                                                                   \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                 \
    float4 res = __builtin_IB_OCL_3d_ld_##ACC_QUAL(id, Coordinate, 0);                                              \
    return __flush_denormals(res);                                                                                  \
}                                                                                                                   \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img3d_##ACC_QUAL* Image, int4 Coordinate) \
{                                                                                                                   \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate.xyz);                                                        \
}                                                                                                                   \
float4 OVERLOADABLE __spirv_ImageRead_Rfloat4(global Img3d_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)   \
{                                                                                                                   \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                                                            \
}                                                                                                                   \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img3d_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands) \
{                                                                                                                   \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate.xyz);                                                        \
}

#define DEF_IMAGE_READ_2D_ARRAY(ACC_QUAL)                                                                                        \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate)                               \
{                                                                                                                                \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                              \
    return __builtin_IB_OCL_2darr_ldui_##ACC_QUAL(id, Coordinate, 0);                                                            \
}                                                                                                                                \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate)                               \
{                                                                                                                                \
    return __spirv_ImageRead_Ruint4(Image, Coordinate.xyz);                                                                      \
}                                                                                                                                \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)            \
{                                                                                                                                \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                          \
}                                                                                                                                \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands)            \
{                                                                                                                                \
    return __spirv_ImageRead_Ruint4(Image, Coordinate.xyz);                                                                      \
}                                                                                                                                \
int4 OVERLOADABLE __spirv_ImageRead_Rint4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate)                                 \
{                                                                                                                                \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                     \
    return as_int4(res);                                                                                                         \
}                                                                                                                                \
int4 OVERLOADABLE __spirv_ImageRead_Rint4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)              \
{                                                                                                                                \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                                                                           \
}                                                                                                                                \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate)    \
{                                                                                                                                \
    return __spirv_ImageRead_Rint4(Image, Coordinate.xyz);                                                                       \
}                                                                                                                                \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands) \
{                                                                                                                                \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                           \
}                                                                                                                                \
float4 OVERLOADABLE __spirv_ImageRead_Rfloat4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate)                             \
{                                                                                                                                \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                              \
    float4 res = __builtin_IB_OCL_2darr_ld_##ACC_QUAL(id, Coordinate, 0);                                                        \
    return __flush_denormals(res);                                                                                               \
}                                                                                                                                \
float4 OVERLOADABLE __spirv_ImageRead_Rfloat4(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, int ImageOperands)          \
{                                                                                                                                \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                                                                         \
}                                                                                                                                \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate)  \
{                                                                                                                                \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate.xyz);                                                                     \
}                                                                                                                                \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, int ImageOperands) \
{                                                                                                                                \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                         \
}

#define DEF_IMAGE_READ_1D(ACC_QUAL)                                                                               \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_##ACC_QUAL* Image, int Coordinate)                       \
{                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                               \
    return __builtin_IB_OCL_1d_ldui_##ACC_QUAL( id, Coordinate, 0);                                               \
}                                                                                                                 \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_##ACC_QUAL* Image, int Coordinate, int ImageOperands)    \
{                                                                                                                 \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                           \
}                                                                                                                 \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_##ACC_QUAL* Image, int Coordinate)    \
{                                                                                                                 \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                      \
    return as_int4(res);                                                                                          \
}                                                                                                                 \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_##ACC_QUAL* Image, int Coordinate, int ImageOperands)    \
{                                                                                                                 \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                    \
}                                                                                                                 \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_##ACC_QUAL* Image, int Coordinate)  \
{                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                               \
    float4 res = __builtin_IB_OCL_1d_ld_##ACC_QUAL(id, Coordinate, 0);                                            \
    return __flush_denormals(res);                                                                                \
}                                                                                                                 \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_##ACC_QUAL* Image, int Coordinate, int ImageOperands)  \
{                                                                                                                 \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                  \
}

#define DEF_IMAGE_READ_1D_BUFFER(ACC_QUAL)                                                                                      \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate)                              \
{                                                                                                                               \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                             \
    return __builtin_IB_OCL_1d_ldui_##ACC_QUAL( id, Coordinate, 0);                                                             \
}                                                                                                                               \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, int ImageOperands)           \
{                                                                                                                               \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                         \
}                                                                                                                               \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate)    \
{                                                                                                                               \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                    \
    return as_int4(res);                                                                                                        \
}                                                                                                                               \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, int ImageOperands) \
{                                                                                                                               \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                           \
}                                                                                                                               \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate)  \
{                                                                                                                               \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                             \
    float4 res = __builtin_IB_OCL_1d_ld_##ACC_QUAL(id, Coordinate, 0);                                                          \
    return __flush_denormals(res);                                                                                              \
}                                                                                                                               \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, int ImageOperands) \
{                                                                                                                               \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                         \
}

#define DEF_IMAGE_READ_1D_ARRAY(ACC_QUAL)                                                                                       \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate)                              \
{                                                                                                                               \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                             \
    return __builtin_IB_OCL_1darr_ldui_##ACC_QUAL( id, Coordinate, 0);                                                          \
}                                                                                                                               \
uint4 OVERLOADABLE __spirv_ImageRead_Ruint4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, int ImageOperands)           \
{                                                                                                                               \
    return __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                         \
}                                                                                                                               \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate)   \
{                                                                                                                               \
    uint4 res = __spirv_ImageRead_Ruint4(Image, Coordinate);                                                                    \
    return as_int4(res);                                                                                                        \
}                                                                                                                               \
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, int ImageOperands) \
{                                                                                                                               \
    return __spirv_ImageRead_Rint4(Image, Coordinate);                          \
}                                                                                                                               \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate)  \
{                                                                                                                               \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                             \
    float4 res = __builtin_IB_OCL_1darr_ld_##ACC_QUAL(id, Coordinate, 0);                                                       \
    return __flush_denormals(res);                                                                                              \
}                                                                                                                               \
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, int ImageOprands) \
{                                                                                                                               \
    return __spirv_ImageRead_Rfloat4(Image, Coordinate);                        \
}

#define DEF_IMAGE_READ_2D_DEPTH(ACC_QUAL)                                                                                      \
float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(global Img2d_depth_##ACC_QUAL* Image, int2 Coordinate)    \
{                                                                                                                              \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                            \
    float4 res = __builtin_IB_OCL_2d_ld_##ACC_QUAL(id, Coordinate, 0);                                                         \
    return __flush_denormals(res).x;                                                                                           \
}

#define DEF_IMAGE_READ_2D_ARRAY_DEPTH(ACC_QUAL)                                                                                          \
float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(global Img2d_array_depth_##ACC_QUAL* Image, int4 Coordinate)  \
{                                                                                                                                        \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                      \
    float4 res = __builtin_IB_OCL_2darr_ld_##ACC_QUAL(id, Coordinate.xyz, 0);                                                            \
    return __flush_denormals(res).x;                                                                                                     \
}

DEF_IMAGE_READ_2D(ro)
DEF_IMAGE_READ_2D(rw)
DEF_IMAGE_READ_3D(ro)
DEF_IMAGE_READ_3D(rw)
DEF_IMAGE_READ_2D_ARRAY(ro)
DEF_IMAGE_READ_2D_ARRAY(rw)
DEF_IMAGE_READ_1D(ro)
DEF_IMAGE_READ_1D(rw)
DEF_IMAGE_READ_1D_BUFFER(ro)
DEF_IMAGE_READ_1D_BUFFER(rw)
DEF_IMAGE_READ_1D_ARRAY(ro)
DEF_IMAGE_READ_1D_ARRAY(rw)
DEF_IMAGE_READ_2D_DEPTH(ro)
DEF_IMAGE_READ_2D_DEPTH(rw)
DEF_IMAGE_READ_2D_ARRAY_DEPTH(ro)
DEF_IMAGE_READ_2D_ARRAY_DEPTH(rw)

#ifdef cl_khr_fp16

#define DEF_HALF_IMAGE_READ(IMAGE_TYPE, COORDS_TYPE, COORDS_TYPE_ABBR)                                                                                         \
half4 __attribute__((overloadable)) __spirv_ImageRead_Rhalf4(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate) \
{                                                                                                                                                              \
    return __spirv_FConvert_Rhalf4(__spirv_ImageRead_Rfloat4(Image, Coordinate)); \
}

DEF_HALF_IMAGE_READ(2d_ro, int2, v2i32)
DEF_HALF_IMAGE_READ(2d_rw, int2, v2i32)
DEF_HALF_IMAGE_READ(3d_ro, int4, v4i32)
DEF_HALF_IMAGE_READ(3d_rw, int4, v4i32)
DEF_HALF_IMAGE_READ(2d_array_ro, int4, v4i32)
DEF_HALF_IMAGE_READ(2d_array_rw, int4, v4i32)
DEF_HALF_IMAGE_READ(1d_ro, int, i32)
DEF_HALF_IMAGE_READ(1d_rw, int, i32)
DEF_HALF_IMAGE_READ(1d_buffer_ro, int, i32)
DEF_HALF_IMAGE_READ(1d_buffer_rw, int, i32)
DEF_HALF_IMAGE_READ(1d_array_ro, int2, v2i32)
DEF_HALF_IMAGE_READ(1d_array_rw, int2, v2i32)

#define DEF_HALF_IMAGE_READ_COORD3(IMAGE_TYPE)                                                           \
half4 OVERLOADABLE __spirv_ImageRead_Rhalf4(global Img##IMAGE_TYPE* Image, int3 Coordinate)              \
{                                                                                                        \
    return __spirv_FConvert_Rhalf4(__spirv_ImageRead_Rfloat4(Image, Coordinate)); \
}

DEF_HALF_IMAGE_READ_COORD3(2d_array_ro)
DEF_HALF_IMAGE_READ_COORD3(2d_array_rw)
DEF_HALF_IMAGE_READ_COORD3(3d_ro)
DEF_HALF_IMAGE_READ_COORD3(3d_rw)

#endif // cl_khr_fp16

// Image Read SYCL Bindless Unsampled

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, TY)                                                   \
TY OVERLOADABLE __spirv_ImageRead_R##TY(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                          \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##4(Image, Coordinate).x;                                                                             \
}                                                                                                                                       \
TY OVERLOADABLE __spirv_ImageRead_R##TY(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands)       \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY(Image, Coordinate);                                                                                  \
}                                                                                                                                       \
TY##2 OVERLOADABLE __spirv_ImageRead_R##TY##2(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                    \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##4(Image, Coordinate).xy;                                                                            \
}                                                                                                                                       \
TY##2 OVERLOADABLE __spirv_ImageRead_R##TY##2(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##2(Image, Coordinate);                                                                               \
}                                                                                                                                       \
TY##3 OVERLOADABLE __spirv_ImageRead_R##TY##3(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                    \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##4(Image, Coordinate).xyz;                                                                           \
}                                                                                                                                       \
TY##3 OVERLOADABLE __spirv_ImageRead_R##TY##3(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##3(Image, Coordinate);                                                                               \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(DIM, ARRAY_ACC_QUAL, COORD_DIM)   \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, int  ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, uint ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, float)

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(DIM, ARRAY_ACC_QUAL, COORD_DIM) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, half  )   \

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, TY, UNSIGNED)                                        \
TY OVERLOADABLE __spirv_ImageRead_R##TY(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                          \
{                                                                                                                                       \
    return convert_##TY(__spirv_ImageRead_R##UNSIGNED##int(Image, Coordinate));                                                         \
}                                                                                                                                       \
TY OVERLOADABLE __spirv_ImageRead_R##TY(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands)       \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY(Image, Coordinate);                                                                                  \
}                                                                                                                                       \
TY##2 OVERLOADABLE __spirv_ImageRead_R##TY##2(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                    \
{                                                                                                                                       \
    return convert_##TY##2(__spirv_ImageRead_R##UNSIGNED##int2(Image, Coordinate));                                                     \
}                                                                                                                                       \
TY##2 OVERLOADABLE __spirv_ImageRead_R##TY##2(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##2(Image, Coordinate);                                                                               \
}                                                                                                                                       \
TY##3 OVERLOADABLE __spirv_ImageRead_R##TY##3(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                    \
{                                                                                                                                       \
    return convert_##TY##3(__spirv_ImageRead_R##UNSIGNED##int3(Image, Coordinate));                                                     \
}                                                                                                                                       \
TY##3 OVERLOADABLE __spirv_ImageRead_R##TY##3(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##3(Image, Coordinate);                                                                               \
}                                                                                                                                       \
TY##4 OVERLOADABLE __spirv_ImageRead_R##TY##4(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate)                    \
{                                                                                                                                       \
    return convert_##TY##4(__spirv_ImageRead_R##UNSIGNED##int4(Image, Coordinate));                                                     \
}                                                                                                                                       \
TY##4 OVERLOADABLE __spirv_ImageRead_R##TY##4(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, int ImageOperands) \
{                                                                                                                                       \
    return __spirv_ImageRead_R##TY##4(Image, Coordinate);                                                                               \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(DIM, ARRAY_ACC_QUAL, COORD_DIM)     \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, short,   ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, ushort, u) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, char,    ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, uchar,  u)

DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(1, ro,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(1, rw,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(1, array_ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(2, ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(2, array_ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(3, ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ(3, rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(1, ro,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(1, rw,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(1, array_ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(2, ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(2, array_ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(3, ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_SC(3, rw, 3)

#ifdef cl_khr_fp16
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(1, ro,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(1, rw,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(1, array_ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(2, ro, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(2, array_ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(3, ro, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_READ_HALF(3, rw, 3)
#endif // cl_khr_fp16

// Image Read MSAA

int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_msaa_ro* Image, int2 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, Coordinate);
    return as_int4(__builtin_IB_OCL_2d_ld2dmsui(id, Coordinate, Sample, mcs));
}

float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_msaa_ro* Image, int2 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, Coordinate);
    float4 res = __builtin_IB_OCL_2d_ld2dms(id, Coordinate, Sample, mcs);
    return __flush_denormals(res);
}

int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(global Img2d_array_msaa_ro* Image, int4 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, Coordinate);
    return as_int4(__builtin_IB_OCL_2darr_ld2dmsui(id, Coordinate, Sample, mcs));
}

float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(global Img2d_array_msaa_ro* Image, int4 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, Coordinate);
    float4 res = __builtin_IB_OCL_2darr_ld2dms(id, Coordinate, Sample, mcs);
    return __flush_denormals(res);
}

float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(global Img2d_msaa_depth_ro* Image, int2 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, Coordinate);
    float4 res = __builtin_IB_OCL_2d_ld2dms(id, Coordinate, Sample, mcs);
    return __flush_denormals(res).x;
}

float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(global Img2d_array_msaa_depth_ro* Image, int4 Coordinate, int ImageOperands, int Sample)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, Coordinate);
    float4 res = __builtin_IB_OCL_2darr_ld2dms(id, Coordinate, Sample, mcs);
    return __flush_denormals(res).x;
}

// Image Write

#define DEF_IMAGE_WRITE_2D(ACC_QUAL)                                                                                              \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, uint Texel)                                 \
{                                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                               \
    __builtin_IB_write_2d_u1i(id, Coordinate, Texel, 0);                                                                          \
}                                                                                                                                 \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, uint2 Texel)                                \
{                                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                               \
    __builtin_IB_write_2d_u2i(id, Coordinate, Texel, 0);                                                                          \
}                                                                                                                                 \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, uint3 Texel)                                \
{                                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                               \
    __builtin_IB_write_2d_u3i(id, Coordinate, Texel, 0);                                                                          \
}                                                                                                                                 \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, uint4 Texel)                                \
{                                                                                                                                 \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                               \
    __builtin_IB_write_2d_u4i(id, Coordinate, Texel, 0);                                                                          \
}                                                                                                                                 \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, int4 Texel)     \
{                                                                                                                                 \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                       \
}                                                                                                                                 \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, float4 Texel)   \
{                                                                                                                                 \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                       \
}                                                                                                                                 \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, uint4 Texel, int ImageOperands)             \
{                                                                                                                                 \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                 \
}                                                                                                                                 \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, int4 Texel, int ImageOperands)   \
{                                                                                                                                 \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                       \
}                                                                                                                                 \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_##ACC_QUAL* Image, int2 Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                                 \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                       \
}

#define DEF_IMAGE_WRITE_2D_ARRAY(ACC_QUAL)                                                                                                   \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, uint Texel)                                      \
{                                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                          \
    __builtin_IB_write_2darr_u1i(id, Coordinate, Texel, 0);                                                                                  \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, uint2 Texel)                                     \
{                                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                          \
    __builtin_IB_write_2darr_u2i(id, Coordinate, Texel, 0);                                                                                  \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, uint3 Texel)                                     \
{                                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                          \
    __builtin_IB_write_2darr_u3i(id, Coordinate, Texel, 0);                                                                                  \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, uint4 Texel)                                     \
{                                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                          \
    __builtin_IB_write_2darr_u4i(id, Coordinate, Texel, 0);                                                                                  \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, uint4 Texel)                                     \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                        \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, int4 Texel)                                      \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                  \
}                                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, int4 Texel)    \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                        \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, float4 Texel)                                    \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                  \
}                                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, float4 Texel)  \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                        \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, uint4 Texel, int ImageOperands)                  \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                            \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, uint4 Texel, int ImageOperands)                  \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                            \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, int4 Texel, int ImageOperands)                   \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                            \
}                                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, int4 Texel, int ImageOperands)   \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                            \
}                                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int3 Coordinate, float4 Texel, int ImageOperands)                 \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                            \
}                                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_##ACC_QUAL* Image, int4 Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                            \
}

#define DEF_IMAGE_WRITE_1D(ACC_QUAL)                                                                                         \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, uint Texel)                             \
{                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                          \
    __builtin_IB_write_1d_u1i(id, Coordinate, Texel, 0);                                                                     \
}                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, uint2 Texel)                            \
{                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                          \
    __builtin_IB_write_1d_u2i(id, Coordinate, Texel, 0);                                                                     \
}                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, uint3 Texel)                            \
{                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                          \
    __builtin_IB_write_1d_u3i(id, Coordinate, Texel, 0);                                                                     \
}                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, uint4 Texel)                            \
{                                                                                                                            \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                          \
    __builtin_IB_write_1d_u4i(id, Coordinate, Texel, 0);                                                                     \
}                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, int4 Texel)   \
{                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                  \
}                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, float4 Texel) \
{                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                  \
}                                                                                                                            \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, uint4 Texel, int ImageOperands)         \
{                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                            \
}                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, int4 Texel, int ImageOperands)   \
{                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                    \
}                                                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_##ACC_QUAL* Image, int Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                            \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                    \
}

#define DEF_IMAGE_WRITE_1D_BUFFER(ACC_QUAL)                                                                                                \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, uint4 Texel)                                   \
{                                                                                                                                          \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                        \
    __builtin_IB_write_1d_u4i(id, Coordinate, Texel, 0);                                                                                   \
}                                                                                                                                          \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, int4 Texel)   \
{                                                                                                                                          \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                \
}                                                                                                                                          \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, float4 Texel) \
{                                                                                                                                          \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                \
}                                                                                                                                          \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, uint4 Texel, int ImageOperands)                \
{                                                                                                                                          \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                          \
}                                                                                                                                          \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, int4 Texel, int ImageOperands)   \
{                                                                                                                                          \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                           \
}                                                                                                                                          \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_buffer_##ACC_QUAL* Image, int Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                                          \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                           \
}

#define DEF_IMAGE_WRITE_1D_ARRAY(ACC_QUAL)                                                                                                  \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, uint Texel)                                     \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_1darr_u1i(id, Coordinate, Texel, 0);                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, uint2 Texel)                                    \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_1darr_u2i(id, Coordinate, Texel, 0);                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, uint3 Texel)                                    \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_1darr_u3i(id, Coordinate, Texel, 0);                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, uint4 Texel)                                    \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_1darr_u4i(id, Coordinate, Texel, 0);                                                                                 \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, int4 Texel)   \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                 \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, float4 Texel) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, uint4 Texel, int ImageOperands)                 \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                           \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, int4 Texel, int ImageOperands)  \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                           \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img1d_array_##ACC_QUAL* Image, int2 Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                           \
}

#define DEF_IMAGE_WRITE_2D_DEPTH(ACC_QUAL)                                                                                                  \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_depth_##ACC_QUAL * Image, int2 Coordinate, float Texel)   \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_2d_u4i(id, Coordinate, (uint4)(as_uint(Texel), 0, 0, 0), 0);                                                         \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_depth_##ACC_QUAL * Image, int2 Coordinate, float Texel, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                             \
}

#define DEF_IMAGE_WRITE_2D_ARRAY_DEPTH(ACC_QUAL)                                                                                            \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_depth_##ACC_QUAL * Image, int4 Coordinate, float Texel)   \
{                                                                                                                                           \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                         \
    __builtin_IB_write_2darr_u4i(id, Coordinate.xyz, (uint4)(as_uint(Texel), 0, 0, 0), 0);                                                  \
}                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_depth_##ACC_QUAL * Image, int4 Coordinate, float Texel, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                       \
}

#define DEF_IMAGE_WRITE_3D(ACC_QUAL)                                                                                                                  \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, uint Texel)                                                     \
{                                                                                                                                                     \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                                   \
    __builtin_IB_write_3d_u1i(id, Coordinate, Texel, 0);                                                                                              \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, uint2 Texel)                                                    \
{                                                                                                                                                     \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                                   \
    __builtin_IB_write_3d_u2i(id, Coordinate, Texel, 0);                                                                                              \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, uint3 Texel)                                                    \
{                                                                                                                                                     \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                                   \
    __builtin_IB_write_3d_u3i(id, Coordinate, Texel, 0);                                                                                              \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, uint4 Texel)                                                    \
{                                                                                                                                                     \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                                   \
    __builtin_IB_write_3d_u4i(id, Coordinate, Texel, 0);                                                                                              \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, uint4 Texel)                                                    \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                                 \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, int4 Texel)                                                     \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                           \
}                                                                                                                                                     \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, int4 Texel)     \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                           \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, float4 Texel)                                                   \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                           \
}                                                                                                                                                     \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, float4 Texel)   \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel));                                                                                           \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, uint4 Texel, int ImageOperands)                                 \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                                     \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, uint4 Texel, int ImageOperands)                                 \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                                     \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, int4 Texel, int ImageOperands)                                  \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                                     \
}                                                                                                                                                     \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, int4 Texel, int ImageOperands)   \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                                 \
}                                                                                                                                                     \
void OVERLOADABLE __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int3 Coordinate, float4 Texel, int ImageOperands)                                \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                                                     \
}                                                                                                                                                     \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img3d_##ACC_QUAL* Image, int4 Coordinate, float4 Texel, int ImageOperands) \
{                                                                                                                                                     \
    __spirv_ImageWrite(Image, Coordinate.xyz, Texel);                                                                                                 \
}

DEF_IMAGE_WRITE_2D(wo)
DEF_IMAGE_WRITE_2D(rw)
DEF_IMAGE_WRITE_2D_ARRAY(wo)
DEF_IMAGE_WRITE_2D_ARRAY(rw)
DEF_IMAGE_WRITE_1D(wo)
DEF_IMAGE_WRITE_1D(rw)
DEF_IMAGE_WRITE_1D_BUFFER(wo)
DEF_IMAGE_WRITE_1D_BUFFER(rw)
DEF_IMAGE_WRITE_1D_ARRAY(wo)
DEF_IMAGE_WRITE_1D_ARRAY(rw)
DEF_IMAGE_WRITE_2D_DEPTH(wo)
DEF_IMAGE_WRITE_2D_DEPTH(rw)
DEF_IMAGE_WRITE_2D_ARRAY_DEPTH(wo)
DEF_IMAGE_WRITE_2D_ARRAY_DEPTH(rw)
DEF_IMAGE_WRITE_3D(wo)
DEF_IMAGE_WRITE_3D(rw)

// Level of details versions

#define DEF_IMAGE_WRITE_LOD(IMAGE_TYPE, IMAGE_TYPE_ABBR, COORDS_TYPE, COORDS_TYPE_ABBR, COORDS_XYZ)                                  \
void OVERLOADABLE __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate, uint4 Texel, int ImageOperands, int Lod) \
{                                                                                                                                    \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                  \
    __builtin_IB_write_##IMAGE_TYPE_ABBR##_u4i(id, Coordinate COORDS_XYZ, Texel, Lod);                                               \
}                                                                                                                                    \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate, int4 Texel, int ImageOperands, int Lod)   \
{                                                                                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel), ImageOperands, Lod);                                                                                                                             \
}                                                                                                                                                                                                           \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate, float4 Texel, int ImageOperands, int Lod) \
{                                                                                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint4(Texel), ImageOperands, Lod);                                                                                                                             \
}

DEF_IMAGE_WRITE_LOD(2d_wo,       2d,    int2, v2i32,     )
DEF_IMAGE_WRITE_LOD(1d_wo,       1d,    int,  i32,       )
DEF_IMAGE_WRITE_LOD(1d_array_wo, 1darr, int2, v2i32,     )
DEF_IMAGE_WRITE_LOD(2d_array_wo, 2darr, int3, v3i32,     )
DEF_IMAGE_WRITE_LOD(2d_array_wo, 2darr, int4, v4i32, .xyz)
DEF_IMAGE_WRITE_LOD(3d_wo,       3d,    int3, v3i32,     ) // old_mangling variants are unused. They could be removed when old_mangling is deprecated.
DEF_IMAGE_WRITE_LOD(3d_wo,       3d,    int4, v4i32, .xyz)

void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_depth_wo* Image, int2 Coordinate, float Texel, int ImageOperands, int Lod)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    __builtin_IB_write_2d_f(id, Coordinate, Texel, Lod);
}

void __attribute__((overloadable)) __spirv_ImageWrite(global Img2d_array_depth_wo* Image, int4 Coordinate, float Texel, int ImageOperands, int Lod)
{
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);
    __builtin_IB_write_2darr_f(id, Coordinate, Texel, Lod);
}

#ifdef cl_khr_fp16
#define DEF_HALF_IMAGE_WRITE(IMAGE_TYPE, COORDS_TYPE, COORDS_TYPE_ABBR)                                                                                                \
void __attribute__((overloadable)) __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate, half4 Texel) \
{                                                                                                                                                                      \
    __spirv_ImageWrite(Image, Coordinate, __spirv_FConvert_Rfloat4(Texel));             \
}                                                                                                                                                                      \
void OVERLOADABLE __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, COORDS_TYPE Coordinate, half4 Texel, int ImageOperands)                                            \
{                                                                                                                                                                      \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                              \
}

DEF_HALF_IMAGE_WRITE(2d_wo, int2, v2i32)
DEF_HALF_IMAGE_WRITE(2d_rw, int2, v2i32)
DEF_HALF_IMAGE_WRITE(3d_wo, int4, v4i32)
DEF_HALF_IMAGE_WRITE(3d_rw, int4, v4i32)
DEF_HALF_IMAGE_WRITE(2d_array_wo, int4, v4i32)
DEF_HALF_IMAGE_WRITE(2d_array_rw, int4, v4i32)
DEF_HALF_IMAGE_WRITE(1d_wo, int, i32)
DEF_HALF_IMAGE_WRITE(1d_rw, int, i32)
DEF_HALF_IMAGE_WRITE(1d_buffer_wo, int, i32)
DEF_HALF_IMAGE_WRITE(1d_buffer_rw, int, i32)
DEF_HALF_IMAGE_WRITE(1d_array_wo, int2, v2i32)
DEF_HALF_IMAGE_WRITE(1d_array_rw, int2, v2i32)

#define DEF_HALF_IMAGE_WRITE_COORD3(IMAGE_TYPE)                                                                      \
void OVERLOADABLE __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, int3 Coordinate, half4 Texel)                    \
{                                                                                                                    \
    __spirv_ImageWrite(Image, Coordinate, __spirv_FConvert_Rfloat4(Texel));                   \
}                                                                                                                    \
void OVERLOADABLE __spirv_ImageWrite(global Img##IMAGE_TYPE* Image, int3 Coordinate, half4 Texel, int ImageOperands) \
{                                                                                                                    \
    __spirv_ImageWrite(Image, Coordinate, Texel);                                                                    \
}

DEF_HALF_IMAGE_WRITE_COORD3(2d_array_wo)
DEF_HALF_IMAGE_WRITE_COORD3(2d_array_rw)
DEF_HALF_IMAGE_WRITE_COORD3(3d_wo)
DEF_HALF_IMAGE_WRITE_COORD3(3d_rw)

#endif // cl_khr_fp16

// Image Write SYCL Bindless Unsampled

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, TY)                                                      \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY color)                       \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint(color));                                                                                  \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY color, int ImageOperands)    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##2 color)                    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint2(color));                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##2 color, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##3 color)                    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, as_uint3(color));                                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##3 color, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_OP(DIM, ARRAY_ACC_QUAL, COORD_DIM, TY)                                                      \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY color, int ImageOperands)    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##2 color, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##3 color, int ImageOperands) \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, color);                                                                                           \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(DIM, ARRAY_ACC_QUAL, COORD_DIM)   \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, int)   \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_TY(DIM, ARRAY_ACC_QUAL, COORD_DIM, float) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_OP(DIM, ARRAY_ACC_QUAL, COORD_DIM, uint)

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, TY, UNSIGNED)                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY color)                       \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, convert_##UNSIGNED##int(color));                                                                  \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY color, int ImageOperands)    \
{                                                                                                                                           \
    return __spirv_ImageWrite(Image, Coordinate, color);                                                                                    \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##2 color)                    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, convert_##UNSIGNED##int2(color));                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##2 color, int ImageOperands) \
{                                                                                                                                           \
    return __spirv_ImageWrite(Image, Coordinate, color);                                                                                    \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##3 color)                    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, convert_##UNSIGNED##int3(color));                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##3 color, int ImageOperands) \
{                                                                                                                                           \
    return __spirv_ImageWrite(Image, Coordinate, color);                                                                                    \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##4 color)                    \
{                                                                                                                                           \
    __spirv_ImageWrite(Image, Coordinate, convert_##UNSIGNED##int4(color));                                                                 \
}                                                                                                                                           \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, TY##4 color, int ImageOperands) \
{                                                                                                                                           \
    return __spirv_ImageWrite(Image, Coordinate, color);                                                                                    \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(DIM, ARRAY_ACC_QUAL, COORD_DIM)     \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, short,   ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, ushort, u) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, char,    ) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_S_C(DIM, ARRAY_ACC_QUAL, COORD_DIM, uchar,  u)

DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(1, wo, )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(1, rw, )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(1, array_wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(2, wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(2, array_wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(3, wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE(3, rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(1, wo,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(1, rw,  )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(1, array_wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(2, wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(2, array_wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(3, wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_SC(3, rw, 3)

#ifdef cl_khr_fp16

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF_1_2_3(DIM, ARRAY_ACC_QUAL, COORD_DIM)                               \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, half Texel)  \
{                                                                                                                        \
    __spirv_ImageWrite(Image, Coordinate, __spirv_FConvert_Rfloat(Texel));                        \
}                                                                                                                        \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, half2 Texel) \
{                                                                                                                        \
    __spirv_ImageWrite(Image, Coordinate, __spirv_FConvert_Rfloat2(Texel));                       \
}                                                                                                                        \
void OVERLOADABLE __spirv_ImageWrite(global Img##DIM##d_##ARRAY_ACC_QUAL* Image, int##COORD_DIM Coordinate, half3 Texel) \
{                                                                                                                        \
    __spirv_ImageWrite(Image, Coordinate, __spirv_FConvert_Rfloat3(Texel));                       \
}

#define DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(DIM, ARRAY_ACC_QUAL, COORD_DIM) \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF_1_2_3(DIM, ARRAY_ACC_QUAL, COORD_DIM)   \
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_OP(DIM, ARRAY_ACC_QUAL, COORD_DIM, half)

DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(1, wo, )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(1, rw, )
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(1, array_wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(1, array_rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(2, wo, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(2, rw, 2)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(2, array_wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(2, array_rw, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(3, wo, 3)
DEF_SYCL_BINDLESS_UNSAMPLED_IMAGE_WRITE_HALF(3, rw, 3)
#endif // cl_khr_fp16

// Image Query

// From the SPIR spec.  Runtime returns values that match SPIR enum
// values but SPIRV enum values start at 0.

#define CLK_SNORM_INT8 0x10D0
#define CLK_R 0x10B0

// ------------------------OpImageQueryFormat------------------------
// Query the image format of an image created with an Unknown Image Format.

// ------------------------OpImageQueryOrder------------------------
// Query the channel order of an image created with an Unknown Image Format.

#define DEF_IMAGE_QUERY_BUILTINS_BASE(IMAGE_TYPE, ACC_QUAL)                                                                          \
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img##IMAGE_TYPE##_##ACC_QUAL* Image) \
{                                                                                                                                    \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                  \
    return __builtin_IB_get_image_channel_data_type(id) - CLK_SNORM_INT8;                                                            \
}                                                                                                                                    \
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img##IMAGE_TYPE##_##ACC_QUAL* Image)  \
{                                                                                                                                    \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                  \
    return __builtin_IB_get_image_channel_order( id ) - CLK_R;                                                                       \
}

#define DEF_IMAGE_QUERY_BUILTINS(IMAGE_TYPE)  \
DEF_IMAGE_QUERY_BUILTINS_BASE(IMAGE_TYPE, ro) \
DEF_IMAGE_QUERY_BUILTINS_BASE(IMAGE_TYPE, wo) \
DEF_IMAGE_QUERY_BUILTINS_BASE(IMAGE_TYPE, rw)

DEF_IMAGE_QUERY_BUILTINS(2d)
DEF_IMAGE_QUERY_BUILTINS(3d)
DEF_IMAGE_QUERY_BUILTINS(1d)
DEF_IMAGE_QUERY_BUILTINS(1d_buffer)
DEF_IMAGE_QUERY_BUILTINS(1d_array)
DEF_IMAGE_QUERY_BUILTINS(2d_array)
DEF_IMAGE_QUERY_BUILTINS(2d_depth)
DEF_IMAGE_QUERY_BUILTINS(2d_array_depth)

DEF_IMAGE_QUERY_BUILTINS_BASE(2d_msaa,             ro)
DEF_IMAGE_QUERY_BUILTINS_BASE(2d_array_msaa,       ro)
DEF_IMAGE_QUERY_BUILTINS_BASE(2d_msaa_depth,       ro)
DEF_IMAGE_QUERY_BUILTINS_BASE(2d_array_msaa_depth, ro)

// Query image size helper functions
uint __intel_query_image_size_Ruint(long id)
{
    return __builtin_IB_get_image_width(id);
}

uint2 __intel_query_arrayed_image_size_Ruint2(long id)
{
    uint width = __builtin_IB_get_image_width(id);
    uint elements = __builtin_IB_get_image1d_array_size(id);
    return (uint2)(width, elements);
}

uint2 __intel_query_image_size_Ruint2(long id)
{
    uint width = __builtin_IB_get_image_width(id);
    uint height = __builtin_IB_get_image_height(id);
    return (uint2)(width, height);
}

uint3 __intel_query_arrayed_image_size_Ruint3(long id)
{
    uint width = __builtin_IB_get_image_width(id);
    uint height = __builtin_IB_get_image_height(id);
    uint elements = __builtin_IB_get_image2d_array_size(id);
    return (uint3)(width, height, elements);
}

uint3 __intel_query_image_size_Ruint3(long id)
{
    uint width = __builtin_IB_get_image_width(id);
    uint height = __builtin_IB_get_image_height(id);
    uint depth = __builtin_IB_get_image_depth(id);
    return (uint3)(width, height, depth);
}

// ------------------------OpImageQuerySize------------------------
// Query the dimensions of Image, with no level of detail.

// ------------------------OpImageQuerySizeLod------------------------
// Query the dimensions of Image for mipmap level for Level of Detail.

#define DEF_IMAGE_QUERY_SIZE_BASE(IMAGE_TYPE, INTRINSIC, ACC_QUAL, VEC_SIZE, VEC_SIZE_ABBR)                                                                             \
int##VEC_SIZE __attribute__((overloadable)) __spirv_ImageQuerySize_Rint##VEC_SIZE(global Img##IMAGE_TYPE##_##ACC_QUAL* Image)   \
{                                                                                                                                                                       \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                                                     \
    return as_int##VEC_SIZE(INTRINSIC##_Ruint##VEC_SIZE(id));                                                                                                           \
}                                                                                                                                                                       \
long##VEC_SIZE __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong##VEC_SIZE(global Img##IMAGE_TYPE##_##ACC_QUAL* Image) \
{                                                                                                                                                                       \
    uint##VEC_SIZE result = as_uint##VEC_SIZE(__spirv_ImageQuerySize_Rint##VEC_SIZE(Image));               \
    return as_long##VEC_SIZE(__spirv_UConvert_Rulong##VEC_SIZE(result));                                           \
}                                                                                                                                                                       \
int##VEC_SIZE __attribute__((overloadable)) __spirv_ImageQuerySizeLod_Rint##VEC_SIZE(global Img##IMAGE_TYPE##_##ACC_QUAL* Image, int Lod)  \
{                                                                                                                                                                       \
    return __spirv_ImageQuerySize_Rint##VEC_SIZE(Image);                                                   \
}                                                                                                                                                                       \
long##VEC_SIZE __attribute__((overloadable)) __spirv_ImageQuerySizeLod_Rlong##VEC_SIZE(global Img##IMAGE_TYPE##_##ACC_QUAL* Image, int Lod) \
{                                                                                                                                                                       \
    uint##VEC_SIZE result = as_uint##VEC_SIZE(__spirv_ImageQuerySizeLod_Rint##VEC_SIZE(Image, Lod)); \
    return as_long##VEC_SIZE(__spirv_UConvert_Rulong##VEC_SIZE(result));                                           \
}

#define DEF_IMAGE_QUERY_SIZE(IMAGE_TYPE, INTRINSIC, VEC_SIZE, VEC_SIZE_ABBR)  \
DEF_IMAGE_QUERY_SIZE_BASE(IMAGE_TYPE, INTRINSIC, ro, VEC_SIZE, VEC_SIZE_ABBR) \
DEF_IMAGE_QUERY_SIZE_BASE(IMAGE_TYPE, INTRINSIC, wo, VEC_SIZE, VEC_SIZE_ABBR) \
DEF_IMAGE_QUERY_SIZE_BASE(IMAGE_TYPE, INTRINSIC, rw, VEC_SIZE, VEC_SIZE_ABBR)

DEF_IMAGE_QUERY_SIZE(1d,             __intel_query_image_size,          ,   )
DEF_IMAGE_QUERY_SIZE(1d_buffer,      __intel_query_image_size,          ,   )
DEF_IMAGE_QUERY_SIZE(1d_array,       __intel_query_arrayed_image_size, 2, v2)
DEF_IMAGE_QUERY_SIZE(2d,             __intel_query_image_size,         2, v2)
DEF_IMAGE_QUERY_SIZE(2d_depth,       __intel_query_image_size,         2, v2)
DEF_IMAGE_QUERY_SIZE(2d_array,       __intel_query_arrayed_image_size, 3, v3)
DEF_IMAGE_QUERY_SIZE(2d_array_depth, __intel_query_arrayed_image_size, 3, v3)
DEF_IMAGE_QUERY_SIZE(3d,             __intel_query_image_size,         3, v3)

// MSAA image query size
DEF_IMAGE_QUERY_SIZE_BASE(2d_msaa,             __intel_query_image_size,         ro, 2, v2)
DEF_IMAGE_QUERY_SIZE_BASE(2d_msaa_depth,       __intel_query_image_size,         ro, 2, v2)
DEF_IMAGE_QUERY_SIZE_BASE(2d_array_msaa,       __intel_query_arrayed_image_size, ro, 3, v3)
DEF_IMAGE_QUERY_SIZE_BASE(2d_array_msaa_depth, __intel_query_arrayed_image_size, ro, 3, v3)

// ------------------------OpImageQueryLevels------------------------
// Query the number of mipmap levels accessible through Image.

#define DEF_IMAGE_QUERY_LEVELS_BASE(IMAGE_TYPE, ACC_QUAL)                                                                             \
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img##IMAGE_TYPE##_##ACC_QUAL* Image)  \
{                                                                                                                                     \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                                   \
    return __builtin_IB_get_image_num_mip_levels(id);                                                                                 \
}

#define DEF_IMAGE_QUERY_LEVELS(IMAGE_TYPE)   \
DEF_IMAGE_QUERY_LEVELS_BASE(IMAGE_TYPE, ro)  \
DEF_IMAGE_QUERY_LEVELS_BASE(IMAGE_TYPE, wo)  \
DEF_IMAGE_QUERY_LEVELS_BASE(IMAGE_TYPE, rw)

DEF_IMAGE_QUERY_LEVELS(1d)
DEF_IMAGE_QUERY_LEVELS(2d)
DEF_IMAGE_QUERY_LEVELS(3d)
DEF_IMAGE_QUERY_LEVELS(1d_array)
DEF_IMAGE_QUERY_LEVELS(2d_array)
DEF_IMAGE_QUERY_LEVELS(2d_depth)
DEF_IMAGE_QUERY_LEVELS(2d_array_depth)

// ------------------------OpImageQuerySamples------------------------
// Query the number of samples available per texel fetch in a multisample image.

#define DEF_IMAGE_QUERY_SAMPLES(IMAGE_TYPE)                                                                            \
uint __attribute__((overloadable)) __spirv_ImageQuerySamples(global Img##IMAGE_TYPE##_ro* Image)  \
{                                                                                                                      \
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(Image);                                                    \
    return __builtin_IB_get_image_num_samples(id);                                                                     \
}

DEF_IMAGE_QUERY_SAMPLES(2d_msaa)
DEF_IMAGE_QUERY_SAMPLES(2d_array_msaa)
DEF_IMAGE_QUERY_SAMPLES(2d_msaa_depth)
DEF_IMAGE_QUERY_SAMPLES(2d_array_msaa_depth)
