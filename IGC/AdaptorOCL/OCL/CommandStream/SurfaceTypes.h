/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace iOpenCL
{

/*****************************************************************************\
ENUM: RESOURCE_ALLOCATION_TYPE
\*****************************************************************************/
enum RESOURCE_ALLOCATION_TYPE
{
    RESOURCE_ALLOCATION_LINEAR,
    RESOURCE_ALLOCATION_TILED_X,
    RESOURCE_ALLOCATION_TILED_Y,
    NUM_RESOURCE_ALLOCATION_TYPES
};

/*****************************************************************************\
ENUM: RESOURCE_MIPMAP_LAYOUT_MODE
\*****************************************************************************/
enum RESOURCE_MIPMAP_LAYOUT_MODE
{
    RESOURCE_MIPMAP_LAYOUT_BELOW,
    RESOURCE_MIPMAP_LAYOUT_RIGHT,
    NUM_RESOURCE_MIPMAP_LAYOUT_MODES
};

/*****************************************************************************\
ENUM: RESOURCE_SAMPLE_PATTERN
\*****************************************************************************/
enum RESOURCE_SAMPLE_PATTERN
{
    RESOURCE_SAMPLE_PATTERN_STANDARD,
    RESOURCE_SAMPLE_PATTERN_CENTER,
    NUM_RESOURCE_SAMPLE_PATTERNS
};

/*****************************************************************************\
ENUM: SURFACE_TYPE
\*****************************************************************************/
enum SURFACE_TYPE
{
    SURFACE_UNKNOWN        = 0,
    SURFACE_NULL           = 1,
    SURFACE_1D             = 2,
    SURFACE_1D_ARRAY       = 3,
    SURFACE_2D             = 4,
    SURFACE_2D_ARRAY       = 5,
    SURFACE_3D             = 6,
    SURFACE_CUBE           = 7,
    SURFACE_CUBE_ARRAY     = 8,
    SURFACE_CONSTANT       = 9,
    SURFACE_BUFFER         = 10,
    SURFACE_2D_MEDIA       = 11,
    SURFACE_2D_MEDIA_BLOCK = 12,
    NUM_SURFACE_TYPES
};

/*****************************************************************************\
ENUM: SURFACE_CUBE_FACE_ENABLES
\*****************************************************************************/
enum SURFACE_CUBE_FACE_ENABLES
{
    SURFACE_CUBE_FACE_ENABLE_NONE       = 0x00000000,
    SURFACE_CUBE_FACE_ENABLE_NEGATIVE_X = 0x00000001,
    SURFACE_CUBE_FACE_ENABLE_POSITIVE_X = 0x00000002,
    SURFACE_CUBE_FACE_ENABLE_NEGATIVE_Y = 0x00000004,
    SURFACE_CUBE_FACE_ENABLE_POSITIVE_Y = 0x00000008,
    SURFACE_CUBE_FACE_ENABLE_NEGATIVE_Z = 0x00000010,
    SURFACE_CUBE_FACE_ENABLE_POSITIVE_Z = 0x00000020,
    SURFACE_CUBE_FACE_ENABLE_ALL        = 0x0000003F,
};

} // namespace iCBE
