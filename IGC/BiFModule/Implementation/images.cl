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

#include "../Headers/spirv.h"

INLINE Image_t getImage( SampledImage_t SampledImage )
{
    return SampledImage.x;
}


INLINE ImageType_t getType( SampledImage_t SampledImage )
{
    return SampledImage.y;
}


INLINE Sampler_t getSampler( SampledImage_t SampledImage )
{
    return SampledImage.z;
}

OVERLOADABLE INLINE bool isImageDim( ImageType_t ImageType, Dimensionality_t Dim )
{
    return ( ( ImageType >> IMAGETYPE_DIM_SHIFT ) & 0x7 ) == Dim;
}

OVERLOADABLE INLINE bool isImageDim( SampledImage_t SampledImage, Dimensionality_t Dim )
{
    return isImageDim( getType( SampledImage ), Dim );
}

OVERLOADABLE INLINE bool isImageArrayed( ImageType_t ImageType )
{
    return ( ( ImageType >> IMAGETYPE_ARRAYED_SHIFT ) & 0x1 ) == 1;
}

OVERLOADABLE INLINE bool isImageArrayed( SampledImage_t SampledImage )
{
    return isImageArrayed( getType( SampledImage ) );
}

OVERLOADABLE INLINE bool isImageMultisampled( ImageType_t ImageType )
{
    return ( ( ImageType >> IMAGETYPE_MULTISAMPLED_SHIFT ) & 0x1 ) == 1;
}

OVERLOADABLE INLINE bool isImageMultisampled( SampledImage_t SampledImage )
{
    return isImageMultisampled( getType( SampledImage ) );
}

OVERLOADABLE INLINE bool isImageDepth( ImageType_t ImageType )
{
    return ( ( ImageType >> IMAGETYPE_DEPTH_SHIFT ) & 0x1 ) == 1;
}

OVERLOADABLE INLINE bool isImageDepth( SampledImage_t SampledImage )
{
    return isImageDepth( getType( SampledImage ) );
}

// Image Instructions

SampledImage_t __builtin_spirv_OpSampledImage_i64_i64_i64( Image_t Image, ImageType_t ImageType, Sampler_t Sampler )
{
    SampledImage_t sampledImage = { Image, ImageType, Sampler };

    return sampledImage;
}



#define INT_SAMPLE_L( INTRINSIC, TYPE, image_id, sampler_id, coordinate, lod )      \
{                                                                                   \
    TYPE floatCoords = convert_##TYPE( (coordinate) );                              \
    return as_uint4( INTRINSIC( (image_id), (sampler_id), floatCoords, lod ) );     \
}


uint4 __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4i32_i32_f32_i64( SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod, INLINEWA )
{

    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    // "Snap workaround" - path
    size_t sampler_sizet = (size_t)sampler;
    if( (__builtin_IB_get_address_mode( sampler_id ) & 0x07 ) == CLK_ADDRESS_CLAMP_TO_EDGE )
    {
        if( isImageDim( SampledImage, Dim1D ) || isImageDim( SampledImage, DimBuffer ) )
        {
            if( isImageArrayed( SampledImage ) )
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32((float)Coordinate.y), 0.0f, (float)(dt - 1));
                float floatCoords = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(Coordinate.x);
                return as_uint4(__builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, (float2)(floatCoords, layer), Lod));
            }
            else
            {
                INT_SAMPLE_L( __builtin_IB_OCL_1d_sample_l, float, image_id, sampler_id, Coordinate.x, Lod );
            }
        }
        else if( isImageDim( SampledImage, Dim2D ) )
        {
            if( isImageArrayed( SampledImage ) )
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32((float)Coordinate.z), 0.0f, (float)(dt - 1));
                float2 floatCoords = SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i32, _Rfloat2)(Coordinate.xy);
                return as_uint4(__builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, (float4)(floatCoords, layer, 0.0f), Lod));
            }
            else
            {
                INT_SAMPLE_L( __builtin_IB_OCL_2d_sample_l, float2, image_id, sampler_id, Coordinate.xy, Lod );
            }
        }
        else if( isImageDim( SampledImage, Dim3D ) )
        {
            INT_SAMPLE_L( __builtin_IB_OCL_3d_sample_l, float4, image_id, sampler_id, Coordinate, Lod );
        }
    }
    else
    {
        float float_lod = SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)( Lod );

        if( isImageDim( SampledImage, Dim1D ) || isImageDim( SampledImage, DimBuffer ) )
        {
            if( isImageArrayed( SampledImage ) )
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32((float)Coordinate.y), 0.0f, (float)(dt - 1));
                return __builtin_IB_OCL_1darr_ldui( image_id, (int2)(Coordinate.x, (int)layer), float_lod );
            }
            else
            {
                return __builtin_IB_OCL_1d_ldui( image_id, Coordinate.x, float_lod );
            }
        }
        else if( isImageDim( SampledImage, Dim2D ) )
        {
            if( isImageArrayed( SampledImage ) )
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32((float)Coordinate.z), 0.0f, (float)(dt - 1));
                return __builtin_IB_OCL_2darr_ldui( image_id, (int4)(Coordinate.xy, (int)layer, 0), float_lod );
            }
            else
            {
                return __builtin_IB_OCL_2d_ldui( image_id, Coordinate.xy, float_lod );
            }
        }
        else if( isImageDim( SampledImage, Dim3D ) )
        {
            return __builtin_IB_OCL_3d_ldui( image_id, Coordinate.xyzw, float_lod );
        }
    }
}


uint4  __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod, INLINEWA )
{
    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    if( isImageArrayed( SampledImage ) )
    {
        if ( isImageDim( SampledImage, Dim1D ) )
        {
            if (Lod == 0.0f)
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32(Coordinate.y), 0.0f, (float)(dt - 1));
                if ((__builtin_IB_get_address_mode(sampler_id) & 0x07) == CLK_ADDRESS_CLAMP)
                {
                    float tmpCoords = Coordinate.x;
                    if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
                    {
                        float width = (float)(__builtin_IB_get_image_width(image_id));
                        tmpCoords = (float)(width*Coordinate.x);
                    }
                    int2 intCoords = SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f32, _Rint2)((float2)(__builtin_spirv_OpenCL_floor_f32(tmpCoords),layer));
                    return __builtin_IB_OCL_1darr_ldui(image_id, intCoords, 0);
                }
                else
                {
                    if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                    {
                        Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                    }
                    return as_uint4(__builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, Coordinate.xy, 0.0f));
                }
            }
            else
            {
                if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                {
                    Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                }
                return as_uint4(__builtin_IB_OCL_1darr_sample_l(image_id, sampler_id, Coordinate.xy, Lod));
            }
        }
        else
        {
            if (Lod == 0.0f)
            {
                int dt = __builtin_IB_get_image_array_size(image_id);
                float layer = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(__builtin_spirv_OpenCL_rint_f32(Coordinate.z), 0.0f, (float)(dt - 1));
                if ((__builtin_IB_get_address_mode( sampler_id ) & 0x07) == CLK_ADDRESS_CLAMP)
                {
                    float2 tmpCoords = Coordinate.xy;
                    if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
                    {
                        float2 dim = SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i32, _Rfloat2)((uint2)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id)));
                        tmpCoords = Coordinate.xy*dim;
                    }
                    int4 intCoords = SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f32, _Rint4)((float4)(__builtin_spirv_OpenCL_floor_v2f32(tmpCoords), layer, 0.0f));
                    return __builtin_IB_OCL_2darr_ldui(image_id, intCoords, 0);
                }
                else
                {
                    if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                    {
                        Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                        Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
                    }
                    return as_uint4(__builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, Coordinate, 0.0f));
                }
            }
            else
            {
                if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                {
                    Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                    Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
                }
                return as_uint4(__builtin_IB_OCL_2darr_sample_l(image_id, sampler_id, Coordinate, Lod));
            }
        }
    }
    else
    {
        if ( isImageDim( SampledImage, Dim2D ) )
        {
            if (Lod == 0.0f)
            {
                // Even though this is a SPIRV builtin, the runtime still patches OCL C enum values for the addressing mode.
                if ( (__builtin_IB_get_address_mode( sampler_id ) & 0x07) == CLK_ADDRESS_CLAMP)
                {
                    float2 coords = Coordinate.xy;
                    if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
                    {
                        float2 dim = SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i32, _Rfloat2)(
                            (uint2)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id)));
                        coords = coords * dim;
                    }
                    int2 intCoords = SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f32, _Rint2)(__builtin_spirv_OpenCL_floor_v2f32(coords));
                    return __builtin_IB_OCL_2d_ldui(image_id, intCoords, 0);
                }
                else
                {
                    return as_uint4(__builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate.xy, 0.0f));
                }
            }
            else
            {
                if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                {
                    Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                    Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
                }
                return as_uint4(__builtin_IB_OCL_2d_sample_l(image_id, sampler_id, Coordinate.xy, Lod));
            }
        }
        else // Should just be 3D
        {
            if (Lod == 0.0f)
            {
                if ((__builtin_IB_get_address_mode( sampler_id ) & 0x07) == CLK_ADDRESS_CLAMP)
                {
                    if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(sampler_id))
                    {
                        float4 dim = SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i32, _Rfloat4)(
                            (uint4)(__builtin_IB_get_image_width(image_id), __builtin_IB_get_image_height(image_id), __builtin_IB_get_image_depth(image_id), 0));
                        Coordinate = Coordinate*dim;
                    }
                    int4 intCoords = SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f32, _Rint4)(__builtin_spirv_OpenCL_floor_v4f32(Coordinate));
                    return __builtin_IB_OCL_3d_ldui(image_id, intCoords, 0);
                }
                else
                {
                    return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate, 0.0f));
                }
            }
            else
            {
                if ( 0 != __builtin_IB_get_snap_wa_reqd(sampler_id))
                {
                    Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
                    Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
                    Coordinate.z = ( Coordinate.z < 0) ? -1.0f :  Coordinate.z;
                }
                return as_uint4(__builtin_IB_OCL_3d_sample_l(image_id, sampler_id, Coordinate, Lod));
            }
        }
    }
}


float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4i32_i32_f32_i64( SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod, INLINEWA)
{
    float4 floatCoords = convert_float4( Coordinate );
    return __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_i64( SampledImage, floatCoords, ImageOperands, Lod, NULL );
}


float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod, INLINEWA )
{
    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    float4 snappedCoords = Coordinate;

    if( __builtin_IB_get_snap_wa_reqd( sampler_id ) != 0 )
    {
        snappedCoords.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
        snappedCoords.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
        snappedCoords.z = ( Coordinate.z < 0) ? -1.0f :  Coordinate.z;
    }

    if( isImageDim( SampledImage, Dim1D ) || isImageDim( SampledImage, DimBuffer ) )
    {

        if( isImageArrayed( SampledImage ) )
        {
            // Array coordinate is not 'snapped'
            return __builtin_IB_OCL_1darr_sample_l( image_id, sampler_id, (float2)(snappedCoords.x, Coordinate.y), Lod );
        }
        else
        {
            return __builtin_IB_OCL_1d_sample_l( image_id, sampler_id, snappedCoords.x, Lod );
        }
    }
    else if( isImageDim( SampledImage, Dim2D ) )
    {
        if( isImageArrayed( SampledImage ) )
        {
            return __builtin_IB_OCL_2darr_sample_l( image_id, sampler_id, (float4)(snappedCoords.xy, Coordinate.zw), Lod );
        }
        else
        {
            return __builtin_IB_OCL_2d_sample_l( image_id, sampler_id, snappedCoords.xy, Lod );
        }
    }
    else if( isImageDim( SampledImage, Dim3D ) )
    {
        return __builtin_IB_OCL_3d_sample_l( image_id, sampler_id, snappedCoords, Lod );
    }

}

// Gradient overloads
float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float dx, float dy, INLINEWA )
{
    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    if( __builtin_IB_get_snap_wa_reqd( sampler_id ) != 0 )
    {
        Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
    }

    if( isImageDim( SampledImage, Dim1D ) )
    {
        if( isImageArrayed( SampledImage ) )
        {
            return __builtin_IB_OCL_1darr_sample_d(image_id, sampler_id, Coordinate.xy, dx, dy);
        }
        else
        {
            return __builtin_IB_OCL_1d_sample_d(image_id, sampler_id, Coordinate.x, dx, dy);
        }
    }
}

float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v2f32_v2f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float2 dx, float2 dy, INLINEWA )
{
    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    if( __builtin_IB_get_snap_wa_reqd( sampler_id ) != 0 )
    {
        Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
        Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
    }

    if( isImageDim( SampledImage, Dim2D ) )
    {
        if( isImageArrayed( SampledImage ) )
        {
            return __builtin_IB_OCL_2darr_sample_d(image_id, sampler_id, Coordinate, dx, dy);
        }
        else
        {
            return __builtin_IB_OCL_2d_sample_d(image_id, sampler_id, Coordinate.xy, dx, dy);
        }
    }
}

float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v4f32_v4f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float4 dx, float4 dy, INLINEWA )
{
    Image_t image = getImage( SampledImage );
    Sampler_t sampler = getSampler( SampledImage );

    int image_id = (int)__builtin_astype( image, ulong );
    int sampler_id = (int)sampler;

    if( __builtin_IB_get_snap_wa_reqd( sampler_id ) != 0 )
    {
        Coordinate.x = ( Coordinate.x < 0) ? -1.0f :  Coordinate.x;
        Coordinate.y = ( Coordinate.y < 0) ? -1.0f :  Coordinate.y;
        Coordinate.z = ( Coordinate.z < 0) ? -1.0f :  Coordinate.z;
    }

    if( isImageDim( SampledImage, Dim3D ) )
    {
        return __builtin_IB_OCL_3d_sample_d(image_id, sampler_id, Coordinate, dx, dy);
    }
}

float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v3f32_v3f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float3 dx, float3 dy, INLINEWA )
{
    return __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v4f32_v4f32_i64(SampledImage, Coordinate, ImageOperands, (float4)(dx, 0.f), (float4)(dy, 0.f), NULL);
}

uint4 __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_f32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float dx, float dy, INLINEWA )
{
    return as_uint4(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_f32_i64( SampledImage, Coordinate, ImageOperands, dx, dy, NULL ));
}

uint4 __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_v2f32_v2f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float2 dx, float2 dy, INLINEWA )
{
    return as_uint4(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v2f32_v2f32_i64( SampledImage, Coordinate, ImageOperands, dx, dy, NULL ));
}

uint4 __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_v4f32_v4f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float4 dx, float4 dy, INLINEWA )
{
    return as_uint4(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v4f32_v4f32_i64( SampledImage, Coordinate, ImageOperands, dx, dy, NULL ));
}

uint4 __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_v3f32_v3f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float3 dx, float3 dy, INLINEWA )
{
    return as_uint4(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_v3f32_v3f32_i64( SampledImage, Coordinate, ImageOperands, dx, dy, NULL ));
}

#ifdef cl_khr_fp16
half4 __builtin_spirv_OpImageSampleExplicitLod_v4f16_v3i64_v4i32_i32_f32_i64( SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod, INLINEWA )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4i32_i32_f32_i64(SampledImage, Coordinate, ImageOperands, Lod, NULL));
}

half4 __builtin_spirv_OpImageSampleExplicitLod_v4f16_v3i64_v4f32_i32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod, INLINEWA )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)(__builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_i64(SampledImage, Coordinate, ImageOperands, Lod, NULL));
}
#endif //cl_khr_fp16


// Image Read

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

float __builtin_spirv_OpImageRead_f32_i64_i64_v4i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA )
{
    int id = (int)Image;

    if ( isImageArrayed( ImageType ) )
    {
        float4 res = __builtin_IB_OCL_2darr_ld(id, Coordinate, 0);
        return __flush_denormals(res).x;
    }
    else
    {
        float4 res = __builtin_IB_OCL_2d_ld(id, Coordinate.xy, 0);
        return __flush_denormals(res).x;
    }
}

uint4  __builtin_spirv_OpImageRead_v4i32_i64_i64_v4i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA )
{
    int id = (int)Image;
    uint4 result = (uint4)0;

    if( isImageDim( ImageType, Dim1D ) || isImageDim( ImageType, DimBuffer ) )
    {
        if( isImageArrayed( ImageType ) )
        {
            result = __builtin_IB_OCL_1darr_ldui( id, Coordinate.xy, 0 );
        }
        else
        {
            result = __builtin_IB_OCL_1d_ldui( id, Coordinate.x, 0 );
        }
    }
    else if( isImageDim( ImageType, Dim2D ) )
    {
        if( isImageArrayed( ImageType ) )
        {
            result = __builtin_IB_OCL_2darr_ldui( id, Coordinate, 0 );
        }
        else
        {
            result = __builtin_IB_OCL_2d_ldui( id, Coordinate.xy, 0 );
        }
    }
    else if( isImageDim( ImageType, Dim3D ) )
    {
        result = __builtin_IB_OCL_3d_ldui( id, Coordinate, 0 );
    }

    return result;

}


float4 __builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA )
{
    int id = (int)Image;

    if (isImageDim(ImageType, Dim1D) || isImageDim(ImageType, DimBuffer))
    {
        if (isImageArrayed(ImageType))
        {
            float4 res = __builtin_IB_OCL_1darr_ld(id, Coordinate.xy, 0);
            return __flush_denormals(res);
        }
        else
        {
            float4 res = __builtin_IB_OCL_1d_ld(id, Coordinate.x, 0);
            return __flush_denormals(res);
        }
    }
    else if (isImageDim(ImageType, Dim2D))
    {
        if (isImageArrayed(ImageType))
        {
            float4 res = __builtin_IB_OCL_2darr_ld(id, Coordinate, 0);
            return __flush_denormals(res);
        }
        else
        {
            float4 res = __builtin_IB_OCL_2d_ld(id, Coordinate.xy, 0);
            return __flush_denormals(res);
        }
    }
    else if (isImageDim(ImageType, Dim3D))
    {
        float4 res = __builtin_IB_OCL_3d_ld(id, Coordinate, 0);
        return __flush_denormals(res);
    }
}

#ifdef cl_khr_fp16
half4 __builtin_spirv_OpImageRead_v4f16_i64_i64_v4i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)(__builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i64( Image, ImageType, Coordinate, NULL ));
}
#endif // cl_khr_fp16

// Image Read MSAA

float4 __builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA )
{
    int id = (int)Image;
    SampledImage_t SampledImage = { Image, ImageType, 0 };

    if( isImageArrayed( SampledImage ) )
    {
        float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, Coordinate);
        float4 res = __builtin_IB_OCL_2darr_ld2dms(id, Coordinate, Sample, mcs);
        return __flush_denormals(res);
    }
    else
    {
        float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, Coordinate.xy);
        float4 res = __builtin_IB_OCL_2d_ld2dms(id, Coordinate.xy, Sample, mcs);
        return __flush_denormals(res);
    }
}

float __builtin_spirv_OpImageRead_f32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA )
{
    return __builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i32_i32_i64(Image, ImageType, Coordinate, ImageOperands, Sample, NULL).x;
}

uint4 __builtin_spirv_OpImageRead_v4i32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA )
{
    int id = (int)Image;
    SampledImage_t SampledImage = { Image, ImageType, 0 };

    if( isImageArrayed( SampledImage ) )
    {
        float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, Coordinate);
        return __builtin_IB_OCL_2darr_ld2dmsui(id, Coordinate, Sample, mcs);
    }
    else
    {
        float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, Coordinate.xy);
        return __builtin_IB_OCL_2d_ld2dmsui(id, Coordinate.xy, Sample, mcs);
    }
}

// Image Write

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i32_i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, uint4 Texel, uint ImageOperands, uint Lod, INLINEWA )
{
    int id = (int)Image;

    SampledImage_t SampledImage = { Image, ImageType, 0 };

    if( isImageDim( SampledImage, Dim1D ) || isImageDim( SampledImage, DimBuffer ) )
    {

        if( isImageArrayed( SampledImage ) )
        {
            __builtin_IB_write_1darr_ui( id, Coordinate.xy, Texel, Lod );
        }
        else
        {
            __builtin_IB_write_1d_ui( id, Coordinate.x, Texel, Lod );
        }
    }
    else if( isImageDim( SampledImage, Dim2D ) )
    {
        if( isImageArrayed( SampledImage ) )
        {
            __builtin_IB_write_2darr_ui( id, Coordinate, Texel, Lod );
        }
        else
        {
            __builtin_IB_write_2d_ui( id, Coordinate.xy, Texel, Lod );
        }
    }
    else if( isImageDim( SampledImage, Dim3D ) )
    {
        __builtin_IB_write_3d_ui( id, Coordinate, Texel, Lod );
    }


}

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f32_i32_i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, float4 Texel, uint ImageOperands, uint Lod, INLINEWA )
{
    __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i32_i32_i64( Image, ImageType, Coordinate, as_uint4(Texel), ImageOperands, Lod, NULL );
}

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, uint4 Texel, INLINEWA )
{
    __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i32_i32_i64(Image, ImageType, Coordinate, Texel, 0, 0, NULL);
}

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, float4 Texel, INLINEWA )
{
    __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i64( Image, ImageType, Coordinate, as_uint4(Texel), NULL );
}

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_f32_i32_i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, float Texel, uint ImageOperands, uint Lod, INLINEWA )
{
    // scalar float texels currently exist for image2d_depth_t and image2d_array_depth_t types.

    int id = (int)Image;
    SampledImage_t SampledImage = { Image, ImageType, 0 };

    if( isImageArrayed( SampledImage ) )
    {
        __builtin_IB_write_2darr_f(id, Coordinate, Texel, Lod);
    }
    else
    {
        __builtin_IB_write_2d_f(id, Coordinate.xy, Texel, Lod);
    }
}

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_f32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, float Texel, INLINEWA )
{
    __builtin_spirv_OpImageWrite_i64_i64_v4i32_f32_i32_i32_i64(Image, ImageType, Coordinate, Texel, 0, 0, NULL);
}

#ifdef cl_khr_fp16
void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f16_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, half4 Texel, INLINEWA )
{
    __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f32_i64(Image, ImageType, Coordinate, SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(Texel), NULL);
}
#endif // cl_khr_fp16

// Image Query

// From the SPIR spec.  Runtime returns values that match SPIR enum
// values but SPIRV enum values start at 0.

#define CLK_SNORM_INT8 0x10D0

uint  __builtin_spirv_OpImageQueryFormat_i64_i64( Image_t Image, INLINEWA )
{
    int id = (int)Image;
    return __builtin_IB_get_image_channel_data_type( id ) - CLK_SNORM_INT8;
}

#define CLK_R 0x10B0

uint  __builtin_spirv_OpImageQueryOrder_i64_i64( Image_t Image, INLINEWA )
{
    int id = (int)Image;
    return __builtin_IB_get_image_channel_order( id ) - CLK_R;
}


uint  __builtin_spirv_OpImageQuerySizeLod_i32_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    // TODO: Can OpenCL query a specific LOD?
    return __builtin_spirv_OpImageQuerySize_i32_i64_i64_i64( Image, ImageType, NULL );
}

ulong __builtin_spirv_OpImageQuerySizeLod_i64_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _i64_i32, _Rulong)(__builtin_spirv_OpImageQuerySizeLod_i32_i64_i64_i32_i64(Image, ImageType, Lod, NULL));
}

uint2 __builtin_spirv_OpImageQuerySizeLod_v2i32_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    // TODO: Can OpenCL query a specific LOD?
    return __builtin_spirv_OpImageQuerySize_v2i32_i64_i64_i64( Image, ImageType, NULL );
}

ulong2 __builtin_spirv_OpImageQuerySizeLod_v2i64_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v2i64_v2i32, _Rulong2)(__builtin_spirv_OpImageQuerySizeLod_v2i32_i64_i64_i32_i64(Image, ImageType, Lod, NULL));
}


uint3 __builtin_spirv_OpImageQuerySizeLod_v3i32_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    // TODO: Can OpenCL query a specific LOD?
    return __builtin_spirv_OpImageQuerySize_v3i32_i64_i64_i64( Image, ImageType, NULL );
}

ulong3 __builtin_spirv_OpImageQuerySizeLod_v3i64_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v3i64_v3i32, _Rulong3)(__builtin_spirv_OpImageQuerySizeLod_v3i32_i64_i64_i32_i64(Image, ImageType, Lod, NULL));
}

uint4 __builtin_spirv_OpImageQuerySizeLod_v4i32_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    // TODO: Can OpenCL query a specific LOD?
    return __builtin_spirv_OpImageQuerySize_v4i32_i64_i64_i64( Image, ImageType, NULL );
}

ulong4 __builtin_spirv_OpImageQuerySizeLod_v4i64_i64_i64_i32_i64( Image_t Image, ImageType_t ImageType, int Lod, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v4i64_v4i32, _Rulong4)(__builtin_spirv_OpImageQuerySizeLod_v4i32_i64_i64_i32_i64(Image, ImageType, Lod, NULL));
}

uint  __builtin_spirv_OpImageQuerySize_i32_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    int id = (int)Image;
    return __builtin_IB_get_image_width(id);
}

ulong __builtin_spirv_OpImageQuerySize_i64_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _i64_i32, _Rulong)(__builtin_spirv_OpImageQuerySize_i32_i64_i64_i64(Image, ImageType, NULL));
}

uint2 __builtin_spirv_OpImageQuerySize_v2i32_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    int id = (int)Image;
    uint width = __builtin_IB_get_image_width(id);

    if (isImageArrayed(ImageType))
    {
        uint elements = __builtin_IB_get_image_array_size( id );
        return (uint2)( width, elements );
    }
    else
    {
        uint height = __builtin_IB_get_image_height(id);
        return (uint2)( width, height );
    }
}

ulong2 __builtin_spirv_OpImageQuerySize_v2i64_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v2i64_v2i32, _Rulong2)(__builtin_spirv_OpImageQuerySize_v2i32_i64_i64_i64(Image, ImageType, NULL));
}

uint3 __builtin_spirv_OpImageQuerySize_v3i32_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    int id = (int)Image;
    uint width  = __builtin_IB_get_image_width(id);
    uint height = __builtin_IB_get_image_height(id);

    if (isImageArrayed(ImageType))
    {
        uint elements = __builtin_IB_get_image_array_size( id );
        return (uint3)(width, height, elements);
    }
    else
    {
        uint depth = __builtin_IB_get_image_depth(id);
        return (uint3)( width, height, depth );
    }
}

ulong3 __builtin_spirv_OpImageQuerySize_v3i64_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v3i64_v3i32, _Rulong3)(__builtin_spirv_OpImageQuerySize_v3i32_i64_i64_i64(Image, ImageType, NULL));
}

uint4 __builtin_spirv_OpImageQuerySize_v4i32_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    int id = (int)Image;
    uint width  = __builtin_IB_get_image_width(id);
    uint height = __builtin_IB_get_image_height(id);
    uint depth  = __builtin_IB_get_image_depth(id);
    uint elements = __builtin_IB_get_image_array_size( id );

    return (uint4)(width, height, depth, elements);
}

ulong4 __builtin_spirv_OpImageQuerySize_v4i64_i64_i64_i64( Image_t Image, ImageType_t ImageType, INLINEWA )
{
    return SPIRV_BUILTIN(UConvert, _v4i64_v4i32, _Rulong4)(__builtin_spirv_OpImageQuerySize_v4i32_i64_i64_i64(Image, ImageType, NULL));
}

uint __builtin_spirv_OpImageQueryLevels_i64_i64( Image_t Image, INLINEWA )
{
    int id = (int)Image;
    return __builtin_IB_get_image_num_mip_levels( id );
}


uint __builtin_spirv_OpImageQuerySamples_i64_i64( Image_t Image, INLINEWA )
{
    int id = (int)Image;
    return __builtin_IB_get_image_num_samples( id );
}
