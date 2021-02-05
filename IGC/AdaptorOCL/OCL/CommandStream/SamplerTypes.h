/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
