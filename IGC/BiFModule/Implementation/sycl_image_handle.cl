/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ---------OpConvertHandleToImage/SampledImage/SamplerINTEL---------
// Convert SYCL bindless image integer handle to spirv image/sampled image
#define DEF_CONVERT_HANDLE_TO_IMAGE(DIM, INT_PARAMS, ACC_QUAL, ARRAY_DEPTH)                                                                           \
global Img##DIM##d##ARRAY_DEPTH##_##ACC_QUAL* OVERLOADABLE __spirv_ConvertHandleToImageINTEL_RPU3AS133__spirv_Image__void_##INT_PARAMS(size_t handle) \
{                                                                                                                                                     \
    return __builtin_astype(handle, global Img##DIM##d##ARRAY_DEPTH##_##ACC_QUAL*);                                                                   \
}

#define DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(DIM, INT_PARAMS, ARRAY_DEPTH) \
DEF_CONVERT_HANDLE_TO_IMAGE(DIM, INT_PARAMS##_0, ro, ARRAY_DEPTH)          \
DEF_CONVERT_HANDLE_TO_IMAGE(DIM, INT_PARAMS##_1, wo, ARRAY_DEPTH)          \
DEF_CONVERT_HANDLE_TO_IMAGE(DIM, INT_PARAMS##_2, rw, ARRAY_DEPTH)          \

DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(1, 0_0_0_0_0_0, )             // 1d
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(1, 0_0_1_0_0_0, _array)       // 1d array
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(2, 1_0_0_0_0_0, )             // 2d
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(2, 1_1_0_0_0_0, _depth)       // 2d depth
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(2, 1_0_1_0_0_0, _array)       // 2d array
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(2, 1_1_1_0_0_0, _array_depth) // 2d depth array
DEF_CONVERT_HANDLE_TO_IMAGE_ACC_QUAL(3, 2_0_0_0_0_0, )             // 3d

#define DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(DIM, INT_PARAMS, ARRAY_DEPTH)                                                                                        \
__spirv_SampledImage_##DIM##D##ARRAY_DEPTH OVERLOADABLE __spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_##INT_PARAMS(size_t handle) \
{                                                                                                                                                                \
    return __builtin_astype(handle, __spirv_SampledImage_##DIM##D##ARRAY_DEPTH);                                                                                 \
}

DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(1, 0_0_0_0_0_0_0, )             // 1d
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(1, 0_0_1_0_0_0_0, _array)       // 1d array
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(2, 1_0_0_0_0_0_0, )             // 2d
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(2, 1_1_0_0_0_0_0, _depth)       // 2d depth
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(2, 1_0_1_0_0_0_0, _array)       // 2d array
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(2, 1_1_1_0_0_0_0, _array_depth) // 2d depth array
DEF_CONVERT_HANDLE_TO_SAMPLED_IMAGE(3, 2_0_0_0_0_0_0, )             // 3d

__spirv_Sampler OVERLOADABLE __spirv_ConvertHandleToSamplerINTEL(size_t handle)
{
    return __builtin_astype(handle, __spirv_Sampler);
}
