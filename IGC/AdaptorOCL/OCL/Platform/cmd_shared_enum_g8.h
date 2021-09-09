/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum_g8.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: Caps
\*****************************************************************************/
const DWORD g_cNumSamplersPerProgram    = 16;
const DWORD g_cNumSurfacesPerProgram    = 252;
const DWORD g_cNumSearchPathStatesGen6  = 14;
const DWORD g_cNumMBModeSetsGen6        = 4;

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_ANISORATIO
\*****************************************************************************/
enum GFXSHAREDSTATE_ANISORATIO
{
    GFXSHAREDSTATE_ANISORATIO_2     = 0x0,
    GFXSHAREDSTATE_ANISORATIO_4     = 0x1,
    GFXSHAREDSTATE_ANISORATIO_6     = 0x2,
    GFXSHAREDSTATE_ANISORATIO_8     = 0x3,
    GFXSHAREDSTATE_ANISORATIO_10    = 0x4,
    GFXSHAREDSTATE_ANISORATIO_12    = 0x5,
    GFXSHAREDSTATE_ANISORATIO_14    = 0x6,
    GFXSHAREDSTATE_ANISORATIO_16    = 0x7
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_CHROMAKEY_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_CHROMAKEY_MODE
{
    GFXSHAREDSTATE_CHROMAKEY_KILL_ON_ANY_MATCH  = 0x0,
    GFXSHAREDSTATE_CHROMAKEY_REPLACE_BLACK      = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_CUBECORNERMODE
\*****************************************************************************/
enum GFXSHAREDSTATE_CUBECORNERMODE
{
    GFXSHAREDSTATE_CUBECORNERMODE_REPLICATE = 0x0,
    GFXSHAREDSTATE_CUBECORNERMODE_AVERAGE   = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_CUBESURFACECONTROLMODE
\*****************************************************************************/
enum GFXSHAREDSTATE_CUBESURFACECONTROLMODE
{
    GFXSHAREDSTATE_CUBESURFACECONTROLMODE_PROGRAMMED    = 0x0,
    GFXSHAREDSTATE_CUBESURFACECONTROLMODE_OVERRIDE      = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_DEFAULTCOLOR_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_DEFAULTCOLOR_MODE
{
    GFXSHAREDSTATE_DEFAULTCOLOR_R32G32B32A32_FLOAT  = 0,
    GFXSHAREDSTATE_DEFAULTCOLOR_R8G8B8A8_UNORM      = 1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_MAPFILTER
\*****************************************************************************/
enum GFXSHAREDSTATE_MAPFILTER
{
    GFXSHAREDSTATE_MAPFILTER_NEAREST        = 0x0,
    GFXSHAREDSTATE_MAPFILTER_LINEAR         = 0x1,
    GFXSHAREDSTATE_MAPFILTER_ANISOTROPIC    = 0x2,
    //GFXSHAREDSTATE_MAPFILTER_RESERVED = 0x3,
    //GFXSHAREDSTATE_MAPFILTER_RESERVED = 0x4,
    //GFXSHAREDSTATE_MAPFILTER_RESERVED = 0x5,
    GFXSHAREDSTATE_MAPFILTER_MONO           = 0x6
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_MIPFILTER
\*****************************************************************************/
enum GFXSHAREDSTATE_MIPFILTER
{
    GFXSHAREDSTATE_MIPFILTER_NONE       = 0x0,
    GFXSHAREDSTATE_MIPFILTER_NEAREST    = 0x1,
    GFXSHAREDSTATE_MIPFILTER_LINEAR     = 0x3
};
/*****************************************************************************\
ENUM: GFXSHAREDSTATE_PREFILTER_OPERATION
\*****************************************************************************/
enum GFXSHAREDSTATE_PREFILTER_OPERATION
{
    GFXSHAREDSTATE_PREFILTER_ALWAYS     = 0x0,
    GFXSHAREDSTATE_PREFILTER_NEVER      = 0x1,
    GFXSHAREDSTATE_PREFILTER_LESS       = 0x2,
    GFXSHAREDSTATE_PREFILTER_EQUAL      = 0x3,
    GFXSHAREDSTATE_PREFILTER_LEQUAL     = 0x4,
    GFXSHAREDSTATE_PREFILTER_GREATER    = 0x5,
    GFXSHAREDSTATE_PREFILTER_NOTEQUAL   = 0x6,
    GFXSHAREDSTATE_PREFILTER_GEQUAL     = 0x7
};
/*****************************************************************************\
ENUM: GFXSHAREDSTATE_RENDERTARGET_ROTATE
\*****************************************************************************/
enum GFXSHAREDSTATE_RENDERTARGET_ROTATE
{
    GFXSHAREDSTATE_RENDERTARGET_ROTATE_0DEG     = 0,
    GFXSHAREDSTATE_RENDERTARGET_ROTATE_90DEG    = 1,
    GFXSHAREDSTATE_RENDERTARGET_ROTATE_270DEG   = 3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT
{
    GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT_BELOW   = 0x0,
    GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT_RIGHT   = 0x1
};
/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACEFORMAT
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACEFORMAT
{
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_FLOAT             = 0x000,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_SINT              = 0x001,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_UINT              = 0x002,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_UNORM             = 0x003,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_SNORM             = 0x004,
    GFXSHAREDSTATE_SURFACEFORMAT_R64G64_FLOAT                   = 0x005,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32X32_FLOAT             = 0x006,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_SSCALED           = 0x007,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32A32_USCALED           = 0x008,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_FLOAT                = 0x040,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_SINT                 = 0x041,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_UINT                 = 0x042,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_UNORM                = 0x043,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_SNORM                = 0x044,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_SSCALED              = 0x045,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32B32_USCALED              = 0x046,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_UNORM             = 0x080,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_SNORM             = 0x081,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_SINT              = 0x082,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_UINT              = 0x083,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_FLOAT             = 0x084,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_FLOAT                   = 0x085,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_SINT                    = 0x086,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_UINT                    = 0x087,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_FLOAT_X8X24_TYPELESS       = 0x088,
    GFXSHAREDSTATE_SURFACEFORMAT_X32_TYPELESS_G8X24_UINT        = 0x089,
    GFXSHAREDSTATE_SURFACEFORMAT_L32A32_FLOAT                   = 0x08A,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_UNORM                   = 0x08B,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_SNORM                   = 0x08C,
    GFXSHAREDSTATE_SURFACEFORMAT_R64_FLOAT                      = 0x08D,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16X16_UNORM             = 0x08E,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16X16_FLOAT             = 0x08F,
    GFXSHAREDSTATE_SURFACEFORMAT_A32X32_FLOAT                   = 0x090,
    GFXSHAREDSTATE_SURFACEFORMAT_L32X32_FLOAT                   = 0x091,
    GFXSHAREDSTATE_SURFACEFORMAT_I32X32_FLOAT                   = 0x092,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_SSCALED           = 0x093,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16A16_USCALED           = 0x094,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_SSCALED                 = 0x095,
    GFXSHAREDSTATE_SURFACEFORMAT_R32G32_USCALED                 = 0x096,
    GFXSHAREDSTATE_SURFACEFORMAT_B8G8R8A8_UNORM                 = 0x0C0,
    GFXSHAREDSTATE_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB            = 0x0C1,
    GFXSHAREDSTATE_SURFACEFORMAT_R10G10B10A2_UNORM              = 0x0C2,
    GFXSHAREDSTATE_SURFACEFORMAT_R10G10B10A2_UNORM_SRGB         = 0x0C3,
    GFXSHAREDSTATE_SURFACEFORMAT_R10G10B10A2_UINT               = 0x0C4,
    GFXSHAREDSTATE_SURFACEFORMAT_R10G10B10_SNORM_A2_UNORM       = 0x0C5,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM                 = 0x0C7,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB            = 0x0C8,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_SNORM                 = 0x0C9,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_SINT                  = 0x0CA,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UINT                  = 0x0CB,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_UNORM                   = 0x0CC,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_SNORM                   = 0x0CD,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_SINT                    = 0x0CE,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_UINT                    = 0x0CF,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_FLOAT                   = 0x0D0,
    GFXSHAREDSTATE_SURFACEFORMAT_B10G10R10A2_UNORM              = 0x0D1,
    GFXSHAREDSTATE_SURFACEFORMAT_B10G10R10A2_UNORM_SRGB         = 0x0D2,
    GFXSHAREDSTATE_SURFACEFORMAT_R11G11B10_FLOAT                = 0x0D3,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_SINT                       = 0x0D6,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_UINT                       = 0x0D7,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_FLOAT                      = 0x0D8,
    GFXSHAREDSTATE_SURFACEFORMAT_R24_UNORM_X8_TYPELESS          = 0x0D9,
    GFXSHAREDSTATE_SURFACEFORMAT_X24_TYPELESS_G8_UINT           = 0x0DA,
    GFXSHAREDSTATE_SURFACEFORMAT_L16A16_UNORM                   = 0x0DF,
    GFXSHAREDSTATE_SURFACEFORMAT_I24X8_UNORM                    = 0x0E0,
    GFXSHAREDSTATE_SURFACEFORMAT_L24X8_UNORM                    = 0x0E1,
    GFXSHAREDSTATE_SURFACEFORMAT_A24X8_UNORM                    = 0x0E2,
    GFXSHAREDSTATE_SURFACEFORMAT_I32_FLOAT                      = 0x0E3,
    GFXSHAREDSTATE_SURFACEFORMAT_L32_FLOAT                      = 0x0E4,
    GFXSHAREDSTATE_SURFACEFORMAT_A32_FLOAT                      = 0x0E5,
    GFXSHAREDSTATE_SURFACEFORMAT_B8G8R8X8_UNORM                 = 0x0E9,
    GFXSHAREDSTATE_SURFACEFORMAT_B8G8R8X8_UNORM_SRGB            = 0x0EA,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8X8_UNORM                 = 0x0EB,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8X8_UNORM_SRGB            = 0x0EC,
    GFXSHAREDSTATE_SURFACEFORMAT_R9G9B9E5_SHAREDEXP             = 0x0ED,
    GFXSHAREDSTATE_SURFACEFORMAT_B10G10R10X2_UNORM              = 0x0EE,
    GFXSHAREDSTATE_SURFACEFORMAT_L16A16_FLOAT                   = 0x0F0,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_UNORM                      = 0x0F1,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_SNORM                      = 0x0F2,
    GFXSHAREDSTATE_SURFACEFORMAT_R10G10B10X2_USCALED            = 0x0F3,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_SSCALED               = 0x0F4,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_USCALED               = 0x0F5,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_SSCALED                 = 0x0F6,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16_USCALED                 = 0x0F7,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_SSCALED                    = 0x0F8,
    GFXSHAREDSTATE_SURFACEFORMAT_R32_USCALED                    = 0x0F9,
    GFXSHAREDSTATE_SURFACEFORMAT_R8B8G8A8_UNORM                 = 0x0FA,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G6R5_UNORM                   = 0x100,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G6R5_UNORM_SRGB              = 0x101,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G5R5A1_UNORM                 = 0x102,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G5R5A1_UNORM_SRGB            = 0x103,
    GFXSHAREDSTATE_SURFACEFORMAT_B4G4R4A4_UNORM                 = 0x104,
    GFXSHAREDSTATE_SURFACEFORMAT_B4G4R4A4_UNORM_SRGB            = 0x105,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_UNORM                     = 0x106,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_SNORM                     = 0x107,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_SINT                      = 0x108,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_UINT                      = 0x109,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_UNORM                      = 0x10A,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_SNORM                      = 0x10B,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_SINT                       = 0x10C,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_UINT                       = 0x10D,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_FLOAT                      = 0x10E,
    GFXSHAREDSTATE_SURFACEFORMAT_I16_UNORM                      = 0x111,
    GFXSHAREDSTATE_SURFACEFORMAT_L16_UNORM                      = 0x112,
    GFXSHAREDSTATE_SURFACEFORMAT_A16_UNORM                      = 0x113,
    GFXSHAREDSTATE_SURFACEFORMAT_L8A8_UNORM                     = 0x114,
    GFXSHAREDSTATE_SURFACEFORMAT_I16_FLOAT                      = 0x115,
    GFXSHAREDSTATE_SURFACEFORMAT_L16_FLOAT                      = 0x116,
    GFXSHAREDSTATE_SURFACEFORMAT_A16_FLOAT                      = 0x117,
    GFXSHAREDSTATE_SURFACEFORMAT_L8A8_UNORM_SRGB                = 0x118,
    GFXSHAREDSTATE_SURFACEFORMAT_R5G5_SNORM_B6_UNORM            = 0x119,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G5R5X1_UNORM                 = 0x11A,
    GFXSHAREDSTATE_SURFACEFORMAT_B5G5R5X1_UNORM_SRGB            = 0x11B,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_SSCALED                   = 0x11C,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8_USCALED                   = 0x11D,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_SSCALED                    = 0x11E,
    GFXSHAREDSTATE_SURFACEFORMAT_R16_USCALED                    = 0x11F,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_UNORM                       = 0x140,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_SNORM                       = 0x141,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_SINT                        = 0x142,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_UINT                        = 0x143,
    GFXSHAREDSTATE_SURFACEFORMAT_A8_UNORM                       = 0x144,
    GFXSHAREDSTATE_SURFACEFORMAT_I8_UNORM                       = 0x145,
    GFXSHAREDSTATE_SURFACEFORMAT_L8_UNORM                       = 0x146,
    GFXSHAREDSTATE_SURFACEFORMAT_P4A4_UNORM                     = 0x147,
    GFXSHAREDSTATE_SURFACEFORMAT_A4P4_UNORM                     = 0x148,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_SSCALED                     = 0x149,
    GFXSHAREDSTATE_SURFACEFORMAT_R8_USCALED                     = 0x14A,
    GFXSHAREDSTATE_SURFACEFORMAT_P8_UNORM                       = 0x14B,
    GFXSHAREDSTATE_SURFACEFORMAT_L8_UNORM_SRGB                  = 0x14C,
    GFXSHAREDSTATE_SURFACEFORMAT_DXT1_RGB_SRGB                  = 0x180,
    GFXSHAREDSTATE_SURFACEFORMAT_R1_UINT                        = 0x181,
    GFXSHAREDSTATE_SURFACEFORMAT_YCRCB_NORMAL                   = 0x182,
    GFXSHAREDSTATE_SURFACEFORMAT_YCRCB_SWAPUVY                  = 0x183,
    GFXSHAREDSTATE_SURFACEFORMAT_P2_UNORM                       = 0x184,
    GFXSHAREDSTATE_SURFACEFORMAT_BC1_UNORM                      = 0x186,
    GFXSHAREDSTATE_SURFACEFORMAT_BC2_UNORM                      = 0x187,
    GFXSHAREDSTATE_SURFACEFORMAT_BC3_UNORM                      = 0x188,
    GFXSHAREDSTATE_SURFACEFORMAT_BC4_UNORM                      = 0x189,
    GFXSHAREDSTATE_SURFACEFORMAT_BC5_UNORM                      = 0x18A,
    GFXSHAREDSTATE_SURFACEFORMAT_BC1_UNORM_SRGB                 = 0x18B,
    GFXSHAREDSTATE_SURFACEFORMAT_BC2_UNORM_SRGB                 = 0x18C,
    GFXSHAREDSTATE_SURFACEFORMAT_BC3_UNORM_SRGB                 = 0x18D,
    GFXSHAREDSTATE_SURFACEFORMAT_MONO8                          = 0x18E,
    GFXSHAREDSTATE_SURFACEFORMAT_YCRCB_SWAPUV                   = 0x18F,
    GFXSHAREDSTATE_SURFACEFORMAT_YCRCB_SWAPY                    = 0x190,
    GFXSHAREDSTATE_SURFACEFORMAT_DXT1_RGB                       = 0x191,
    GFXSHAREDSTATE_SURFACEFORMAT_FXT1                           = 0x192,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8_UNORM                   = 0x193,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8_SNORM                   = 0x194,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8_SSCALED                 = 0x195,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8_USCALED                 = 0x196,
    GFXSHAREDSTATE_SURFACEFORMAT_R64G64B64A64_FLOAT             = 0x197,
    GFXSHAREDSTATE_SURFACEFORMAT_R64G64B64_FLOAT                = 0x198,
    GFXSHAREDSTATE_SURFACEFORMAT_BC4_SNORM                      = 0x199,
    GFXSHAREDSTATE_SURFACEFORMAT_BC5_SNORM                      = 0x19A,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16_UNORM                = 0x19C,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16_SNORM                = 0x19D,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16_SSCALED              = 0x19E,
    GFXSHAREDSTATE_SURFACEFORMAT_R16G16B16_USCALED              = 0x19F,
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8_UNORM_SRGB              = 0x1A0,
    GFXSHAREDSTATE_SURFACEFORMAT_BC6H_SF16                      = 0x1A1,
    GFXSHAREDSTATE_SURFACEFORMAT_BC7_UNORM                      = 0x1A2,
    GFXSHAREDSTATE_SURFACEFORMAT_BC7_UNORM_SRGB                 = 0x1A3,
    GFXSHAREDSTATE_SURFACEFORMAT_BC6H_UF16                      = 0x1A4,
    GFXSHAREDSTATE_SURFACEFORMAT_PLANAR_420_8                   = 0x1A5,
    GFXSHAREDSTATE_SURFACEFORMAT_RAW                            = 0x1FF,
    NUM_GFXSHAREDSTATE_SURFACEFORMATS
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACERETURNFORMAT
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACERETURNFORMAT
{
    GFXSHAREDSTATE_SURFACERETURNFORMAT_FLOAT32  = 0x0,
    GFXSHAREDSTATE_SURFACERETURNFORMAT_S1_14    = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACETYPE
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACETYPE
{
    GFXSHAREDSTATE_SURFACETYPE_1D       = 0x0,
    GFXSHAREDSTATE_SURFACETYPE_2D       = 0x1,
    GFXSHAREDSTATE_SURFACETYPE_3D       = 0x2,
    GFXSHAREDSTATE_SURFACETYPE_CUBE     = 0x3,
    GFXSHAREDSTATE_SURFACETYPE_BUFFER   = 0x4,
    GFXSHAREDSTATE_SURFACETYPE_STRBUF   = 0x5,
    //GFXSHAREDSTATE_RESERVED = 0x6,
    GFXSHAREDSTATE_SURFACETYPE_NULL     = 0x7,
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TEXCOORDMODE
\*****************************************************************************/
enum GFXSHAREDSTATE_TEXCOORDMODE
{
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP            = 0x0,
    GFXSHAREDSTATE_TEXCOORDMODE_MIRROR          = 0x1,
    GFXSHAREDSTATE_TEXCOORDMODE_CLAMP           = 0x2,
    GFXSHAREDSTATE_TEXCOORDMODE_CUBE            = 0x3,
    GFXSHAREDSTATE_TEXCOORDMODE_CLAMP_BORDER    = 0x4,
    GFXSHAREDSTATE_TEXCOORDMODE_MIRROR_ONCE     = 0x5,
    GFXSHAREDSTATE_TEXCOORDMODE_MIRROR_101      = 0x7
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_FLEXFILTERMODE
\*****************************************************************************/
enum GFXSHAREDSTATE_FLEXFILTERMODE
{
    GFXSHAREDSTATE_FLEXFILTERMODE_SEP           = 0x0,
    GFXSHAREDSTATE_FLEXFILTERMODE_NONSEP        = 0x1,
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TILEWALK
\*****************************************************************************/
enum GFXSHAREDSTATE_TILEWALK
{
    GFXSHAREDSTATE_TILEWALK_XMAJOR          = 0x0,
    GFXSHAREDSTATE_TILEWALK_YMAJOR          = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TRILINEAR_QUALITY
\*****************************************************************************/
enum GFXSHAREDSTATE_TRILINEAR_QUALITY
{
    GFXSHAREDSTATE_TRILINEAR_QUALITY_FULL   = 0x0,
    GFXSHAREDSTATE_TRILINEAR_QUALITY_HIGH   = 0x1,
    GFXSHAREDSTATE_TRILINEAR_QUALITY_MED    = 0x2,
    GFXSHAREDSTATE_TRILINEAR_QUALITY_LOW    = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TEXTURE_BORDER_COLOR_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_TEXTURE_BORDER_COLOR_MODE
{
    GFXSHAREDSTATE_TEXTURE_BORDER_COLOR_MODE_DX10OGL    = 0x0,
    GFXSHAREDSTATE_TEXTURE_BORDER_COLOR_MODE_DX9        = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_NUM_MULTISAMPLES
\*****************************************************************************/
enum GFXSHAREDSTATE_NUM_MULTISAMPLES
{
    GFXSHAREDSTATE_NUMSAMPLES_1 = 0x0,
    //GFXSHAREDSTATE_RESERVED = 0x1,
    GFXSHAREDSTATE_NUMSAMPLES_4 = 0x2,
    GFXSHAREDSTATE_NUMSAMPLES_8 = 0x3
    //GFXSHAREDSTATE_RESERVED = 0x4,
    //GFXSHAREDSTATE_RESERVED = 0x5,
    //GFXSHAREDSTATE_RESERVED = 0x6,
    //GFXSHAREDSTATE_RESERVED = 0x7
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_MSFMT
\*****************************************************************************/
enum GFXSHAREDSTATE_MSFMT
{
    GFXSHAREDSTATE_MSFMT_MSS            = 0x0,
    GFXSHAREDSTATE_MSFMT_DEPTH_STENCIL  = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_CACHEABILITY_CONTROL
\*****************************************************************************/
enum GFXSHAREDSTATE_CACHEABILITY_CONTROL
{
    GFXSHAREDSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY       = 0x0,
    GFXSHAREDSTATE_CACHEABILITY_CONTROL_NEITHER_LLC_NOR_MLC = 0x1,
    GFXSHAREDSTATE_CACHEABILITY_CONTROL_LLC_NOT_MLC         = 0x2,
    GFXSHAREDSTATE_CACHEABILITY_CONTROL_LLC_AND_MLC         = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE
\*****************************************************************************/
enum GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE
{
    GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE_GTT     = 0x0,
    GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE_SURFACE = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
{
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_NORMAL              = 0x0,
    //GFXSHAREDSTATE_RESERVED = 0x1,
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_PROGRESSIVE_FRAME   = 0x2,
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_INTERLACED_FRAME    = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
{
    GFXSHAREDSTATE_RENDER_CACHE_WRITE_ONLY_ON_MISS      = 0x0,
    GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_ONLY_ON_MISS = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACE_ARRAY_SPACING
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACE_ARRAY_SPACING
{
    GFXSHAREDSTATE_SURFACE_ARRAY_SPACING_FULL   = 0x0,
    GFXSHAREDSTATE_SURFACE_ARRAY_SPACING_LOD0   = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT
{
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_2     = 0x0,
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_4     = 0x1,
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_8     = 0x2,
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_16    = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT
\*****************************************************************************/
enum GFXSHAREDSTATE_SURFACE_HORIZONTRAL_ALIGNMENT
{
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_2     = 0x0,
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_4     = 0x1,
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_8     = 0x2,
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_16    = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_RENDER_TARGET_ROTATION
\*****************************************************************************/
enum GFXSHAREDSTATE_RENDER_TARGET_ROTATION
{
    GFXSHAREDSTATE_RROTATE_0DEG      = 0x0,
    GFXSHAREDSTATE_RROTATE_90DEG     = 0x1,
    GFXSHAREDSTATE_RROTATE_270DEG    = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL
\*****************************************************************************/
enum GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL
{
    GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL_NOT_CACHEABLE    = 0x0,
    GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL_CACHEABLE        = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_LLC_CACHEABILITY_CONTROL
\*****************************************************************************/
enum GFXSHAREDSTATE_LLC_CACHEABILITY_CONTROL
{
    GFXSHAREDSTATE_LLC_CACHEABILITY_CONTROL_USE_GTT_ENTRY   = 0x0,
    GFXSHAREDSTATE_LLC_CACHEABILITY_CONTROL_CACHEABLE       = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_CLEARCOLOR
\*****************************************************************************/
enum GFXSHAREDSTATE_CLEARCOLOR
{
    GFXSHAREDSTATE_CLEARCOLOR_ZERO  = 0x0,
    GFXSHAREDSTATE_CLEARCOLOR_ONE   = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_SHADERCHANNELSELECT
\*****************************************************************************/
enum GFXSHAREDSTATE_SHADERCHANNELSELECT
{
    GFXSHAREDSTATE_SHADERCHANNELSELECT_ZERO     = 0x0,
    GFXSHAREDSTATE_SHADERCHANNELSELECT_ONE      = 0x1,
    GFXSHAREDSTATE_SHADERCHANNELSELECT_RED      = 0x4,
    GFXSHAREDSTATE_SHADERCHANNELSELECT_GREEN    = 0x5,
    GFXSHAREDSTATE_SHADERCHANNELSELECT_BLUE     = 0x6,
    GFXSHAREDSTATE_SHADERCHANNELSELECT_ALPHA    = 0x7
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TILEMODE
\*****************************************************************************/
enum GFXSHAREDSTATE_TILEMODE
{
    GFXSHAREDSTATE_TILEMODE_LINEAR          = 0x0,
    GFXSHAREDSTATE_TILEMODE_WMAJOR          = 0x1,
    GFXSHAREDSTATE_TILEMODE_XMAJOR          = 0x2,
    GFXSHAREDSTATE_TILEMODE_YMAJOR          = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_REDUCTION_TYPE
\*****************************************************************************/
enum GFXSHAREDSTATE_REDUCTION_TYPE
{
    GFXSHAREDSTATE_STD_FILTER       = 0x0,
    GFXSHAREDSTATE_COMPARISON       = 0x1,
    GFXSHAREDSTATE_MINIMUM          = 0x2,
    GFXSHAREDSTATE_MAXIMUM          = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_COHERENCY_TYPE
\*****************************************************************************/
enum GFXSHAREDSTATE_COHERENCY_TYPE
{
    GFXSHAREDSTATE_NON_COHERENT    = 0x0,
    GFXSHAREDSTATE_IA_CHERENT      = 0x1
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT
\*****************************************************************************/
enum GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT
{
    GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT_64       = 0x0,
    GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT_128      = 0x1,
    GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT_256      = 0x2,
    GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT_512      = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT
\*****************************************************************************/
enum GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT
{
    GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT_64       = 0x0,
    GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT_128      = 0x1,
    GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT_256      = 0x2,
    GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT_512      = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_AUXILIARY_SURFACE_MODE
\*****************************************************************************/
enum GFXSHAREDSTATE_AUXILIARY_SURFACE_MODE
{
    GFXSHAREDSTATE_AUX_NONE       = 0x0,
    GFXSHAREDSTATE_AUX_CCS        = 0x1,
    GFXSHAREDSTATE_AUX_APPEND     = 0x2,
    GFXSHAREDSTATE_AUX_HIZ        = 0x3
};

/*****************************************************************************\
ENUM: GFXSHAREDSTATE_MAPFILTER
\*****************************************************************************/
enum GFXSHAREDSTATE_ANISOTROPIC_ALGORITHM
{
    GFXSHAREDSTATE_ANISOTROPIC_LEGACY               = 0x0,
    GFXSHAREDSTATE_ANISOTROPIC_EWA_APPROXIMATION    = 0x1
};
} // namespace G6HWC
