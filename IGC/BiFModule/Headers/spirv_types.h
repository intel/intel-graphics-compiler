/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_TYPES_H__
#define __SPIRV_TYPES_H__

typedef global struct __spirv_Pipe__0*      __spirv_Pipe_ro;
typedef global struct __spirv_Pipe__1*      __spirv_Pipe_wo;
typedef private struct __spirv_ReserveId*   __spirv_ReserveId;
typedef private struct __spirv_DeviceEvent* __spirv_DeviceEvent;
typedef private struct __spirv_Event*       __spirv_Event;
typedef private struct __spirv_Queue*       __spirv_Queue;

#define DEF_IMAGE_TYPE(SPIRV_IMAGE_TYPE, SHORT_IMAGE_TYPE)     \
    typedef struct SPIRV_IMAGE_TYPE##_0 SHORT_IMAGE_TYPE##_ro; \
    typedef struct SPIRV_IMAGE_TYPE##_1 SHORT_IMAGE_TYPE##_wo; \
    typedef struct SPIRV_IMAGE_TYPE##_2 SHORT_IMAGE_TYPE##_rw;

DEF_IMAGE_TYPE(__spirv_Image__void_0_0_0_0_0_0, Img1d)
DEF_IMAGE_TYPE(__spirv_Image__void_5_0_0_0_0_0, Img1d_buffer)
DEF_IMAGE_TYPE(__spirv_Image__void_0_0_1_0_0_0, Img1d_array)
DEF_IMAGE_TYPE(__spirv_Image__void_1_0_0_0_0_0, Img2d)
DEF_IMAGE_TYPE(__spirv_Image__void_1_0_1_0_0_0, Img2d_array)
DEF_IMAGE_TYPE(__spirv_Image__void_1_1_0_0_0_0, Img2d_depth)
DEF_IMAGE_TYPE(__spirv_Image__void_1_1_1_0_0_0, Img2d_array_depth)
DEF_IMAGE_TYPE(__spirv_Image__void_1_0_0_1_0_0, Img2d_msaa)
DEF_IMAGE_TYPE(__spirv_Image__void_1_0_1_1_0_0, Img2d_array_msaa)
DEF_IMAGE_TYPE(__spirv_Image__void_1_1_0_1_0_0, Img2d_msaa_depth)
DEF_IMAGE_TYPE(__spirv_Image__void_1_1_1_1_0_0, Img2d_array_msaa_depth)
DEF_IMAGE_TYPE(__spirv_Image__void_2_0_0_0_0_0, Img3d)

typedef global struct __spirv_SampledImage__void_1_0_0_0_0_0_0* __spirv_SampledImage_2D;
typedef global struct __spirv_SampledImage__void_2_0_0_0_0_0_0* __spirv_SampledImage_3D;
typedef global struct __spirv_SampledImage__void_1_0_1_0_0_0_0*
    __spirv_SampledImage_2D_array;
typedef global struct __spirv_SampledImage__void_0_0_0_0_0_0_0* __spirv_SampledImage_1D;
typedef global struct __spirv_SampledImage__void_0_0_1_0_0_0_0*
    __spirv_SampledImage_1D_array;
typedef global struct __spirv_SampledImage__void_1_1_0_0_0_0_0*
    __spirv_SampledImage_2D_depth;
typedef global struct __spirv_SampledImage__void_1_1_1_0_0_0_0*
    __spirv_SampledImage_2D_array_depth;

typedef constant struct __spirv_Sampler*                 __spirv_Sampler;
typedef global struct __spirv_Image__void_1_0_0_0_0_0_0* __spirv_Image;

#endif // __SPIRV_TYPES_H__
