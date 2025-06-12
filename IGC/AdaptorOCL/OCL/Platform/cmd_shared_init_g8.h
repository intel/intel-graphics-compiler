/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_shared_enum_g8.h"
#include "cmd_shared_def_g8.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: SSharedStateBindingTableState
\*****************************************************************************/
static const SSharedStateBindingTableState g_cInitSharedStateBindingTableState =
{
    // DW0
    {
        {
            0,                                          // _Unused
            0                                           // SurfaceStatePointer
        }
    }
};

/*****************************************************************************\
CONST: g_cInitGfxSamplerIndirectState
\*****************************************************************************/
static const SGfxSamplerIndirectState g_cInitGfxSamplerIndirectState =
{
    0.0f, 0.0f, 0.0f, 0.0f, // Border Colors RGBA
    0, 0, 0, 0, 0, 0, 0, 0  // Unused DWORDS 4-11
};

/*****************************************************************************\
CONST: g_cInitSamplerState
\*****************************************************************************/
static const SSharedStateSamplerState g_cInitSharedStateSamplerState =
{
    // DWORD 0
    {
        {
            GFXSHAREDSTATE_PREFILTER_ALWAYS,                // ShadowFunction
            0,                                              // TextureLODBias
            GFXSHAREDSTATE_MAPFILTER_NEAREST,               // MinModeFilter
            GFXSHAREDSTATE_MAPFILTER_NEAREST,               // MagModeFilter
            GFXSHAREDSTATE_MIPFILTER_NONE,                  // MipModeFilter
            0,                                              // BaseMipLevel
            0,                                              // _Unused1
            false,                                          // LODPreClampEnable
            GFXSHAREDSTATE_DEFAULTCOLOR_R32G32B32A32_FLOAT, // TextureBorderColorMode
            0,                                              // _Unused2
            true                                            // SamplerDisable
        }
    },

    // DWORD 1
    {
        {
            GFXSHAREDSTATE_TEXCOORDMODE_WRAP,               // TCZAddressControlMode
            GFXSHAREDSTATE_TEXCOORDMODE_WRAP,               // TCYAddressControlMode
            GFXSHAREDSTATE_TEXCOORDMODE_WRAP,               // TCXAddressControlMode
            GFXSHAREDSTATE_CUBESURFACECONTROLMODE_OVERRIDE, // CubeSurfaceControlMode
            0,                                              // _Unused
            0,                                              // MaxLOD
            0                                               // MinLOD
        }
    },

    // DWORD 2
    {
        {
            0,                                              // _Unused
            0                                               // BorderColorPointer
        }
    },

    // DWORD 3,
    {
        {
            false,                                          // NonNormalizedCoordinatesEnable
            0,                                              // _Unused1
            false,                                          // RAddressMinFilterAddressRoundingEnable
            false,                                          // RAddressMagFilterAddressRoundingEnable
            false,                                          // VAddressMinFilterAddressRoundingEnable
            false,                                          // VAddressMagFilterAddressRoundingEnable
            false,                                          // UAddressMinFilterAddressRoundingEnable
            false,                                          // UAddressMagFilterAddressRoundingEnable
            GFXSHAREDSTATE_ANISORATIO_2,                    // MaximumAnisotropy
            GFXSHAREDSTATE_CHROMAKEY_KILL_ON_ANY_MATCH,     // ChromaKeyMode
            0,                                              // ChromaKeyIndex
            false,                                          // ChromaKeyEnable
            0                                               // _Unused2
        }
    }
};

/*****************************************************************************\
CONST: g_cInitSamplerStateDW0Gen7
\*****************************************************************************/
static const SSharedStateSamplerState::_DW0::_Gen7 g_cInitSharedStateSamplerStateDW0Gen7 =
{
    0,                                                  // _Unused1
    0,                                                  // TextureLODBias
    GFXSHAREDSTATE_MAPFILTER_NEAREST,                   // MinModeFilter
    GFXSHAREDSTATE_MAPFILTER_NEAREST,                   // MagModeFilter
    GFXSHAREDSTATE_MIPFILTER_NONE,                      // MipModeFilter
    0,                                                  // BaseMipLevel
    0,                                                  // _Unused2
    false,                                              // LODPreClampEnable
    GFXSHAREDSTATE_DEFAULTCOLOR_R32G32B32A32_FLOAT,     // TextureBorderColorMode
    0,                                                  // _Unused3
    true                                                // SamplerDisable
};

/*****************************************************************************\
CONST: g_cInitSamplerStateDW0Gen9
\*****************************************************************************/
static const SSharedStateSamplerState::_DW0::_Gen9 g_cInitSharedStateSamplerStateDW0Gen9 =
{
    GFXSHAREDSTATE_ANISOTROPIC_LEGACY,                  // AnisotropicAlgorithm
    0,                                                  // TextureLODBias
    GFXSHAREDSTATE_MAPFILTER_NEAREST,                   // MinModeFilter
    GFXSHAREDSTATE_MAPFILTER_NEAREST,                   // MagModeFilter
    GFXSHAREDSTATE_MIPFILTER_NONE,                      // MipModeFilter
    0,                                                  // CoarseLODQualityMode
    0,                                                  // _Unused1
    false,                                              // LODPreClampEnable
    GFXSHAREDSTATE_DEFAULTCOLOR_R32G32B32A32_FLOAT,     // TextureBorderColorMode
    0,                                                  // _Unused2
    true                                                // SamplerDisable
};

/*****************************************************************************\
CONST: g_cInitSamplerStateDW1Gen7
\*****************************************************************************/
static const SSharedStateSamplerState::_DW1::_Gen7 g_cInitSharedStateSamplerStateDW1Gen7 =
{
    GFXSHAREDSTATE_CUBESURFACECONTROLMODE_OVERRIDE,     // CubeSurfaceControlMode
    GFXSHAREDSTATE_PREFILTER_ALWAYS,                    // ShadowFunction
    0,                                                  // _Unused
    0,                                                  // MaxLOD
    0                                                   // MinLOD
};

/*****************************************************************************\
CONST: g_cInitSamplerStateDW3Gen7
\*****************************************************************************/
static const SSharedStateSamplerState::_DW3::_Gen7 g_cInitSharedStateSamplerStateDW3Gen7 =
{
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                   // TCZAddressControlMode
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                   // TCYAddressControlMode
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                   // TCXAddressControlMode
    0,                                                  // _Unused1
    false,                                              // NonnormalizedCoordinateEnable
    GFXSHAREDSTATE_TRILINEAR_QUALITY_LOW,               // TrilinearFilterQuality
    false,                                              // RAddressMinFilterAddressRoundingEnable
    false,                                              // RAddressMagFilterAddressRoundingEnable
    false,                                              // VAddressMinFilterAddressRoundingEnable
    false,                                              // VAddressMagFilterAddressRoundingEnable
    false,                                              // UAddressMinFilterAddressRoundingEnable
    false,                                              // UAddressMagFilterAddressRoundingEnable
    GFXSHAREDSTATE_ANISORATIO_2,                        // MaximumAnisotropy
    GFXSHAREDSTATE_CHROMAKEY_KILL_ON_ANY_MATCH,         // ChromaKeyMode
    0,                                                  // ChromaKeyIndex
    false,                                              // ChromaKeyEnable
    0                                                   // _Unused2
};

/*****************************************************************************\
CONST: g_cInitSamplerStateDW3Gen9
\*****************************************************************************/
static const SSharedStateSamplerState::_DW3::_Gen9 g_cInitSharedStateSamplerStateDW3Gen9 =
{
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                       // TCZAddressControlMode
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                       // TCYAddressControlMode
    GFXSHAREDSTATE_TEXCOORDMODE_WRAP,                       // TCXAddressControlMode
    false,                                                  // ReductionTypeEnable
    false,                                                  // NonNormalizedCoordinateEnable
    GFXSHAREDSTATE_TRILINEAR_QUALITY_LOW,                   // TrilinearFilterQuality
    false,                                                  // RAddressMinFilterAddressRoundingEnable
    false,                                                  // RAddressMagFilterAddressRoundingEnable
    false,                                                  // VAddressMinFilterAddressRoundingEnable
    false,                                                  // VAddressMagFilterAddressRoundingEnable
    false,                                                  // UAddressMinFilterAddressRoundingEnable
    false,                                                  // UAddressMagFilterAddressRoundingEnable
    GFXSHAREDSTATE_ANISORATIO_2,                            // MaximumAnisotropy
    GFXSHAREDSTATE_STD_FILTER,                              // ReductionType
    0                                                       // NonSeparableFilterFootprintMask
};

/*****************************************************************************\
CONST: g_cInitVmeState
\*****************************************************************************/
static const SSharedStateSearchPathLUTState::_DW0::_Byte g_cInitSharedSearchPathLUTStates[ g_cNumSearchPathStatesGen6 ] =
{
// clang-format off
    { 0x01,0x01,0x01,0x01 }, // 0
    { 0x01,0x01,0x01,0x10 }, // 1
    { 0x0F,0x0F,0x0F,0x0F }, // 2
    { 0x0F,0x0F,0x0F,0x10 }, // 3
    { 0x01,0x01,0x01,0x01 }, // 4
    { 0x01,0x01,0x01,0x10 }, // 5
    { 0x0F,0x0F,0x0F,0x0F }, // 6
    { 0x0F,0x0F,0x0F,0x10 }, // 7
    { 0x01,0x01,0x01,0x01 }, // 8
    { 0x01,0x01,0x01,0x10 }, // 9
    { 0x0F,0x0F,0x0F,0x0F }, // 10
    { 0x0F,0x0F,0x0F,0x00 }, // 11
    { 0x00,0x00,0x00,0x00 }, // 12
    { 0x00,0x00,0x00,0x00 }  // 13
// clang-format on
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceState
\*****************************************************************************/
static const SSharedStateSurfaceState   g_cInitSharedStateSurfaceState =
{
    // DWORD 0
    {
        {
            0,                                              // CubeFaceEnablesPositiveZ
            0,                                              // CubeFaceEnablesNegativeZ
            0,                                              // CubeFaceEnablesPositiveY
            0,                                              // CubeFaceEnablesNegativeY
            0,                                              // CubeFaceEnablesPositiveX
            0,                                              // CubeFaceEnablesNegativeX
            GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_NORMAL,     // MediaBoundaryPixelMode
            GFXSHAREDSTATE_RENDER_CACHE_WRITE_ONLY_ON_MISS, // RenderCacheReadWriteMode
            GFXSHAREDSTATE_CUBECORNERMODE_REPLICATE,        // CubeMapCornerMode
            GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT_BELOW,      // MipMapLayoutMode
            0,                                              // VerticalLineStrideOffset
            0,                                              // VerticalLineStride
            0,                                              // _Unused1
            GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM,    // SurfaceFormat
            GFXSHAREDSTATE_SURFACERETURNFORMAT_FLOAT32,     // DataReturnFormat
            0,                                              // _Unused2
            GFXSHAREDSTATE_SURFACETYPE_NULL                 // SurfaceType
        }
    },

    // DWORD 1
    {
        {
            0                                               // SurfaceBaseAddress
        }
    },

    // DWORD 2
    {
        {
            GFXSHAREDSTATE_RENDERTARGET_ROTATE_0DEG,        // RenderTargetRotation
            0,                                              // MipCount
            0,                                              // Width
            0                                               // Height
        }
    },

    // DWORD 3
    {
        {
            GFXSHAREDSTATE_TILEWALK_YMAJOR,                 // TileWalk
            false,                                          // TiledSurface
            0,                                              // _Unused1
            0,                                              // SurfacePitch
            0,                                              // _Unused2
            0                                               // Depth
        }
    },

    // DWORD 4
    {
        {
            0,                                              // MultisamplePositionPaletteIndex
            0,                                              // _Unused1
            GFXSHAREDSTATE_NUMSAMPLES_1,                    // NumMultisamples
            0,                                              // _Unused2
            0,                                              // RenderTargetViewExtent
            0,                                              // MinimumArrayElement
            0                                               // SurfaceMinLOD
        }
    },

    // DWORD 5
    {
        {
            0,                                              // _Unused
            GFXSHAREDSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,  // SurfaceCacheabilityControl
            GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE_GTT,    // SurfaceGraphicsDataType
            false,                                          // SurfaceEncryptedDataEnable
            0,                                              // YOffset
            GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_2,    // SurfaceVerticalAlignment
            0                                               // XOffset
        }
    },

    // DWORD 6
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 7
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 8
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 9
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 10
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 11
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 12
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 13
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 14
    {
        {
            0                                               // Reserved
        }
    },

    // DWORD 15
    {
        {
            0                                               // Reserved
        }
    }
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW0Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW0::_Gen7   g_cInitSharedStateSurfaceStateDW0Gen7 =
{
    0,                                                  // CubeFaceEnablesPositiveZ
    0,                                                  // CubeFaceEnablesNegativeZ
    0,                                                  // CubeFaceEnablesPositiveY
    0,                                                  // CubeFaceEnablesNegativeY
    0,                                                  // CubeFaceEnablesPositiveX
    0,                                                  // CubeFaceEnablesNegativeX
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_NORMAL,         // MediaBoundaryPixelMode
    GFXSHAREDSTATE_RENDER_CACHE_WRITE_ONLY_ON_MISS,     // RenderCacheReadWriteMode
    0,                                                  // _Unused1
    GFXSHAREDSTATE_SURFACE_ARRAY_SPACING_FULL,          // SurfaceArraySpacing
    0,                                                  // VerticalLineStrideOffset
    0,                                                  // VerticalLineStride
    GFXSHAREDSTATE_TILEWALK_YMAJOR,                     // TileWalk
    false,                                              // TiledSurface
    0,                                                  // _Unused2
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM,        // SurfaceFormat
    0,                                                  // _Unused3
    false,                                              // SurfaceArray
    GFXSHAREDSTATE_SURFACETYPE_NULL                     // SurfaceType
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW0Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW0::_Gen8   g_cInitSharedStateSurfaceStateDW0Gen8 =
{
    0,                                                  // CubeFaceEnablesPositiveZ
    0,                                                  // CubeFaceEnablesNegativeZ
    0,                                                  // CubeFaceEnablesPositiveY
    0,                                                  // CubeFaceEnablesNegativeY
    0,                                                  // CubeFaceEnablesPositiveX
    0,                                                  // CubeFaceEnablesNegativeX
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_NORMAL,         // MediaBoundaryPixelMode
    GFXSHAREDSTATE_RENDER_CACHE_WRITE_ONLY_ON_MISS,     // RenderCacheReadWriteMode
    0,                                                  // SurfaceArraySpace
    0,                                                  // VerticalLineStrideOffset
    0,                                                  // VerticalLineStride
    GFXSHAREDSTATE_TILEMODE_YMAJOR,                     // TileMode
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_16,     // SurfaceHorizontalAlignment
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_16,       // SurfaceVerticalAlignment
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM,        // SurfaceFormat
    0,                                                  // _Unused
    false,                                              // SurfaceArray
    GFXSHAREDSTATE_SURFACETYPE_NULL                     // SurfaceType
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW0Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW0::_Gen9   g_cInitSharedStateSurfaceStateDW0Gen9 =
{
    0,                                                  // CubeFaceEnablesPositiveZ
    0,                                                  // CubeFaceEnablesNegativeZ
    0,                                                  // CubeFaceEnablesPositiveY
    0,                                                  // CubeFaceEnablesNegativeY
    0,                                                  // CubeFaceEnablesPositiveX
    0,                                                  // CubeFaceEnablesNegativeX
    GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_NORMAL,         // MediaBoundaryPixelMode
    GFXSHAREDSTATE_RENDER_CACHE_WRITE_ONLY_ON_MISS,     // RenderCacheReadWriteMode
    0,                                                  // SurfaceArraySpace
    0,                                                  // VerticalLineStrideOffset
    0,                                                  // VerticalLineStride
    GFXSHAREDSTATE_TILEMODE_YMAJOR,                     // TileMode
    GFXSHAREDSTATE_SURFACE_HORIZONTAL_ALIGNMENT_16,     // SurfaceHorizontalAlignment
    GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT_16,       // SurfaceVerticalAlignment
    GFXSHAREDSTATE_SURFACEFORMAT_R8G8B8A8_UNORM,        // SurfaceFormat
    false,                                              // ASTCEnable
    false,                                              // SurfaceArray
    GFXSHAREDSTATE_SURFACETYPE_NULL                     // SurfaceType
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW1Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW1::_Gen8   g_cInitSharedStateSurfaceStateDW1Gen8 =
{
    0,                                                  // SurfaceQPitch
    0,                                                  // _Unused1
    GFXSTATE_SOURCE_AGE_CONTROL_BEST_HIT_CHANCE,        // SurfaceObjectAgeControl
    false,                                              // EncryptedData
    GFXSTATE_TARGET_CACHE_ELLC_ONLY,                    // TargetCache
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // CacheabilityControl
    0                                                   // _Unused2
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW2Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW2::_Gen7   g_cInitSharedStateSurfaceStateDW2Gen7 =
{
    0,                                                  // Width
    0,                                                  // _Unused1
    0,                                                  // Height
    0                                                   // _Unused2
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW2AdvGen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW2::_Gen7_Media g_cInitSharedStateSurfaceStateDW2AdvGen7 =
{
    GFXSHAREDSTATE_TILEWALK_XMAJOR,                     // TileWalk
    0,                                                  // TiledSurface
    0,                                                  // HalfPitchforChroma
    0,                                                  // SurfacePitch
    0,                                                  // _Unused1
    0,                                                  // SurfaceObjectControlState
    0,                                                  // _Unused2
    0,                                                  // InterleaveChroma
    0 // MEDIASTATE_SURFACEFORMAT_YCRCB_NORMAL          // SurfaceFormat
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW3Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW3::_Gen7   g_cInitSharedStateSurfaceStateDW3Gen7 =
{
    0,                                                  // SurfacePitch
    0,                                                  // _Unused
    0                                                   // Depth
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW4Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW4::_Gen7   g_cInitSharedStateSurfaceStateDW4Gen7 =
{
    {
        0,                                                  // MultisamplePositionPaletteIndex
        GFXSHAREDSTATE_NUMSAMPLES_1,                        // NumMultisamples
        GFXSHAREDSTATE_MSFMT_MSS,                           // MultisampledSurfaceStorageFormat
        0,                                                  // RenderTargetViewExtent
        0,                                                  // MinimumArrayElement
        GFXSHAREDSTATE_RROTATE_0DEG,                        // RenderTargetRotation
        0                                                   // _Unused
    }
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW5Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW5::_Gen7   g_cInitSharedStateSurfaceStateDW5Gen7 =
{
    0,                                                  // MIPCountLOD
    0,                                                  // SurfaceMinLOD
    0,                                                  // _Unused1
    GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL_NOT_CACHEABLE,// CacheabilityControlL3
    GFXSHAREDSTATE_LLC_CACHEABILITY_CONTROL_USE_GTT_ENTRY,// CacheabilityControlLLC
    GFXSTATE_GFDT_SOURCE_GTT,                           // SurfaceGraphicsDataType
    false,                                              // SurfaceEncryptedDataEnable
    0,                                                  // YOffset
    0,                                                  // _Unused2
    0                                                   // XOffset
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW5Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW5::_Gen8   g_cInitSharedStateSurfaceStateDW5Gen8 =
{
    0,                                                  // MIPCountLOD
    0,                                                  // SurfaceMinLOD
    0,                                                  // _Unused1
    0,                                                  // YOffset
    0,                                                  // _Unused2
    0                                                   // XOffset
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW5Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW5::_Gen9   g_cInitSharedStateSurfaceStateDW5Gen9 =
{
    0,                                                      // MIPCountLOD
    0,                                                      // SurfaceMinLOD
    0,                                                      // MipTailStartLOD
    0,                                                      // _Unused1
    GFXSHAREDSTATE_NON_COHERENT,                            // GFXSHAREDSTATE_COHERENCY_TYPE
    GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT_64,    // GFXSHAREDSTATE_TILED_RESOURCE_VERTICAL_ALIGNMENT
    GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT_64,  // GFXSHAREDSTATE_TILED_RESOURCE_HORIZONTAL_ALIGNMENT
    false,                                                  // TiledResourceEnable
    0,                                                      // _Unused2
    0,                                                      // YOffset
    0,                                                      // _Unused3
    0                                                       // XOffset
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW6MCSGen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW6::_Gen7::_SurfaceMCS
    g_cInitSharedStateSurfaceStateDW6MCSGen7 =
{
    false,                                              // MCSEnable
    0,                                                  // _Unused
    0,                                                  // MCSSurfacePitch
    0                                                   // MCSBaseAddress
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW6AppendCounterGen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW6::_Gen7::_SurfaceAppendCounter
    g_cInitSharedStateSurfaceStateDW6AppendCounterGen7 =
{
    false,                                              // AppendCounterEnable
    0,                                                  // _Unused
    0                                                   // AppendCounterAddress
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW6SurfacePlanarGen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW6::_Gen9::_SurfacePlanar
    g_cInitSharedStateSurfaceStateDW6SurfacePlanarGen9 =
{
    0,                                                  // YOffset
    0,                                                  // _Unused1
    0,                                                  // XOffset
    0                                                   // _Unused2
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW6SurfaceOtherGen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW6::_Gen9::_SurfaceOther
    g_cInitSharedStateSurfaceStateDW6SurfaceOtherGen9 =
{
    GFXSHAREDSTATE_AUX_NONE,                            // GFXSHAREDSTATE_AUXILIARY_SURFACE_MODE
    false,                                              // RenderTargetCompressionEnable
    0,                                                  // AuxiliarySurfacePitch
    0,                                                  // _Unused1
    0,                                                  // AuxilarySurfaceQPitch
    0                                                   // _Unused2
};


/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW7Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW7::_Gen7   g_cInitSharedStateSurfaceStateDW7Gen7 =
{
    0,                                                  // ResourceMinLOD
    0,                                                  // _Unused
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // AlphaClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // BlueClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // GreenClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                      // RedClearColor
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW7Gen7
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW7::_Gen7_5    g_cInitSharedStateSurfaceStateDW7Gen7_5 =
{
    0,                                                  // ResourceMinLOD
    0,                                                  // _Unused
    GFXSHAREDSTATE_SHADERCHANNELSELECT_ALPHA,           // ShaderChannelSelectA
    GFXSHAREDSTATE_SHADERCHANNELSELECT_BLUE,            // ShaderChannelSelectB
    GFXSHAREDSTATE_SHADERCHANNELSELECT_GREEN,           // ShaderChannelSelectG
    GFXSHAREDSTATE_SHADERCHANNELSELECT_RED,             // ShaderChannelSelectR
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // AlphaClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // BlueClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO,                     // GreenClearColor
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                      // RedClearColor
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW7Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW7::_Gen9    g_cInitSharedStateSurfaceStateDW7Gen9 =
{
    0,                                                  // ResourceMinLOD
    0,                                                  // _Unused1
    GFXSHAREDSTATE_SHADERCHANNELSELECT_ALPHA,           // ShaderChannelSelectA
    GFXSHAREDSTATE_SHADERCHANNELSELECT_BLUE,            // ShaderChannelSelectB
    GFXSHAREDSTATE_SHADERCHANNELSELECT_GREEN,           // ShaderChannelSelectG
    GFXSHAREDSTATE_SHADERCHANNELSELECT_RED,             // ShaderChannelSelectR
    0                                                   // _Unused2
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW8Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW8::_Gen8    g_cInitSharedStateSurfaceStateDW8Gen8 =
{
    0                                                   // SurfaceBaseAddress
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW9Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW9::_Gen8    g_cInitSharedStateSurfaceStateDW9Gen8 =
{
    0,                                                  // Surface64bitBaseAddress
    0                                                   // _Unused
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW10Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW10::_Gen8    g_cInitSharedStateSurfaceStateDW10Gen8 =
{
    0                                                   // AuxiliarySurfaceBaseAddress
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW10Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW10::_Gen9::_SurfaceOther    g_cInitSharedStateSurfaceStateDW10SurfaceOtherGen9 =
{
    0,                                                  // QuiltWidth
    0,                                                  // QuiltHeight
    0,                                                  // _Unused
    0                                                   // AuxiliarySurfaceBaseAddress
};
static const SSharedStateSurfaceState::_DW10::_Gen9::_SurfacePlanar    g_cInitSharedStateSurfaceStateDW10SurfacePlanarGen9 =
{
    0,                                                  // QuiltWidth
    0,                                                  // QuiltHeight
    0                                                  // _Unused
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW11Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW11::_Gen8    g_cInitSharedStateSurfaceStateDW11Gen8 =
{
    0,                                                  // Auxiliary64bitBaseAddress
    0                                                   // _Unused
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW12Gen8
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW12::_Gen8    g_cInitSharedStateSurfaceStateDW12Gen8 =
{
    0                                                   // HierarchicalDepthClearValue
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW12Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW12::_Gen9    g_cInitSharedStateSurfaceStateDW12Gen9 =
{
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                     // RedClearColor
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW13Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW13::_Gen9    g_cInitSharedStateSurfaceStateDW13Gen9 =
{
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                     // GreenClearColor
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW14Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW14::_Gen9    g_cInitSharedStateSurfaceStateDW14Gen9 =
{
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                     // BlueClearColor;
};

/*****************************************************************************\
CONST: g_cInitSharedStateSurfaceStateDW15Gen9
\*****************************************************************************/
static const SSharedStateSurfaceState::_DW15::_Gen9    g_cInitSharedStateSurfaceStateDW15Gen9 =
{
    GFXSHAREDSTATE_CLEARCOLOR_ZERO                     // AlphaClearColor
};

/*****************************************************************************\
CONST: g_cInitSSurfaceStateBufferLength
\*****************************************************************************/
static const SSurfaceStateBufferLength g_cInitSurfaceStateBufferLength =
{
    //DWORD 0
    {
        0,                                              // Width
        0,                                              // Height
        0,                                              // Depth
        0                                               // _Unused
    }
};

} // namespace G6HWC
