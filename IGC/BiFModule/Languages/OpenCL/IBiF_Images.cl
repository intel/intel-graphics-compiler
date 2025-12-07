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

///////////////////////////////
//
// sRGB write support in SW
// Note : no impact if(BIF_FLAG_CTRL_GET(EnableSWSrgbWrites))
//
///////////////////////////////

#ifdef cl_khr_gl_msaa_sharing
#pragma OPENCL EXTENSION cl_khr_gl_msaa_sharing : enable
#endif //cl_khr_gl_msaa_sharing

#undef DECL_GET_IMAGE_CHANNEL_ORDER_ALL_TYPES
#undef DECL_GET_IMAGE_CHANNEL_ORDER

// Note : this function makes an assumption that the result is being
//        clampled to (0, 1) by the caller
INLINE static float OVERLOADABLE __intel_rgb_to_srgb( float c )
{
    const float threshold       = 0.0031308f;
    const float exponent        = 1.0f / 2.4f;
    const float smallInputCoeff = 12.92f;

    float big     = native_powr( c, exponent );
    big           = fma( 1.055f, big, -0.055f );
    float small   = smallInputCoeff * c;
    return (c < threshold) ? small : big;
}

INLINE static float4 OVERLOADABLE __intel_rgb_to_srgb( float4 c )
{
    float4 ret;
    ret.x = __intel_rgb_to_srgb(c.x);
    ret.y = __intel_rgb_to_srgb(c.y);
    ret.z = __intel_rgb_to_srgb(c.z);
    ret.w = c.w;
    return ret;
}

INLINE static int OVERLOADABLE __intel_image_is_sRGB(long image_id)
{
    return (__builtin_IB_get_image_srgb_channel_order( image_id ) != 0);
}

INLINE static float4 OVERLOADABLE __intel_image_convert_RGB_to_output_format(long image_id, float4 color)
{
    if(BIF_FLAG_CTRL_GET(EnableSWSrgbWrites) && __intel_image_is_sRGB(image_id))
    {
        return __intel_rgb_to_srgb(color);
    }
    else
    {
        return color;
    }
}

///////////////////////////////
//
// read_image*()
//
///////////////////////////////
// 2D reads
INLINE float4 OVERLOADABLE read_imagef(read_only image2d_t image, sampler_t sampler, int2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float2 floatCoords = convert_float2(coords);
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f);
}

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_t image, sampler_t sampler, float2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_t image, sampler_t sampler, int2 coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_t image, sampler_t sampler, float2 coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_t image, sampler_t sampler, int2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float2 floatCoords = convert_float2(coords);
        return as_uint4(__builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f));
    }
    else
    {
        return __builtin_IB_OCL_2d_ldui(id, coords, 0);
    }
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_t image, sampler_t sampler, float2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP)
    {
        if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(__builtin_IB_convert_sampler_to_int(sampler)))
        {
          float2 dim = convert_float2(get_image_dim(image));
          coords = coords * dim;
        }
        int2 intCoords;
        intCoords = convert_int2(floor(coords));
        return __builtin_IB_OCL_2d_ldui(id, intCoords, 0);
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f));
    }
}

// 3D reads
INLINE float4 OVERLOADABLE read_imagef(read_only image3d_t image, sampler_t sampler, int4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float3 floatCoords = convert_float3(coords.xyz);
    return __builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f);
}

INLINE float4 OVERLOADABLE read_imagef(read_only image3d_t image, sampler_t sampler, float4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);

    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        coords.z = ( coords.z < 0) ? -1.0f :  coords.z;
    }
    return __builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, 0.0f);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image3d_t image, sampler_t sampler, int4 coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE int4 OVERLOADABLE read_imagei(read_only image3d_t image, sampler_t sampler, float4 coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image3d_t image, sampler_t sampler, int4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float3 floatCoords = convert_float3(coords.xyz);
        return as_uint4(__builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f));
    }
    else
    {
        return __builtin_IB_OCL_3d_ldui(id, coords.xyz, 0);
    }

}

INLINE uint4 OVERLOADABLE read_imageui(read_only image3d_t image, sampler_t sampler, float4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP)
    {
        if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(__builtin_IB_convert_sampler_to_int(sampler)))
        {
          float4 dim = convert_float4(get_image_dim(image));
          coords = coords*dim;
        }
        int4 intCoords = convert_int4(floor(coords));
        return __builtin_IB_OCL_3d_ldui(id, intCoords.xyz, 0);
    }
    else
    {
        return as_uint4(__builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, 0.0f));
    }
}

// 1D reads
INLINE float4 OVERLOADABLE read_imagef(read_only image1d_t image, sampler_t sampler, int coords) {

    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float tmpCoords = (float)coords;
    return __builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), tmpCoords, 0.0f);
}

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_t image, sampler_t sampler, float coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords = ( coords < 0) ? -1.0f :  coords;
    }
    return __builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_t image, sampler_t sampler, int coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_t image, sampler_t sampler, float coords) {
    return as_int4(read_imageui(image, sampler, coords));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_t image, sampler_t sampler, int coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float floatCoords = (float)coords;
        return as_uint4(__builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f));
    }
    else
    {
        return __builtin_IB_OCL_1d_ldui(id, coords, 0);
    }
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_t image, sampler_t sampler, float coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP)
    {
        if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(__builtin_IB_convert_sampler_to_int(sampler)))
        {
          float width = (float)get_image_width(image);
          coords = coords*width;
        }
        int intCoords = (int)floor(coords);
        return __builtin_IB_OCL_1d_ldui(id, intCoords, 0);
    }
    else
    {
        if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
        {
            coords = ( coords < 0) ? -1.0f :  coords;
        }
        return as_uint4(__builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f));
    }
}

// 1D reads with mipmap support

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_t image, sampler_t sampler, float coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords = ( coords < 0) ? -1.0f :  coords;
    }
    return __builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image1d_t image, sampler_t sampler, float coords, float lod) {
    return as_int4(read_imageui(image, sampler, coords, lod));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_t image, sampler_t sampler, float coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords = ( coords < 0) ? -1.0f :  coords;
    }
    return as_uint4(__builtin_IB_OCL_1d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod));
}

// 1D reads with mipmap support using gradients for LOD computation

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_t image, sampler_t sampler, float coords, float gradientX, float gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords = ( coords < 0) ? -1.0f :  coords;
    }
    return __builtin_IB_OCL_1d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image1d_t image, sampler_t sampler, float coords, float gradientX, float gradientY) {
    return as_int4(read_imageui(image, sampler, coords, gradientX, gradientY));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_t image, sampler_t sampler, float coords, float gradientX, float gradientY) {
    return as_uint4(read_imagef(image, sampler, coords, gradientX, gradientY));
}

// 1D Array Reads

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_array_t image_array, sampler_t sampler, int2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    float2 tmpCoords = convert_float2(coords);
    return __builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), tmpCoords, 0.0f);
}

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
    }
    return __builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_array_t image_array, sampler_t sampler, int2 coords) {
    return as_int4(read_imageui(image_array, sampler, coords));
}

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coords) {
    return as_int4(read_imageui(image_array, sampler, coords));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_array_t image_array, sampler_t sampler, int2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    long IDimage_array = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    int dt = __builtin_IB_get_image1d_array_size(IDimage_array);
    float layer = clamp(rint((float)coords.y), 0.0f, (float)(--dt));
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float floatCoords = convert_float(coords.x);
        return as_uint4(__builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), (float2)(floatCoords, layer), 0.0f));
    }
    else
    {
        return __builtin_IB_OCL_2d_ldui(id, (int2)(coords.x, (int)layer), 0);
    }
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    long IDimage_array = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    int dt = __builtin_IB_get_image1d_array_size(IDimage_array);
    float layer = clamp(rint(coords.y), 0.0f, (float)(--dt));
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP)
    {
        float tmpCoords = coords.x;
        if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(__builtin_IB_convert_sampler_to_int(sampler)))
        {
          float width = (float)(get_image_width(image_array));
          tmpCoords = (float)(width*coords.x);
        }
        int2 intCoords = convert_int2((float2)(floor(tmpCoords),layer));
        return __builtin_IB_OCL_1darr_ldui(id, intCoords, 0);
    }
    else
    {
        if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
        {
            coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        }
        return as_uint4(__builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f));
    }
}

// 1D Array Reads with mipmap support

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
    }
    return __builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float lod) {
    return as_int4(read_imageui(image_array, sampler, coords, lod));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
    }
    return as_uint4(__builtin_IB_OCL_1darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod));
}

// 1D Array Reads with mipmap support using gradients for LOD computation

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float gradientX, float gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
    }
    return __builtin_IB_OCL_1darr_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float gradientX, float gradientY) {
    return as_int4(read_imageui(image_array, sampler, coords, gradientX, gradientY));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_array_t image_array, sampler_t sampler, float2 coords, float gradientX, float gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
    }
    return as_uint4(__builtin_IB_OCL_1darr_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY));
}

// 2D reads with mipmap support

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_t image, sampler_t sampler, float2 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image2d_t image, sampler_t sampler, float2 coords, float lod) {
    return as_int4(read_imageui(image, sampler, coords, lod));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_t image, sampler_t sampler, float2 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return as_uint4(__builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod));
}

// 2D reads with mipmap support using gradients for LOD computation

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_t image, sampler_t sampler, float2 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image2d_t image, sampler_t sampler, float2 coords, float2 gradientX, float2 gradientY) {
    return as_int4(read_imageui(image, sampler, coords, gradientX, gradientY));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_t image, sampler_t sampler, float2 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return as_uint4(__builtin_IB_OCL_2d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY));
}

// 2D depth reads with mipmap support using gradients for LOD computation

INLINE float OVERLOADABLE read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords, gradientX, gradientY).x;
}

// 2D depth reads with mipmap support

INLINE float OVERLOADABLE read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, lod).x;
}

// 2D Array Reads

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_t image_array, sampler_t sampler, int4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    float3 tmpCoords = convert_float3(coords.xyz);
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), tmpCoords, 0.0f);

}

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);

    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, 0.0f);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_t image_array, sampler_t sampler, int4 coords) {
    return as_int4(read_imageui(image_array, sampler, coords));
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coords) {
    return as_int4(read_imageui(image_array, sampler, coords));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_t image_array, sampler_t sampler, int4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    long IDimage_array = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    int dt = __builtin_IB_get_image2d_array_size(IDimage_array);
    float layer = clamp(rint((float)coords.z), 0.0f, (float)(--dt));
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP_TO_EDGE)
    {
        float2 floatCoords = convert_float2(coords.xy);
        return as_uint4(__builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), (float3)(floatCoords, layer), 0.0f));
    }
    else
    {
        return __builtin_IB_OCL_2darr_ldui(id, (int3)(coords.x,coords.y,(int)layer), 0);
    }
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    long IDimage_array = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    int dt = __builtin_IB_get_image2d_array_size(IDimage_array);
    float layer = clamp(rint(coords.z), 0.0f, (float)(--dt));
    if ((__builtin_IB_get_address_mode(__builtin_IB_convert_sampler_to_int(sampler)) & 0x07) == CLK_ADDRESS_CLAMP)
    {
        float2 tmpCoords = coords.xy;
        if( CLK_NORMALIZED_COORDS_TRUE == __builtin_IB_is_normalized_coords(__builtin_IB_convert_sampler_to_int(sampler)))
        {
            float2 dim = convert_float2(get_image_dim(image_array));
            tmpCoords = coords.xy*dim;
        }
        int3 intCoords = convert_int3((float3)(floor(tmpCoords), layer));
        return __builtin_IB_OCL_2darr_ldui(id, intCoords, 0);
    }
    else
    {
        if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
        {
            coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
            coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        }
        return as_uint4(__builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, 0.0f));
    }
}

// 2D Array Reads with mipmap support

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, lod);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float lod) {
    return as_int4(read_imageui(image_array, sampler, coords, lod));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return as_uint4(__builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, lod));
}

// 2D Array Reads with mipmap support using gradients for LOD computation

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, gradientX, gradientY);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float2 gradientX, float2 gradientY) {
    return as_int4(read_imageui(image_array, sampler, coords, gradientX, gradientY));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_t image_array, sampler_t sampler, float4 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return as_uint4(__builtin_IB_OCL_2darr_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, gradientX, gradientY));
}

// 2D Depth Array Reads with mipmap support

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, lod).x;
}

// 2D Depth Array Reads with mipmap support using gradients for LOD computation

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coords, float2 gradientX, float2 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, gradientX, gradientY).x;
}

// 3D reads with mipmap support

INLINE float4 OVERLOADABLE read_imagef(read_only image3d_t image, sampler_t sampler, float4 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        coords.z = ( coords.z < 0) ? -1.0f :  coords.z;
    }
    return __builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, lod);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image3d_t image, sampler_t sampler, float4 coords, float lod) {
    return as_int4(read_imageui(image, sampler, coords, lod));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image3d_t image, sampler_t sampler, float4 coords, float lod) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        coords.z = ( coords.z < 0) ? -1.0f :  coords.z;
    }
    return as_uint4(__builtin_IB_OCL_3d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, lod));
}

// 3D reads with mipmap support using gradients for LOD computation

INLINE float4 OVERLOADABLE read_imagef(read_only image3d_t image, sampler_t sampler, float4 coords, float4 gradientX, float4 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        coords.z = ( coords.z < 0) ? -1.0f :  coords.z;
    }
    return __builtin_IB_OCL_3d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, gradientX.xyz, gradientY.xyz);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE int4 OVERLOADABLE read_imagei(read_only image3d_t image, sampler_t sampler, float4 coords, float4 gradientX, float4 gradientY) {
    return as_int4(read_imageui(image, sampler, coords, gradientX, gradientY));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE uint4 OVERLOADABLE read_imageui(read_only image3d_t image, sampler_t sampler, float4 coords, float4 gradientX, float4 gradientY) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
        coords.z = ( coords.z < 0) ? -1.0f :  coords.z;
    }
    return as_uint4(__builtin_IB_OCL_3d_sample_d(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, gradientX.xyz, gradientY.xyz));
}

// Sampler less image access

#ifdef __IGC_BUILD__
float4 __flush_denormals(float4 in);
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1d_ld(id, coord, 0);
    return __flush_denormals(res);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image1d_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1d_ld(id, coord, 0);
    return __flush_denormals(res);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_t image, int coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image1d_t image, int coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1d_ldui(id, coord, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image1d_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1d_ldui(id, coord, 0);
}
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_buffer_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1d_ld(id, coord, 0);
    return __flush_denormals(res);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image1d_buffer_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1d_ld(id, coord, 0);
    return __flush_denormals(res);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_buffer_t image, int coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image1d_buffer_t image, int coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_buffer_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1d_ldui(id, coord, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image1d_buffer_t image, int coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1d_ldui(id, coord, 0);
}
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image1d_array_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1darr_ld(id, coord, 0);
    return __flush_denormals(res);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image1d_array_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_1darr_ld(id, coord, 0);
    return __flush_denormals(res);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image1d_array_t image, int2 coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image1d_array_t image, int2 coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image1d_array_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1darr_ldui(id, coord, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image1d_array_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_1darr_ldui(id, coord, 0);
}
#endif

#ifdef __IGC_BUILD__
INLINE float4 OVERLOADABLE static __read_imagef_2d(long id, int2 coord) {
    float4 res = __builtin_IB_OCL_2d_ld(id, coord, 0);
    return __flush_denormals(res);
}
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_2d(id, coord);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image2d_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_2d(id, coord);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_t image, int2 coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image2d_t image, int2 coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_2d_ldui(id, coord, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image2d_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_2d_ldui(id, coord, 0);
}
#endif

#ifdef __IGC_BUILD__
INLINE float4 OVERLOADABLE static __read_imagef_3d(long id, int4 coord) {
    float4 res = __builtin_IB_OCL_3d_ld(id, coord.xyz, 0);
    return __flush_denormals(res);
}
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_2darr_ld(id, coord.xyz, 0);
    return __flush_denormals(res);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image2d_array_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_2darr_ld(id, coord.xyz, 0);
    return __flush_denormals(res);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_t image, int4 coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image2d_array_t image, int4 coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_2darr_ldui(id, coord.xyz, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image2d_array_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_2darr_ldui(id, coord.xyz, 0);
}
#endif

INLINE float4 OVERLOADABLE read_imagef(read_only image3d_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_3d(id, coord);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float4 OVERLOADABLE read_imagef(read_write image3d_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_3d(id, coord);
}
#endif

INLINE int4 OVERLOADABLE read_imagei(read_only image3d_t image, int4 coord) {
    return as_int4(read_imageui(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int4 OVERLOADABLE read_imagei(read_write image3d_t image, int4 coord) {
    return as_int4(read_imageui(image, coord));
}
#endif

INLINE uint4 OVERLOADABLE read_imageui(read_only image3d_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_3d_ldui(id, coord.xyz, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE uint4 OVERLOADABLE read_imageui(read_write image3d_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_OCL_3d_ldui(id, coord.xyz, 0);
}
#endif

// MSAA Reads

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_msaa_t image, int2 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, coords);
    float4 res = __builtin_IB_OCL_2d_ld2dms(id, coords, sample, mcs);
    return __flush_denormals(res);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_msaa_t image, int2 coords, int sample) {
    return as_int4(read_imageui(image, coords, sample));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_msaa_t image, int2 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, coords);
    return __builtin_IB_OCL_2d_ld2dmsui(id, coords, sample, mcs);
}

INLINE float OVERLOADABLE read_imagef(read_only image2d_msaa_depth_t image, int2 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2d_ldmcs(id, coords);
    float4 res = __builtin_IB_OCL_2d_ld2dms(id, coords, sample, mcs);
    return __flush_denormals(res).x;
}

INLINE float4 OVERLOADABLE read_imagef(read_only image2d_array_msaa_t image, int4 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, coords);
    float4 res = __builtin_IB_OCL_2darr_ld2dms(id, coords, sample, mcs);
    return __flush_denormals(res);
}

INLINE int4 OVERLOADABLE read_imagei(read_only image2d_array_msaa_t image, int4 coords, int sample) {
    return as_int4(read_imageui(image, coords, sample));
}

INLINE uint4 OVERLOADABLE read_imageui(read_only image2d_array_msaa_t image, int4 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, coords);
    return __builtin_IB_OCL_2darr_ld2dmsui(id, coords, sample, mcs);
}

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_msaa_depth_t image, int4 coords, int sample) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 mcs = __builtin_IB_OCL_2darr_ldmcs(id, coords);
    float4 res = __builtin_IB_OCL_2darr_ld2dms(id, coords, sample, mcs);
    return __flush_denormals(res).x;
}

// Depth Reads
INLINE float OVERLOADABLE read_imagef(read_only image2d_depth_t image, sampler_t sampler, float2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords, 0.0f).x;
}

INLINE float OVERLOADABLE read_imagef(read_only image2d_depth_t image, sampler_t sampler, int2 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float2 floatCoords = convert_float2(coords);
    return __builtin_IB_OCL_2d_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), floatCoords, 0.0f).x;
}

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, float4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);

    if ( 0 != __builtin_IB_get_snap_wa_reqd(__builtin_IB_convert_sampler_to_int(sampler)))
    {
        coords.x = ( coords.x < 0) ? -1.0f :  coords.x;
        coords.y = ( coords.y < 0) ? -1.0f :  coords.y;
    }
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), coords.xyz, 0.0f).x;
}

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_depth_t image, sampler_t sampler, int4 coords) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float3 tmpCoords = convert_float3(coords.xyz);
    return __builtin_IB_OCL_2darr_sample_l(id, __builtin_IB_convert_sampler_to_int(sampler), tmpCoords, 0.0f).x;
}

// Sampler less Depth reads

INLINE float OVERLOADABLE read_imagef(read_only image2d_depth_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_2d(id, coord).x;
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float OVERLOADABLE read_imagef(read_write image2d_depth_t image, int2 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __read_imagef_2d(id, coord).x;
}
#endif

INLINE float OVERLOADABLE read_imagef(read_only image2d_array_depth_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_2darr_ld(id, coord.xyz, 0);
    return __flush_denormals(res).x;
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE float OVERLOADABLE read_imagef(read_write image2d_array_depth_t image, int4 coord) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    float4 res = __builtin_IB_OCL_2darr_ld(id, coord.xyz, 0);
    return __flush_denormals(res).x;
}
#endif

/// Half read image functions
INLINE half4 OVERLOADABLE read_imageh(read_only image1d_t image, sampler_t sampler, int coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image1d_t image, sampler_t sampler, float coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image1d_array_t image_array, sampler_t sampler, int2 coords) {
    return convert_half4(read_imagef(image_array, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image1d_array_t image_array, sampler_t sampler, float2 coords) {
    return convert_half4(read_imagef(image_array, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_t image, sampler_t sampler, int2 coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_t image, sampler_t sampler, float2 coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image3d_t image, sampler_t sampler, int4 coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image3d_t image, sampler_t sampler, float4 coords) {
    return convert_half4(read_imagef(image, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_array_t image_array, sampler_t sampler, int4 coords) {
    return convert_half4(read_imagef(image_array, sampler, coords));
}

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_array_t image_array, sampler_t sampler, float4 coords) {
    return convert_half4(read_imagef(image_array, sampler, coords));
}

// samplerless reads (half)
INLINE half4 OVERLOADABLE read_imageh(read_only image1d_t image, int coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image1d_t image, int coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_t image, int2 coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image2d_t image, int2 coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

INLINE half4 OVERLOADABLE read_imageh(read_only image3d_t image, int4 coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image3d_t image, int4 coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

INLINE half4 OVERLOADABLE read_imageh(read_only image1d_array_t image, int2 coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image1d_array_t image, int2 coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

INLINE half4 OVERLOADABLE read_imageh(read_only image2d_array_t image, int4 coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image2d_array_t image, int4 coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

INLINE half4 OVERLOADABLE read_imageh(read_only image1d_buffer_t image, int coord) {
    return convert_half4(read_imagef(image, coord));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE half4 OVERLOADABLE read_imageh(read_write image1d_buffer_t image, int coord) {
    return convert_half4(read_imagef(image, coord));
}
#endif

///////////////////////////////
//
// write_image*()
//
///////////////////////////////
// 2D writes
INLINE void OVERLOADABLE write_imagef(write_only image2d_t image, int2 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image2d_t image, int2 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image2d_t image, int2 coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image2d_t image, int2 coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image2d_t image, int2 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_u4i(id, coords, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image2d_t image, int2 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_u4i(id, coords, color, 0);
}
#endif

// 3D writes
INLINE void OVERLOADABLE write_imagef(write_only image3d_t image, int4 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image3d_t image, int4 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image3d_t image, int4 coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image3d_t image, int4 coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image3d_t image, int4 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_3d_u4i(id, coords.xyz, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image3d_t image, int4 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_3d_u4i(id, coords.xyz, color, 0);
}
#endif

// 1D writes
INLINE void OVERLOADABLE write_imagef(write_only image1d_t image, int coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image1d_t image, int coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image1d_t image, int coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image1d_t image, int coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image1d_t image, int coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_1d_u4i(id, coords, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image1d_t image, int coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_1d_u4i(id, coords, color, 0);
}
#endif

// 1D writes with mipmap support
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE write_imagef(write_only image1d_t image, int coords, int lod, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, lod, as_uint4(color));
}

INLINE void OVERLOADABLE write_imagei(write_only image1d_t image, int coords, int lod, int4 color) {
    write_imageui(image, coords, lod, as_uint4(color));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE void OVERLOADABLE write_imageui(write_only image1d_t image, int coords, int lod, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_1d_u4i(id, coords, color, lod);
}

// 1D buffer writes
INLINE void OVERLOADABLE write_imagef(write_only image1d_buffer_t image, int coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image1d_buffer_t image, int coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image1d_buffer_t image, int coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image1d_buffer_t image, int coords, int4 color) {
    write_imageui(image, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image1d_buffer_t image, int coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_1d_u4i(id, coords, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image1d_buffer_t image, int coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_1d_u4i(id, coords, color, 0);
}
#endif

// 1D Array writes
INLINE void OVERLOADABLE write_imagef(write_only image1d_array_t image_array, int2 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image1d_array_t image_array, int2 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image1d_array_t image_array, int2 coords, int4 color) {
    write_imageui(image_array, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image1d_array_t image_array, int2 coords, int4 color) {
    write_imageui(image_array, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image1d_array_t image_array, int2 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_1darr_u4i(id, coords, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image1d_array_t image_array, int2 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_1darr_u4i(id, coords, color, 0);
}
#endif

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// 1D Array writes with mipmap support
INLINE void OVERLOADABLE write_imagef(write_only image1d_array_t image_array, int2 coords, int lod, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, lod, as_uint4(color));
}

INLINE void OVERLOADABLE write_imagei(write_only image1d_array_t image_array, int2 coords, int lod, int4 color) {
    write_imageui(image_array, coords, lod, as_uint4(color));
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE void OVERLOADABLE write_imageui(write_only image1d_array_t image_array, int2 coords, int lod, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_1darr_u4i(id, coords, color, lod);
}

// 2D Array writes
INLINE void OVERLOADABLE write_imagef(write_only image2d_array_t image_array, int4 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image2d_array_t image_array, int4 coords, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imagei(write_only image2d_array_t image_array, int4 coords, int4 color) {
    write_imageui(image_array, coords, as_uint4(color));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagei(read_write image2d_array_t image_array, int4 coords, int4 color) {
    write_imageui(image_array, coords, as_uint4(color));
}
#endif

INLINE void OVERLOADABLE write_imageui(write_only image2d_array_t image_array, int4 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_2darr_u4i(id, coords.xyz, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageui(read_write image2d_array_t image_array, int4 coords, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_2darr_u4i(id, coords.xyz, color, 0);
}
#endif

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// 2D writes with mipmap support
INLINE void OVERLOADABLE write_imagef(write_only image2d_t image, int2 coords, int lod, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, lod, as_uint4(color));
}

INLINE void OVERLOADABLE write_imagei(write_only image2d_t image, int2 coords, int lod, int4 color) {
    write_imageui(image, coords, lod, as_uint4(color));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE void OVERLOADABLE write_imageui(write_only image2d_t image, int2 coords, int lod, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_u4i(id, coords, color, lod);
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// 2D Array writes with mipmap support
INLINE void OVERLOADABLE write_imagef(write_only image2d_array_t image_array, int4 coords, int lod, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image_array, coords, lod, as_uint4(color));
}

INLINE void OVERLOADABLE write_imagei(write_only image2d_array_t image_array, int4 coords, int lod, int4 color) {
    write_imageui(image_array, coords, lod, as_uint4(color));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE void OVERLOADABLE write_imageui(write_only image2d_array_t image_array, int4 coords, int lod, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    __builtin_IB_write_2darr_u4i(id, coords.xyz, color, lod);
}

#if(__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
// 3D writes with mipmap support
INLINE void OVERLOADABLE write_imagef(write_only image3d_t image, int4 coords, int lod, float4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    color = __intel_image_convert_RGB_to_output_format(id, color);
    write_imageui(image, coords, lod, as_uint4(color));
}

INLINE void OVERLOADABLE write_imagei(write_only image3d_t image, int4 coords, int lod, int4 color) {
    write_imageui(image, coords, lod, as_uint4(color));
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

INLINE void OVERLOADABLE write_imageui(write_only image3d_t image, int4 coords, int lod, uint4 color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_3d_u4i(id, coords.xyz, color, lod);
}

// Depth Writes

INLINE void OVERLOADABLE write_imagef(write_only image2d_depth_t image, int2 coord, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_f(id, coord, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image2d_depth_t image, int2 coord, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_f(id, coord, color, 0);
}
#endif

INLINE void OVERLOADABLE write_imagef(write_only image2d_array_depth_t image, int4 coord, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2darr_f(id, coord, color, 0);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imagef(read_write image2d_array_depth_t image, int4 coord, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2darr_f(id, coord, color, 0);
}
#endif

// Depth Writes with mipmap support

INLINE void OVERLOADABLE write_imagef(write_only image2d_depth_t image, int2 coord, int lod, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2d_f(id, coord, color, lod);
}

INLINE void OVERLOADABLE write_imagef(write_only image2d_array_depth_t image, int4 coord, int lod, float color) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    __builtin_IB_write_2darr_f(id, coord, color, lod);
}

/// Half write image functions
INLINE void OVERLOADABLE write_imageh(write_only image1d_t image, int coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image1d_t image, int coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}
#endif

INLINE void OVERLOADABLE write_imageh(write_only image2d_t image, int2 coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image2d_t image, int2 coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}
#endif

INLINE void OVERLOADABLE write_imageh(write_only image3d_t image, int4 coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image3d_t image, int4 coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}
#endif

INLINE void OVERLOADABLE write_imageh(write_only image1d_array_t image_array, int2 coords, half4 colorH) {
    write_imagef(image_array, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image1d_array_t image_array, int2 coords, half4 colorH) {
    write_imagef(image_array, coords, convert_float4(colorH));
}
#endif

INLINE void OVERLOADABLE write_imageh(write_only image2d_array_t image_array, int4 coords, half4 colorH) {
    write_imagef(image_array, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image2d_array_t image_array, int4 coords, half4 colorH) {
    write_imagef(image_array, coords, convert_float4(colorH));
}
#endif

INLINE void OVERLOADABLE write_imageh(write_only image1d_buffer_t image, int coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE void OVERLOADABLE write_imageh(read_write image1d_buffer_t image, int coords, half4 colorH) {
    write_imagef(image, coords, convert_float4(colorH));
}
#endif

///////////////////////////////
//
// get_image_width()
//
///////////////////////////////
// Other image functions
INLINE int OVERLOADABLE get_image_width(read_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_only image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_width(write_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(write_only image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_width(read_write image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}

INLINE int OVERLOADABLE get_image_width(read_write image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_width(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

///////////////////////////////
//
// get_image_height()
//
///////////////////////////////

INLINE int OVERLOADABLE get_image_height(read_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_height(write_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(write_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_height(read_write image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_height(id);
}

INLINE int OVERLOADABLE get_image_height(read_write image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_height(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

///////////////////////////////
//
// get_image_depth()
//
///////////////////////////////

INLINE int OVERLOADABLE get_image_depth(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_depth(id);
}

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_depth(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_depth(id);
}

INLINE int OVERLOADABLE get_image_depth(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_depth(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

//////////////////////////////////
//
// get_image_channel_data_type()
//
//////////////////////////////////

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(write_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_data_type(id);
}

INLINE int OVERLOADABLE get_image_channel_data_type(read_write image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_data_type(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

//////////////////////////////////
//
// get_image_channel_order()
//
//////////////////////////////////

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(write_only image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image1d_buffer_t image_buffer) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_buffer);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image1d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_array_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_array_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_array_msaa_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_channel_order(id);
}

INLINE int OVERLOADABLE get_image_channel_order(read_write image2d_array_msaa_depth_t image_array) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image_array);
    return __builtin_IB_get_image_channel_order(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

//////////////////////////////////
//
// get_image_dim()
//
//////////////////////////////////

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int4 OVERLOADABLE get_image_dim(read_only image3d_t image) {
    return (int4)(get_image_width(image), get_image_height(image), get_image_depth(image), 0);
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_msaa_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_array_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_array_msaa_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_array_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_msaa_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_only image2d_array_msaa_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int4 OVERLOADABLE get_image_dim(write_only image3d_t image) {
    return (int4)(get_image_width(image), get_image_height(image), get_image_depth(image), 0);
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_msaa_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_array_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_array_msaa_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_array_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_msaa_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(write_only image2d_array_msaa_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int4 OVERLOADABLE get_image_dim(read_write image3d_t image) {
    return (int4)(get_image_width(image), get_image_height(image), get_image_depth(image), 0);
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_msaa_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_array_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_array_msaa_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_array_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_msaa_depth_t image) {
    return (int2)(get_image_width(image), get_image_height(image));
}

INLINE int2 OVERLOADABLE get_image_dim(read_write image2d_array_msaa_depth_t image_array) {
    return (int2)(get_image_width(image_array), get_image_height(image_array));
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

//////////////////////////////////
//
// get_image_num_mip_levels()
//
//////////////////////////////////

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image1d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image2d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_only image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image1d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image2d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(write_only image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image1d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image2d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image3d_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image1d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image2d_array_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image2d_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}

INLINE int OVERLOADABLE get_image_num_mip_levels(read_write image2d_array_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_mip_levels(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

//////////////////////////////////
//
// get_image_num_samples()
//
//////////////////////////////////
// MSAA Num Samples

INLINE int OVERLOADABLE get_image_num_samples(read_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_only image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_only image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_num_samples(write_only image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(write_only image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(write_only image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}
INLINE int OVERLOADABLE get_image_num_samples(write_only image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD

/////////////////////////////////////////////////////////////////////////

#if SUPPORT_ACCESS_QUAL_OVERLOAD
INLINE int OVERLOADABLE get_image_num_samples(read_write image2d_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_write image2d_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_write image2d_array_msaa_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}

INLINE int OVERLOADABLE get_image_num_samples(read_write image2d_array_msaa_depth_t image) {
    long id = (long)__builtin_IB_cast_object_to_generic_ptr(image);
    return __builtin_IB_get_image_num_samples(id);
}
#endif // SUPPORT_ACCESS_QUAL_OVERLOAD


