/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace iOpenCL
{

/*****************************************************************************\
ENUM: SAMPLER_TEXTURE_ADDRESS_MODE_TYPE
\*****************************************************************************/
enum SAMPLER_TEXTURE_ADDRESS_MODE
{
    SAMPLER_TEXTURE_ADDRESS_MODE_WRAP       = 0,
    SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR     = 1,
    SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP      = 2,
    SAMPLER_TEXTURE_ADDRESS_MODE_BORDER     = 3,
    SAMPLER_TEXTURE_ADDRESS_MODE_MIRRORONCE = 4,
    SAMPLER_TEXTURE_ADDRESS_MODE_MIRROR101  = 5,
    NUM_SAMPLER_TEXTURE_ADDRESS_MODES
};

/*****************************************************************************\
ENUM: SAMPLER_MAPFILTER_TYPE
\*****************************************************************************/
enum SAMPLER_MAPFILTER_TYPE
{
    SAMPLER_MAPFILTER_POINT         = 0,
    SAMPLER_MAPFILTER_LINEAR        = 1,
    SAMPLER_MAPFILTER_ANISOTROPIC   = 2,
    SAMPLER_MAPFILTER_GAUSSIANQUAD  = 3,
    SAMPLER_MAPFILTER_PYRAMIDALQUAD = 4,
    SAMPLER_MAPFILTER_MONO          = 5,
    NUM_SAMPLER_MAPFILTER_TYPES
};

/*****************************************************************************\
ENUM: SAMPLER_MIPFILTER_TYPE
\*****************************************************************************/
enum SAMPLER_MIPFILTER_TYPE
{
    SAMPLER_MIPFILTER_POINT,
    SAMPLER_MIPFILTER_LINEAR,
    SAMPLER_MIPFILTER_NONE,
    NUM_SAMPLER_MIPFILTER_TYPES
};

/*****************************************************************************\
ENUM: SAMPLER_COMPARE_FUNC_TYPE
\*****************************************************************************/
enum SAMPLER_COMPARE_FUNC_TYPE
{
    SAMPLER_COMPARE_FUNC_ALWAYS,
    SAMPLER_COMPARE_FUNC_NEVER,
    SAMPLER_COMPARE_FUNC_LESS,
    SAMPLER_COMPARE_FUNC_EQUAL,
    SAMPLER_COMPARE_FUNC_LEQUAL,
    SAMPLER_COMPARE_FUNC_GREATER,
    SAMPLER_COMPARE_FUNC_NOTEQUAL,
    SAMPLER_COMPARE_FUNC_GEQUAL,
    NUM_SAMPLER_COMPARE_FUNC_TYPES
};

} // namespace iCBE
