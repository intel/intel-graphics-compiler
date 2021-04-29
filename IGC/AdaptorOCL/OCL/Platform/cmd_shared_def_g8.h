/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_shared_enum_g8.h"

// Set packing alignment to a single byte
#pragma pack(1)

namespace G6HWC
{

/*****************************************************************************\
STRUCT: SSharedStateBindingTableState (BINDING_TABLE_STATE)
\*****************************************************************************/
struct SSharedStateBindingTableState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,4  );
            DWORD       SurfaceStatePointer                 : BITFIELD_RANGE(  5,31 );  // GTT[31:5] S3DStateSurfaceState*
        } All;

        DWORD           Value;
    } DW0;
};

static_assert(SIZE32(SSharedStateBindingTableState) == 1);

/*****************************************************************************\
STRUCT: SGfxSamplerIndirectState (SAMPLER_INDIRECT_STATE)
\*****************************************************************************/
struct SGfxSamplerIndirectState
{
    // DWORD 0
    float   BorderColorRed;

    // DWORD 1
    float   BorderColorGreen;

    // DWORD 2
    float   BorderColorBlue;

    // DWORD 3
    float   BorderColorAlpha;

    // DWORDs 4-11 should all be set to zero:
    DWORD   _Unused1;
    DWORD   _Unused2;
    DWORD   _Unused3;
    DWORD   _Unused4;
    DWORD   _Unused5;
    DWORD   _Unused6;
    DWORD   _Unused7;
    DWORD   _Unused8;
};
static_assert(SIZE32(SGfxSamplerIndirectState) == 12);

/*****************************************************************************\
STRUCT: SSharedStateSamplerState ( SAMPLER_STATE )
\*****************************************************************************/
struct SSharedStateSamplerState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       ShadowFunction                      : BITFIELD_RANGE(  0,2  );  // GFXSHAREDSTATE_PREFILTER_OPERATION
            DWORD       TextureLODBias                      : BITFIELD_RANGE(  3,13 );  // 11-bit signed (S4.6) [-16.0, 16.0)
            DWORD       MinModeFilter                       : BITFIELD_RANGE( 14,16 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MagModeFilter                       : BITFIELD_RANGE( 17,19 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MipModeFilter                       : BITFIELD_RANGE( 20,21 );  // GFXSHAREDSTATE_MIPFILTER
            DWORD       BaseMipLevel                        : BITFIELD_RANGE( 22,26 );  // U4.1 [0,13]
            DWORD       _Unused1                            : BITFIELD_BIT(      27 );
            DWORD       LODPreClampEnable                   : BITFIELD_BIT(      28 );  // bool
            DWORD       TextureBorderColorMode              : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_DEFAULTCOLOR_MODE
            DWORD       _Unused2                            : BITFIELD_BIT(      30 );
            DWORD       SamplerDisable                      : BITFIELD_BIT(      31 );  // bool
        } All;

        struct _Gen7
        {
            DWORD       _Unused1                            : BITFIELD_BIT(      0  );  // Reserved
            DWORD       TextureLODBias                      : BITFIELD_RANGE(  1,13 );  // S4.2 2's comp
            DWORD       MinModeFilter                       : BITFIELD_RANGE( 14,16 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MagModeFilter                       : BITFIELD_RANGE( 17,19 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MipModeFilter                       : BITFIELD_RANGE( 20,21 );  // GFXSHAREDSTATE_MIPFILTER
            DWORD       BaseMipLevel                        : BITFIELD_RANGE( 22,26 );  // U4.1 [0,13]
            DWORD       _Unused2                            : BITFIELD_BIT(      27 );
            DWORD       LODPreClampEnable                   : BITFIELD_BIT(      28 );  // bool
            DWORD       TextureBorderColorMode              : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_DEFAULTCOLOR_MODE
            DWORD       _Unused3                            : BITFIELD_BIT(      30 );
            DWORD       SamplerDisable                      : BITFIELD_BIT(      31 );  // bool
        } Gen7;

        struct _Gen9
        {
            DWORD       AnisotropicAlgorithm                : BITFIELD_BIT(      0  );  //
            DWORD       TextureLODBias                      : BITFIELD_RANGE(  1,13 );  // S4.2 2's comp
            DWORD       MinModeFilter                       : BITFIELD_RANGE( 14,16 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MagModeFilter                       : BITFIELD_RANGE( 17,19 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MipModeFilter                       : BITFIELD_RANGE( 20,21 );  // GFXSHAREDSTATE_MIPFILTER
            DWORD       CoarseLODQualityMode                : BITFIELD_RANGE( 22,26 );  // U5
            DWORD       _Unused1                            : BITFIELD_BIT(      27 );
            DWORD       LODPreClampEnable                   : BITFIELD_BIT(      28 );  // bool
            DWORD       TextureBorderColorMode              : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_DEFAULTCOLOR_MODE
            DWORD       _Unused2                            : BITFIELD_BIT(      30 );
            DWORD       SamplerDisable                      : BITFIELD_BIT(      31 );  // bool
        } Gen9;

        struct _Gen10
        {
            DWORD       AnisotropicAlgorithm                : BITFIELD_BIT(      0  );  //
            DWORD       TextureLODBias                      : BITFIELD_RANGE(  1,13 );  // S4.2 2's comp
            DWORD       MinModeFilter                       : BITFIELD_RANGE( 14,16 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MagModeFilter                       : BITFIELD_RANGE( 17,19 );  // GFXSHAREDSTATE_MAPFILTER
            DWORD       MipModeFilter                       : BITFIELD_RANGE( 20,21 );  // GFXSHAREDSTATE_MIPFILTER
            DWORD       CoarseLODQualityMode                : BITFIELD_RANGE( 22,26 );  // U5
            DWORD       LODPreClampEnable                   : BITFIELD_RANGE( 27,28 );  // bool
            DWORD       TextureBorderColorMode              : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_DEFAULTCOLOR_MODE
            DWORD       CpsLODCompensation                  : BITFIELD_BIT(      30 );  // CPS LOD Compensation Enable
            DWORD       SamplerDisable                      : BITFIELD_BIT(      31 );  // bool
        } Gen10;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       TCZAddressControlMode               : BITFIELD_RANGE(  0,2  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCYAddressControlMode               : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCXAddressControlMode               : BITFIELD_RANGE(  6,8  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       CubeSurfaceControlMode              : BITFIELD_BIT(      9  );  // GFXSHAREDSTATE_CUBESURFACECONTROLMODE
            DWORD       _Unused                             : BITFIELD_RANGE( 10,11 );
            DWORD       MaxLOD                              : BITFIELD_RANGE( 12,21 );  // U4.6 in LOD units [0.0,13.0]
            DWORD       MinLOD                              : BITFIELD_RANGE( 22,31 );  // U4.6 in LOD units [0.0,13.0]
        } All;

        struct _Gen7
        {
            DWORD       CubeSurfaceControlMode              : BITFIELD_BIT(      0  );  // GFXSHAREDSTATE_CUBESURFACECONTROLMODE
            DWORD       ShadowFunction                      : BITFIELD_RANGE(  1,3  );  // GFXSHAREDSTATE_PREFILTER_OPERATION
            DWORD       _Unused                             : BITFIELD_RANGE(  4,7  );  // Reserved
            DWORD       MaxLOD                              : BITFIELD_RANGE(  8,19 );  // U4.8
            DWORD       MinLOD                              : BITFIELD_RANGE( 20,31 );  // U4.8
        } Gen7;

        struct _Gen8
        {
            DWORD       ChromaKeyMode                       : BITFIELD_BIT(       4 );  // U1
            DWORD       ChromaKeyIndex                      : BITFIELD_RANGE(  5,6  );  // U2
            DWORD       ChromaKeyEnable                     : BITFIELD_BIT(       7 );  // bool
        } Gen8;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,4  );
            DWORD       BorderColorPointer                  : BITFIELD_RANGE(  5,31 );  // DynamicStateOffset[31:5]
        } All;

        union _Gen8
        {
            struct _All
            {
                DWORD       LODClampMagnificationMode           : BITFIELD_BIT(       0 );  // U1
                DWORD       FlexibleFilterVerticalAlignment     : BITFIELD_BIT(       1 );  // U1
                DWORD       FlexibleFilterHorizontalAlignment   : BITFIELD_BIT(       2 );  // U1
                DWORD       FlexibleFilterCoefficientSize       : BITFIELD_BIT(       3 );  // U1
                DWORD       FlexibleFilterMode                  : BITFIELD_BIT(       4 );  // GFXSHAREDSTATE_FLEXFILTERMODE
                DWORD       _Unused1                            : BITFIELD_BIT(       5 );  // Reserved
                DWORD       IndirectStatePointer                : BITFIELD_RANGE(  6,23 );  // DynamicStateOffset[23:6]
                DWORD       _Unused2                            : BITFIELD_RANGE( 24,31 );
            } All;

            struct _FlexibleFilterSeparable
            {
                DWORD       _Unused                             : BITFIELD_RANGE(  0,25 );
                DWORD       SeparableFilterHeight               : BITFIELD_RANGE( 26,27 );  // U2
                DWORD       SeparableFilterWidth                : BITFIELD_RANGE( 28,29 );  // U2
                DWORD       SeparableFilterCoefficientTableSize : BITFIELD_RANGE( 30,31 );  // U2
            } FlexibleFilterSeparable;

            struct _FlexibleFilterNonSeparable
            {
                DWORD       _Unused                             : BITFIELD_RANGE(  0,23 );
                DWORD       NonSeparableFilterFootprintMask     : BITFIELD_RANGE( 24,31 );  // Mask
            } FlexibleFilterNonSeparable;

        } Gen8;

        struct _Gen10
        {
            DWORD       LODClampMagnificationMode           : BITFIELD_BIT(       0 );  // U1
            DWORD       _Unused                             : BITFIELD_RANGE(  1,4  );
            DWORD       ForceGather4                        : BITFIELD_BIT(       5 );  //
            DWORD       IndirectStatePointer                : BITFIELD_RANGE(  6,23 );  // DynamicStateOffset[23:6]
            DWORD       _Unused2                            : BITFIELD_RANGE( 24,31 );
        } Gen10;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       NonNormalizedCoordinatesEnable          : BITFIELD_BIT(       1 );  // bool
            DWORD       _Unused1                                : BITFIELD_RANGE(  1,12 );  // Reserved
            DWORD       RAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      13 );  // bool
            DWORD       RAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      14 );  // bool
            DWORD       VAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      15 );  // bool
            DWORD       VAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      16 );  // bool
            DWORD       UAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      17 );  // bool
            DWORD       UAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      18 );  // bool
            DWORD       MaximumAnisotropy                       : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_ANISORATIO
            DWORD       ChromaKeyMode                           : BITFIELD_BIT(      22 );  // GFXSHAREDSTATE_CHROMAKEY_MODE
            DWORD       ChromaKeyIndex                          : BITFIELD_RANGE( 23,24 );  // U2
            DWORD       ChromaKeyEnable                         : BITFIELD_BIT(      25 );  // bool
            DWORD       _Unused2                                : BITFIELD_RANGE( 26,31 );  // Reserved
        } All;

        struct _Gen7
        {
            DWORD       TCZAddressControlMode                   : BITFIELD_RANGE(  0,2  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCYAddressControlMode                   : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCXAddressControlMode                   : BITFIELD_RANGE(  6,8  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       _Unused1                                : BITFIELD_BIT(      9  );  // Reserved
            DWORD       NonNormalizedCoordinateEnable           : BITFIELD_BIT(      10 );  // bool
            DWORD       TrilinearFilterQuality                  : BITFIELD_RANGE( 11,12 );  // GFXSHAREDSTATE_TRILINEAR_QUALITY
            DWORD       RAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      13 );  // bool
            DWORD       RAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      14 );  // bool
            DWORD       VAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      15 );  // bool
            DWORD       VAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      16 );  // bool
            DWORD       UAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      17 );  // bool
            DWORD       UAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      18 );  // bool
            DWORD       MaximumAnisotropy                       : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_ANISORATIO
            DWORD       ChromaKeyMode                           : BITFIELD_BIT(      22 );  // GFXSHAREDSTATE_CHROMAKEY_MODE
            DWORD       ChromaKeyIndex                          : BITFIELD_RANGE( 23,24 );  // U2
            DWORD       ChromaKeyEnable                         : BITFIELD_BIT(      25 );  // bool
            DWORD       _Unused2                                : BITFIELD_RANGE( 26,31 );  // Reserved
        } Gen7;

        struct _Gen8
        {
            DWORD       _Unused                                 : BITFIELD_RANGE( 22,23 );  // Reserved
            DWORD       NonSeparableFilterFootprintMask         : BITFIELD_RANGE( 24,31 );  // Mask

        } Gen8;

        struct _Gen9
        {
            DWORD       TCZAddressControlMode                   : BITFIELD_RANGE(  0,2  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCYAddressControlMode                   : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       TCXAddressControlMode                   : BITFIELD_RANGE(  6,8  );  // GFXSHAREDSTATE_TEXCOORDMODE
            DWORD       ReductionTypeEnable                     : BITFIELD_BIT(      9  );  // bool
            DWORD       NonNormalizedCoordinateEnable           : BITFIELD_BIT(      10 );  // bool
            DWORD       TrilinearFilterQuality                  : BITFIELD_RANGE( 11,12 );  // GFXSHAREDSTATE_TRILINEAR_QUALITY
            DWORD       RAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      13 );  // bool
            DWORD       RAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      14 );  // bool
            DWORD       VAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      15 );  // bool
            DWORD       VAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      16 );  // bool
            DWORD       UAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      17 );  // bool
            DWORD       UAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      18 );  // bool
            DWORD       MaximumAnisotropy                       : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_ANISORATIO
            DWORD       ReductionType                           : BITFIELD_RANGE( 22,23 );  // GFXSHAREDSTATE_REDUCTION_TYPE
            DWORD       NonSeparableFilterFootprintMask         : BITFIELD_RANGE( 24,31 );  // Mask
        } Gen9;

        union _Gen10
        {
            struct _All
            {
                DWORD       TCZAddressControlMode                   : BITFIELD_RANGE(  0,2  );  // GFXSHAREDSTATE_TEXCOORDMODE
                DWORD       TCYAddressControlMode                   : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_TEXCOORDMODE
                DWORD       TCXAddressControlMode                   : BITFIELD_RANGE(  6,8  );  // GFXSHAREDSTATE_TEXCOORDMODE
                DWORD       ReductionTypeEnable                     : BITFIELD_BIT(      9  );  // bool
                DWORD       NonNormalizedCoordinateEnable           : BITFIELD_BIT(      10 );  // bool
                DWORD       TrilinearFilterQuality                  : BITFIELD_RANGE( 11,12 );  // GFXSHAREDSTATE_TRILINEAR_QUALITY
                DWORD       RAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      13 );  // bool
                DWORD       RAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      14 );  // bool
                DWORD       VAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      15 );  // bool
                DWORD       VAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      16 );  // bool
                DWORD       UAddressMinFilterAddressRoundingEnable  : BITFIELD_BIT(      17 );  // bool
                DWORD       UAddressMagFilterAddressRoundingEnable  : BITFIELD_BIT(      18 );  // bool
                DWORD       MaximumAnisotropy                       : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_ANISORATIO
                DWORD       ReductionType                           : BITFIELD_RANGE( 22,23 );  // GFXSHAREDSTATE_REDUCTION_TYPE
                DWORD       NonSeparableFilterFootprintMask         : BITFIELD_RANGE( 24,31 );  // Mask
            } ALL;

            struct _FlexibleFilterMinMagMode
            {
                DWORD       FlexibleFilterDisableClamping           : BITFIELD_BIT(      10 );  //
            }FlexibleFilterMinMagMode;
        } Gen10;

        DWORD   Value;
    } DW3;
};

static_assert(SIZE32(SSharedStateSamplerState) == 4);

/*****************************************************************************\
STRUCT: SSharedStateSearchPathLUTState
\*****************************************************************************/
struct SSharedStateSearchPathLUTState
{
    // DWORD 0
    union _DW0
    {
        struct _Bitfield
        {
            DWORD   SearchPathLocation_X_0  : 4;
            DWORD   SearchPathLocation_Y_0  : 4;
            DWORD   SearchPathLocation_X_1  : 4;
            DWORD   SearchPathLocation_Y_1  : 4;
            DWORD   SearchPathLocation_X_2  : 4;
            DWORD   SearchPathLocation_Y_2  : 4;
            DWORD   SearchPathLocation_X_3  : 4;
            DWORD   SearchPathLocation_Y_3  : 4;
        } BitField;

        struct _Byte
        {
            BYTE    Byte0;
            BYTE    Byte1;
            BYTE    Byte2;
            BYTE    Byte3;
        } Byte;

        DWORD   Value;
    } DW0;
};

static_assert(SIZE32(SSharedStateSearchPathLUTState) == 1);

/*****************************************************************************\
STRUCT: SSharedStateRDLUTSet
\*****************************************************************************/
struct SSharedStateRDLUTSet
{
    // DWORD 0
    union _DW0
    {
        struct _Bitfield
        {
            DWORD   LUT_MbMode_0    : 8;
            DWORD   LUT_MbMode_1    : 8;
            DWORD   LUT_MbMode_2    : 8;
            DWORD   LUT_MbMode_3    : 8;
        } BitField;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _Bitfield
        {
            DWORD   LUT_MbMode_4    : 8;
            DWORD   LUT_MbMode_5    : 8;
            DWORD   LUT_MbMode_6    : 8;
            DWORD   LUT_MbMode_7    : 8;
        } BitField;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _Bitfield
        {
            DWORD   LUT_MV_0        : 8;
            DWORD   LUT_MV_1        : 8;
            DWORD   LUT_MV_2        : 8;
            DWORD   LUT_MV_3        : 8;
        } BitField;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _Bitfield
        {
            DWORD   LUT_MV_4        : 8;
            DWORD   LUT_MV_5        : 8;
            DWORD   LUT_MV_6        : 8;
            DWORD   LUT_MV_7        : 8;
        } BitField;

        DWORD   Value;
    } DW3;
};

static_assert(SIZE32(SSharedStateRDLUTSet) == 4);

/*****************************************************************************\
STRUCT: SSharedStateVmeState ( VME_STATE )
\*****************************************************************************/
struct SSharedStateVmeState
{
    // DWORD 0 - DWORD 13
    SSharedStateSearchPathLUTState  SearchPath[ g_cNumSearchPathStatesGen6 ];

    // DWORD 14
    union _DW14
    {
        struct _Bitfield
        {
            DWORD   LUT_MbMode_8_0  : 8;
            DWORD   LUT_MbMode_9_0  : 8;
            DWORD   LUT_MbMode_8_1  : 8;
            DWORD   LUT_MbMode_9_1  : 8;
        } BitField;

        DWORD   Value;
    } DW14;

    // DWORD 15
    union _DW15
    {
        struct _Bitfield
        {
            DWORD   LUT_MbMode_8_2  : 8;
            DWORD   LUT_MbMode_9_2  : 8;
            DWORD   LUT_MbMode_8_3  : 8;
            DWORD   LUT_MbMode_9_3  : 8;
        } BitField;

        DWORD   Value;
    } DW15;

    // DWORD 16 - DWORD 31
    struct SSharedStateRDLUTSet   RdLutSet[ g_cNumMBModeSetsGen6 ];
};

static_assert(SIZE32(SSharedStateVmeState) == 32);

/*****************************************************************************\
STRUCT: SSamplerStateErodeDilateMinMaxFilter ( SAMPLER_STATE Erode/Dilate/MinMaxFilter )
\*****************************************************************************/
struct SSamplerStateErodeDilateMinMaxFilter
{
    DWORD DW0;
    DWORD DW1;
    DWORD DW2;
    DWORD DW3;
    DWORD DW4;
    DWORD DW5;
    DWORD DW6;
    DWORD DW7;
};

static_assert(SIZE32(SSamplerStateErodeDilateMinMaxFilter) == 8);

/*****************************************************************************\
STRUCT: SSharedStateSurfaceState ( SURFACE_STATE )
\*****************************************************************************/
struct SSharedStateSurfaceState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       CubeFaceEnablesPositiveZ            : BITFIELD_BIT(      0  );  // bool
            DWORD       CubeFaceEnablesNegativeZ            : BITFIELD_BIT(      1  );  // bool
            DWORD       CubeFaceEnablesPositiveY            : BITFIELD_BIT(      2  );  // bool
            DWORD       CubeFaceEnablesNegativeY            : BITFIELD_BIT(      3  );  // bool
            DWORD       CubeFaceEnablesPositiveX            : BITFIELD_BIT(      4  );  // bool
            DWORD       CubeFaceEnablesNegativeX            : BITFIELD_BIT(      5  );  // bool
            DWORD       MediaBoundaryPixelMode              : BITFIELD_RANGE(  6,7  );  // GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
            DWORD       RenderCacheReadWriteMode            : BITFIELD_BIT(      8  );  // GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
            DWORD       CubeMapCornerMode                   : BITFIELD_BIT(      9  );  // GFXSHAREDSTATE_CUBECORNERMODE
            DWORD       MipMapLayoutMode                    : BITFIELD_BIT(      10 );  // GFXSHAREDSTATE_SURFACE_MIPMAPLAYOUT
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      11 );  // U1
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      12 );  // U1
            DWORD       _Unused1                            : BITFIELD_RANGE( 13,17 );
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 18,26 );  // GFXSHAREDSTATE_SURFACEFORMAT
            DWORD       DataReturnFormat                    : BITFIELD_BIT(      27 );  // GFXSHAREDSTATE_SURFACERETURNFORMAT
            DWORD       _Unused2                            : BITFIELD_BIT(      28 );
            DWORD       SurfaceType                         : BITFIELD_RANGE( 29,31 );  // GFXSHAREDSTATE_SURFACETYPE
        } All;

        struct _Gen7
        {
            DWORD       CubeFaceEnablesPositiveZ            : BITFIELD_BIT(      0  );  // bool
            DWORD       CubeFaceEnablesNegativeZ            : BITFIELD_BIT(      1  );  // bool
            DWORD       CubeFaceEnablesPositiveY            : BITFIELD_BIT(      2  );  // bool
            DWORD       CubeFaceEnablesNegativeY            : BITFIELD_BIT(      3  );  // bool
            DWORD       CubeFaceEnablesPositiveX            : BITFIELD_BIT(      4  );  // bool
            DWORD       CubeFaceEnablesNegativeX            : BITFIELD_BIT(      5  );  // bool
            DWORD       MediaBoundaryPixelMode              : BITFIELD_RANGE(  6,7  );  // GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
            DWORD       RenderCacheReadWriteMode            : BITFIELD_BIT(      8  );  // GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
            DWORD       _Unused1                            : BITFIELD_BIT(      9  );  // Reserved
            DWORD       SurfaceArraySpacing                 : BITFIELD_BIT(      10 );  // GFXSHAREDSTATE_SURFACE_ARRAY_SPACING
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      11 );  // U1
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      12 );  // U1
            DWORD       TileWalk                            : BITFIELD_BIT(      13 );  // GFXSHAREDSTATE_TILEWALK
            DWORD       TiledSurface                        : BITFIELD_BIT(      14 );  // bool
            DWORD       _Unused2                            : BITFIELD_RANGE( 15,17 );
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 18,26 );  // GFXSHAREDSTATE_SURFACEFORMAT
            DWORD       _Unused3                            : BITFIELD_BIT(      27 );
            DWORD       SurfaceArray                        : BITFIELD_BIT(      28 );  // bool
            DWORD       SurfaceType                         : BITFIELD_RANGE( 29,31 );  // GFXSHAREDSTATE_SURFACETYPE
        } Gen7;

        struct _Gen7_Media
        {
            DWORD       SurfaceBaseAddress;                     // GTT[31:0]
        } Gen7Media;

        struct _Gen8
        {
            DWORD       CubeFaceEnablesPositiveZ            : BITFIELD_BIT(      0  );  // bool
            DWORD       CubeFaceEnablesNegativeZ            : BITFIELD_BIT(      1  );  // bool
            DWORD       CubeFaceEnablesPositiveY            : BITFIELD_BIT(      2  );  // bool
            DWORD       CubeFaceEnablesNegativeY            : BITFIELD_BIT(      3  );  // bool
            DWORD       CubeFaceEnablesPositiveX            : BITFIELD_BIT(      4  );  // bool
            DWORD       CubeFaceEnablesNegativeX            : BITFIELD_BIT(      5  );  // bool
            DWORD       MediaBoundaryPixelMode              : BITFIELD_RANGE(  6,7  );  // GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
            DWORD       RenderCacheReadWriteMode            : BITFIELD_BIT(      8  );  // GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
            DWORD       SurfaceArraySpacing                 : BITFIELD_BIT(      9  );  // U1
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      10 );  // U1
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      11 );  // U1
            DWORD       TileMode                            : BITFIELD_RANGE( 12,13 );  // GFXSHAREDSTATE_TILEMODE
            DWORD       SurfaceHorizontalAlignment          : BITFIELD_RANGE( 14,15 );  // U2
            DWORD       SurfaceVerticalAlignment            : BITFIELD_RANGE( 16,17 );  // U2
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 18,26 );  // GFXSHAREDSTATE_SURFACEFORMAT
            DWORD       _Unused                             : BITFIELD_BIT(      27 );  // Reserved
            DWORD       SurfaceArray                        : BITFIELD_BIT(      28 );  // bool
            DWORD       SurfaceType                         : BITFIELD_RANGE( 29,31 );  // GFXSHAREDSTATE_SURFACETYPE
        } Gen8;

        struct _Gen8_Media
        {
            DWORD       Reserved                            : BITFIELD_RANGE( 0, 31 );
        } Gen8Media;

        struct _Gen9
        {
            DWORD       CubeFaceEnablesPositiveZ            : BITFIELD_BIT(      0  );  // bool
            DWORD       CubeFaceEnablesNegativeZ            : BITFIELD_BIT(      1  );  // bool
            DWORD       CubeFaceEnablesPositiveY            : BITFIELD_BIT(      2  );  // bool
            DWORD       CubeFaceEnablesNegativeY            : BITFIELD_BIT(      3  );  // bool
            DWORD       CubeFaceEnablesPositiveX            : BITFIELD_BIT(      4  );  // bool
            DWORD       CubeFaceEnablesNegativeX            : BITFIELD_BIT(      5  );  // bool
            DWORD       MediaBoundaryPixelMode              : BITFIELD_RANGE(  6,7  );  // GFXSHAREDSTATE_MEDIA_BOUNDARY_PIXEL_MODE
            DWORD       RenderCacheReadWriteMode            : BITFIELD_BIT(      8  );  // GFXSHAREDSTATE_RENDER_CACHE_READ_WRITE_MODE
            DWORD       SurfaceArraySpacing                 : BITFIELD_BIT(      9  );  // U1
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      10 );  // U1
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      11 );  // U1
            DWORD       TileMode                            : BITFIELD_RANGE( 12,13 );  // GFXSHAREDSTATE_TILEMODE
            DWORD       SurfaceHorizontalAlignment          : BITFIELD_RANGE( 14,15 );  // U2
            DWORD       SurfaceVerticalAlignment            : BITFIELD_RANGE( 16,17 );  // U2
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 18,26 );  // GFXSHAREDSTATE_SURFACEFORMAT
            DWORD       ASTCEnable                          : BITFIELD_BIT(      27 );  // bool
            DWORD       SurfaceArray                        : BITFIELD_BIT(      28 );  // bool
            DWORD       SurfaceType                         : BITFIELD_RANGE( 29,31 );  // GFXSHAREDSTATE_SURFACETYPE
        } Gen9;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       SurfaceBaseAddress;
        } All;

        struct _Gen7_Media
        {
            DWORD       UVPixelOffsetVDirection             : BITFIELD_RANGE(  0, 1 );  // U0.2
            DWORD       PictureStructure                    : BITFIELD_RANGE(  2, 3 );  // ?
            DWORD       Width                               : BITFIELD_RANGE(  4,17 );  // U14
            DWORD       Height                              : BITFIELD_RANGE( 18,31 );  // U14
        } Gen7Media;

        struct _Gen8
        {
            DWORD       SurfaceQPitch                       : BITFIELD_RANGE(  0,14 );  // QPitch[16:2]
            DWORD       _Unused1                            : BITFIELD_RANGE( 15,23 );  // Reserved
            DWORD       SurfaceObjectAgeControl             : BITFIELD_RANGE( 24,25 );  // GFXSTATE_SOURCE_AGE_CONTROL
            DWORD       SurfaceObjectEncryptedDataEnable    : BITFIELD_BIT(      26 );  // bool
            DWORD       SurfaceObjectTargetCache            : BITFIELD_RANGE( 27,28 );  // GFXSTATE_TARGET_CACHE
            DWORD       SurfaceObjectCacheabilityControl    : BITFIELD_RANGE( 29,30 );  // GFXSTATE_CACHEABILITY_CONTROL
            DWORD       _Unused2                            : BITFIELD_BIT(      31 );  // Reserved
        } Gen8;

        struct _Gen9
        {
            DWORD       SurfaceQPitch                       : BITFIELD_RANGE(  0,14 );  // QPitch[16:2]
            DWORD       _Unused1                            : BITFIELD_RANGE( 15,18 );  // Reserved
            DWORD       BaseMipLevel                        : BITFIELD_RANGE( 19,23 );  //
            DWORD       EncryptedDataEnable                 : BITFIELD_BIT(      24 );  // Reserved
            DWORD       MOCSTablesIndex                     : BITFIELD_RANGE( 25,30 );  // define the L3 and system cache memory properties
            DWORD       _Unused2                            : BITFIELD_BIT(      31 );  // Reserved
        } Gen9;

        struct _Gen10
        {
            DWORD       SurfaceQPitch                       : BITFIELD_RANGE(  0,14 );  // QPitch[16:2]
            DWORD       _Unused1                            : BITFIELD_RANGE( 15,18 );  // Reserved
            DWORD       BaseMipLevel                        : BITFIELD_RANGE( 19,23 );  //
            DWORD       EncryptedDataEnable                 : BITFIELD_BIT(      24 );  // Reserved
            DWORD       MOCSTablesIndex                     : BITFIELD_RANGE( 25,30 );  // define the L3 and system cache memory properties
            DWORD       _Unused2                            : BITFIELD_BIT(      31 );  // Reserved
        } Gen10;

        struct _Gen10_Media
        {
            DWORD       UVPixelOffsetVDirection             : BITFIELD_RANGE(  0, 1 );  // U0.2
            DWORD       PictureStructure                    : BITFIELD_RANGE(  2, 3 );  // ?
            DWORD       Width                               : BITFIELD_RANGE(  4,17 );  // U14 //width except if format is structure buffer :reading the Data base Structure buffer (or) Test Vector Structure Buffer (or) Index Table.
            DWORD       Height                              : BITFIELD_RANGE( 18,31 );  // U14 //height except if format is structure buffer :reading the Data base Structure buffer (or) Test Vector Structure Buffer (or) Index Table.
        } Gen10Media;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       RenderTargetRotation                : BITFIELD_RANGE(  0,1  );  // GFXSHAREDSTATE_RENDER_TARGET_ROTATION
            DWORD       MipCount                            : BITFIELD_RANGE(  2,5  );  // U4 in LOD units
            DWORD       Width                               : BITFIELD_RANGE(  6,18 );  // U13
            DWORD       Height                              : BITFIELD_RANGE( 19,31 );  // U13
        } All;

        struct _Gen7
        {
            DWORD       Width                               : BITFIELD_RANGE(  0,13 );  // U14
            DWORD       _Unused1                            : BITFIELD_RANGE( 14,15 );  // U2
            DWORD       Height                              : BITFIELD_RANGE( 16,29 );  // U14
            DWORD       _Unused2                            : BITFIELD_RANGE( 30,31 );
        } Gen7;

        struct _Gen7_Media
        {
            DWORD       TiledSurface                        : BITFIELD_BIT(       0 );  // bool
            DWORD       TileWalk                            : BITFIELD_BIT(       1 );  // GFX3DSTATE_TILEWALK
            DWORD       HalfPitchForChroma                  : BITFIELD_BIT(       2 );  // bool
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  3,20 );  // U18
            DWORD       _Unused1                            : BITFIELD_BIT(      21 );
            DWORD       SurfaceObjectControlState           : BITFIELD_RANGE( 22,25 );  // ?
            DWORD       _Unused2                            : BITFIELD_BIT(      26 );
            DWORD       InterleaveChroma                    : BITFIELD_BIT(      27 );  // bool
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 28,31 );  // MEDIASTATE_SURFACEFORMAT
        } Gen7Media;

        struct _Gen8_Media
        {
            DWORD       TiledSurface                        : BITFIELD_BIT(       0 );  // bool
            DWORD       TileWalk                            : BITFIELD_BIT(       1 );  // GFX3DSTATE_TILEWALK
            DWORD       HalfPitchForChroma                  : BITFIELD_BIT(       2 );  // bool
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  3,20 );  // U18
            DWORD       AddressControl                      : BITFIELD_BIT(      21 );  // ?
            DWORD       _Unused                             : BITFIELD_RANGE( 22,25 );
            DWORD       InterleaveChroma                    : BITFIELD_BIT(      26 );  // bool
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 27,31 );  // MEDIASTATE_SURFACEFORMAT
        } Gen8Media;

        struct _Gen9_Media
        {
            DWORD       TiledSurface                        : BITFIELD_BIT(       0 );  // bool
            DWORD       TileWalk                            : BITFIELD_BIT(       1 );  // GFX3DSTATE_TILEWALK
            DWORD       HalfPitchForChroma                  : BITFIELD_BIT(       2 );  // bool
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  3,20 );  // U18
            DWORD       AddressControl                      : BITFIELD_BIT(      21 );  // ?
            DWORD       MemoryCompressionEnable             : BITFIELD_BIT(      22 );  // ?
            DWORD       MemoryCompressionMode               : BITFIELD_BIT(      23 );  // ?
            DWORD       OffsetVDirection                    : BITFIELD_BIT(      24 );  // Cr(V)/Cb(U) Pixel Offset V Direction MSB
            DWORD       OffsetUDirection                    : BITFIELD_BIT(      25 );  // Cr(V)/Cb(U) Pixel Offset U Direction
            DWORD       InterleaveChroma                    : BITFIELD_BIT(      26 );  // bool
            DWORD       SurfaceFormat                       : BITFIELD_RANGE( 27,31 );  // MEDIASTATE_SURFACEFORMAT
        } Gen9Media;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       TileWalk                            : BITFIELD_BIT(      0  );  // GFXSHAREDSTATE_TILEWALK
            DWORD       TiledSurface                        : BITFIELD_BIT(      1  );  // bool
            DWORD       _Unused1                            : BITFIELD_BIT(      2  );
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  3,19 );  // U17
            DWORD       _Unused2                            : BITFIELD_BIT(      20 );
            DWORD       Depth                               : BITFIELD_RANGE( 21,31 );  // U11
        } All;

        struct _Gen7
        {
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  0,17 );  // U17
            DWORD       _Unused1                            : BITFIELD_RANGE( 18,20 );  // Reserved
            DWORD       Depth                               : BITFIELD_RANGE( 21,31 );  // U11
        } Gen7;

        struct _Gen7_Media
        {
            DWORD       YOffsetForU                         : BITFIELD_RANGE(  0,13 );  // U14
            DWORD       _Unused1                            : BITFIELD_RANGE( 14,15 );
            DWORD       XOffsetForU                         : BITFIELD_RANGE( 16,29 );  // U14
            DWORD       _Unused2                            : BITFIELD_RANGE( 30,31 );
        } Gen7Media;

        struct _Gen10
        {
            DWORD       SurfacePitch                        : BITFIELD_RANGE(  0,17 );  // U17
            DWORD       _Unused1                            : BITFIELD_RANGE( 18,19 );  // Reserved
            DWORD       TileAddressMappingMode              : BITFIELD_BIT( 20 );       // 0: Gen9, 1: Gen10
            DWORD       Depth                               : BITFIELD_RANGE( 21,31 );  // U11
        } Gen10;

        DWORD   Value;
    } DW3;

    // DWORD 4
    union _DW4
    {
        struct _All
        {
            DWORD       MultisamplePositionPaletteIndex     : BITFIELD_RANGE(  0,2  );  // U3
            DWORD       _Unused1                            : BITFIELD_BIT(      3  );
            DWORD       NumMultisamples                     : BITFIELD_RANGE(  4,6  );  // GFXSHAREDSTATE_NUM_MULTISAMPLES
            DWORD       _Unused2                            : BITFIELD_BIT(      7  );
            DWORD       RenderTargetViewExtent              : BITFIELD_RANGE(  8,16 );  // U9
            DWORD       MinimumArrayElement                 : BITFIELD_RANGE( 17,27 );  // U9
            DWORD       SurfaceMinLOD                       : BITFIELD_RANGE( 28,31 );  // U4 in LOD units
        } All;

        union _Gen7
        {
            struct _SurfaceAll
            {
                DWORD   MultisamplePositionPaletteIndex     : BITFIELD_RANGE(  0,2  );  // U3
                DWORD   NumMultisamples                     : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_NUM_MULTISAMPLES
                DWORD   MultisampledSurfaceStorageFormat    : BITFIELD_BIT(      6  );  // GFXSHAREDSTATE_MSFMT
                DWORD   RenderTargetViewExtent              : BITFIELD_RANGE(  7,17 );  // U11
                DWORD   MinimumArrayElement                 : BITFIELD_RANGE( 18,28 );  // U11
                DWORD   RenderTargetRotation                : BITFIELD_RANGE( 29,30 );  // U4 in LOD units
                DWORD   _Unused                             : BITFIELD_BIT(      31 );  // Reserved
            } SurfaceAll;

            struct _SurfaceStrBuf
            {
                DWORD   MinimumArrayElement                 : BITFIELD_RANGE(  0,26 );  // U27
                DWORD   _Unused                             : BITFIELD_RANGE( 27,31 );  // Reserved
            } SurfaceStrBuf;
        } Gen7;

        struct _Gen7_Media
        {
            DWORD       YOffsetforV                         : BITFIELD_RANGE(  0,14 );  // U15
            DWORD       _Unused1                            : BITFIELD_BIT(      15 );
            DWORD       XOffsetforV                         : BITFIELD_RANGE( 16,29 );  // U14
            DWORD       _Unused2                            : BITFIELD_RANGE( 30,31 );
        } Gen7Media;

        union _Gen10
        {
            struct _SurfaceAll
            {
                DWORD   MultisamplePositionPaletteIndex     : BITFIELD_RANGE(  0,2  );  // U3
                DWORD   NumMultisamples                     : BITFIELD_RANGE(  3,5  );  // GFXSHAREDSTATE_NUM_MULTISAMPLES
                DWORD   MultisampledSurfaceStorageFormat    : BITFIELD_BIT(      6  );  // GFXSHAREDSTATE_MSFMT
                DWORD   RenderTargetViewExtent              : BITFIELD_RANGE(  7,17 );  // U11
                DWORD   MinimumArrayElement                 : BITFIELD_RANGE( 18,28 );  // U11
                DWORD   RenderTargetRotation                : BITFIELD_RANGE( 29,30 );  // U4 in LOD units
                DWORD   ForceNonComparisonReductionType     : BITFIELD_BIT(      31 );
            } SurfaceAll;

            struct _SurfaceStrBuf
            {
                DWORD   _Unused                             : BITFIELD_RANGE( 0,31 );  // Reserved
            } SurfaceStrBuf;
        } Gen10;

        DWORD   Value;
    } DW4;

    // DWORD 5
    union _DW5
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,15 );
            DWORD       SurfaceCacheabilityControl          : BITFIELD_RANGE( 16,17 );  // GFXSHAREDSTATE_CACHEABILITY_CONTROL
            DWORD       SurfaceGraphicsDataType             : BITFIELD_BIT(      18 );  // GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE
            DWORD       SurfaceEncryptedDataEnable          : BITFIELD_BIT(      19 );  // bool
            DWORD       YOffset                             : BITFIELD_RANGE( 20,23 );  // U9
            DWORD       SurfaceVerticalAlignment            : BITFIELD_BIT(      24 );  // GFXSHAREDSTATE_SURFACE_VERTICAL_ALIGNMENT
            DWORD       XOffset                             : BITFIELD_RANGE( 25,31 );  // U4 in LOD units
        } All;

        struct _Gen7
        {
            DWORD       MipCountLOD                         : BITFIELD_RANGE(  0,3  );  // U4
            DWORD       SurfaceMinLOD                       : BITFIELD_RANGE(  4,7  );  // U4
            // bit 14 - Coherency Type (Gen7.5+)
            // bit 15 - Stateless Data PortAccess Force Write Thru (Gen7.5+)
            DWORD       _Unused1                            : BITFIELD_RANGE(  8,15 );  // Reserved
            DWORD       CacheabilityControlL3               : BITFIELD_BIT(      16 );  // GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL
            DWORD       CacheabilityControlLLC              : BITFIELD_BIT(      17 );  // GFXSHAREDSTATE_L3_CACHEABILITY_CONTROL
            DWORD       SurfaceGraphicsDataType             : BITFIELD_BIT(      18 );  // GFXSHAREDSTATE_GRAPHICS_DATATYPE_SOURCE
            DWORD       SurfaceEncryptedDataEnable          : BITFIELD_BIT(      19 );  // bool
            DWORD       YOffset                             : BITFIELD_RANGE( 20,23 );  // U9
            DWORD       _Unused2                            : BITFIELD_BIT(      24 );  // Reserved
            DWORD       XOffset                             : BITFIELD_RANGE( 25,31 );  // U4 in LOD units
        } Gen7;

        struct _Gen7_Media
        {
            DWORD      _Unused                              : BITFIELD_RANGE(  0,29 );
            DWORD      VerticalLineStrideOffest             : BITFIELD_BIT(      30 );  // U1, Gen7.5+
            DWORD      VerticalLineStride                   : BITFIELD_BIT(      31 );  // U1, Gen7.5+
        } Gen7Media;

        struct _Gen8
        {
            DWORD       MipCountLOD                         : BITFIELD_RANGE(  0,3  );  // U4
            DWORD       SurfaceMinLOD                       : BITFIELD_RANGE(  4,7  );  // U4
            // bit 14 - Coherency Type (Gen8+)
            DWORD       _Unused1                            : BITFIELD_RANGE(  8,20 );  // Reserved
            DWORD       YOffset                             : BITFIELD_RANGE( 21,23 );  // U8
            DWORD       _Unused2                            : BITFIELD_BIT(      24 );  // Reserved
            DWORD       XOffset                             : BITFIELD_RANGE( 25,31 );  // U4 in LOD units
        } Gen8;

        struct _Gen8_Media
        {
            DWORD       SurfaceObjectControlState           : BITFIELD_RANGE(  0, 6 );  // MEMORY_OBJECT_CONTROL_STATE
            DWORD       _Unused                             : BITFIELD_RANGE(  7,29 );
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      30 );
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      31 );
        } Gen8Media;

        struct _Gen9
        {
            DWORD       MipCountLOD                         : BITFIELD_RANGE(  0,3  );  // U4
            DWORD       SurfaceMinLOD                       : BITFIELD_RANGE(  4,7  );  // U4
            DWORD       MipTailStartLOD                     : BITFIELD_RANGE( 8, 11 );  // MipTailStartLOD
            DWORD       _Unused1                            : BITFIELD_RANGE( 12, 13 ); // reserved
            DWORD       CoherencyType                       : BITFIELD_BIT( 14 );   // Coherency Type (Gen8+)
            DWORD       TiledResourceVerticalAlignment      : BITFIELD_RANGE( 15, 16 ); //
            DWORD       TiledResourceHorizontalAlignment    : BITFIELD_RANGE( 17, 18 ); //
            DWORD       TiledResourceEnable                 : BITFIELD_BIT( 19 );       // bool
            DWORD       _Unused2                            : BITFIELD_BIT( 20 );       // Reserved
            DWORD       YOffset                             : BITFIELD_RANGE( 21,23 );  // U8
            DWORD       _Unused3                            : BITFIELD_BIT(      24 );  // Reserved
            DWORD       XOffset                             : BITFIELD_RANGE( 25,31 );  // U4 in LOD units
        } Gen9;

        struct _Gen9_Media
        {
            DWORD       SurfaceObjectControlState           : BITFIELD_RANGE(  0, 6 );  // MEMORY_OBJECT_CONTROL_STATE
            DWORD       _Unused                             : BITFIELD_RANGE(  7,17 );
            DWORD       TiledResourceMode                   : BITFIELD_RANGE(  18,19 );
            DWORD       _Unused2                            : BITFIELD_RANGE(  20,29 ); // Reserved
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      30 );
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      31 );
        } Gen9Media;

        struct _Gen10_Media
        {
            DWORD       SurfaceObjectControlState           : BITFIELD_RANGE(  0, 6 );  // MEMORY_OBJECT_CONTROL_STATE
            DWORD       _Unused                             : BITFIELD_RANGE(  7,17 );
            DWORD       TiledResourceMode                   : BITFIELD_RANGE(  18,19 );
            DWORD       Depth                               : BITFIELD_RANGE(  20,23 );
            DWORD       _Unused2                            : BITFIELD_RANGE(  24,29 ); // Reserved
            DWORD       VerticalLineStrideOffset            : BITFIELD_BIT(      30 );
            DWORD       VerticalLineStride                  : BITFIELD_BIT(      31 );
        } Gen10Media;


        DWORD   Value;
    } DW5;

    // DWORD 6
    union _DW6
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        union _Gen7
        {
            struct _SurfaceMCS
            {
                DWORD       MCSEnable                       : BITFIELD_BIT(      0  );  // bool
                DWORD       _Unused                         : BITFIELD_RANGE(  1,2  );  // Reserved
                DWORD       MCSSurfacePitch                 : BITFIELD_RANGE(  3,11 );  // U9
                DWORD       MCSBaseAddress                  : BITFIELD_RANGE( 12,31 );  // GraphicsAddress[31:12]
            } SurfaceMCS;

            struct _SurfaceAppendCounter
            {
                DWORD   AppendCounterEnable                 : BITFIELD_BIT(      0  );  // bool
                DWORD   _Unused                             : BITFIELD_RANGE(  2,5  );
                DWORD   AppendCounterAddress                : BITFIELD_RANGE(  6,31 );  // GraphicsAddress[31:12]
            } SurfaceAppendCounter;
        } Gen7;

        struct _Gen8_Media
        {
            DWORD       SurfaceBaseAddress                  : BITFIELD_RANGE( 0, 31 );
        } Gen8Media;

        union _Gen9
        {
            struct _SurfacePlanar
            {
                DWORD   YOffset                             : BITFIELD_RANGE( 0,  13 );  // U14
                DWORD   _Unused1                            : BITFIELD_RANGE( 14, 15 );  // reserved
                DWORD   XOffset                             : BITFIELD_RANGE( 16, 29 );  // U14
                DWORD   _Unused2                            : BITFIELD_RANGE( 30, 31 );  // reserved
            } SurfacePlanar;

            struct _SurfaceOther
            {
                DWORD   AuxiliarySurfaceMode                : BITFIELD_RANGE( 0, 1   );   //
                DWORD   RenderTargetCompressionEnable       : BITFIELD_BIT(   2      );   //
                DWORD   AuxiliarySurfacePitch               : BITFIELD_RANGE( 3,  11 );   // U9
                DWORD   _Unused1                            : BITFIELD_RANGE( 12, 15 );   // reserved
                DWORD   AuxilarySurfaceQPitch               : BITFIELD_RANGE( 16, 30 );   //
                DWORD   _Unused2                            : BITFIELD_BIT(   31     );   // reserved
            } SurfaceOther;
        } Gen9;

        DWORD   Value;
    } DW6;

    // DWORD 7
    union _DW7
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen7
        {
            DWORD       ResourceMinLOD                      : BITFIELD_RANGE(  0,11 );  // 4.8
            DWORD       _Unused                             : BITFIELD_RANGE( 12,27 );  // Reserved
            DWORD       ClearColorAlpha                     : BITFIELD_BIT(      28 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorBlue                      : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorGreen                     : BITFIELD_BIT(      30 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorRed                       : BITFIELD_BIT(      31 );  // GFXSHAREDSTATE_CLEARCOLOR
        } Gen7;

        struct _Gen7_5
        {
            DWORD       ResourceMinLOD                      : BITFIELD_RANGE(  0,11 );  // 4.8
            DWORD       _Unused                             : BITFIELD_RANGE( 12,15 );
            DWORD       ShaderChannelSelectAlpha            : BITFIELD_RANGE( 16,18 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectBlue             : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectGreen            : BITFIELD_RANGE( 22,24 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectRed              : BITFIELD_RANGE( 25,27 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ClearColorAlpha                     : BITFIELD_BIT(      28 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorBlue                      : BITFIELD_BIT(      29 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorGreen                     : BITFIELD_BIT(      30 );  // GFXSHAREDSTATE_CLEARCOLOR
            DWORD       ClearColorRed                       : BITFIELD_BIT(      31 );  // GFXSHAREDSTATE_CLEARCOLOR
        } Gen7_5;

        struct _Gen8_Media
        {
            DWORD       _Unused                             : BITFIELD_RANGE( 16, 31 );
            DWORD       SurfaceBaseAddressHigh              : BITFIELD_RANGE( 0,  15 );
        } Gen8Media;

        struct _Gen9
        {
            DWORD       ResourceMinLOD                      : BITFIELD_RANGE(  0,11 );  // 4.8
            DWORD       _Unused1                            : BITFIELD_RANGE( 12,15 );
            DWORD       ShaderChannelSelectAlpha            : BITFIELD_RANGE( 16,18 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectBlue             : BITFIELD_RANGE( 19,21 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectGreen            : BITFIELD_RANGE( 22,24 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       ShaderChannelSelectRed              : BITFIELD_RANGE( 25,27 );  // GFXSHAREDSTATE_SHADERCHANNELSELECT
            DWORD       _Unused2                            : BITFIELD_RANGE( 28,31 );  // reserved
        } Gen9;

        DWORD   Value;
    } DW7;

    union _DW8
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen8
        {
            DWORD       SurfaceBaseAddress                  : BITFIELD_RANGE(  0,31 );  // GTT[31:0]
        } Gen8;

        DWORD   Value;
    } DW8;

    union _DW9
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen8
        {
            DWORD       Surface64bitBaseAddress             : BITFIELD_RANGE(  0,15 );  // GTT[47:32]
            DWORD       _Unused                             : BITFIELD_RANGE( 16,31 );  // Reserved
        } Gen8;

        DWORD   Value;
    } DW9;

    union _DW10
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen8
        {
            DWORD       AuxiliarySurfaceBaseAddress         : BITFIELD_RANGE(  0,31 );  // GTT[31:0]
        } Gen8;

        union _Gen9
        {
            struct _SurfacePlanar
            {
                DWORD       QuiltWidth                          : BITFIELD_RANGE( 0,  4  );  // SKL U5
                DWORD       QuiltHeight                         : BITFIELD_RANGE( 5,  9  );  // SKL U5
                DWORD       _Unused                             : BITFIELD_RANGE( 10, 31 );
            } SurfacePlanar;

            struct _SurfaceOther
            {
                DWORD       QuiltWidth                          : BITFIELD_RANGE( 0,  4  );  // SKL U5
                DWORD       QuiltHeight                         : BITFIELD_RANGE( 5,  9  );  // SKL U5
                DWORD       _Unused                             : BITFIELD_RANGE( 10, 11 );
                DWORD       AuxiliarySurfaceBaseAddress         : BITFIELD_RANGE( 12, 31 );  // GTT[31:0]
            } SurfaceOther;
        } Gen9;
        DWORD   Value;
    } DW10;

    union _DW11
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen8
        {
            DWORD       Auxiliary64bitBaseAddress           : BITFIELD_RANGE(  0,15 );  // GTT[47:32]
            DWORD       _Unused                             : BITFIELD_RANGE( 16,31 );  // Reserved
        } Gen8;

        union _Gen9
        {
            struct _SurfacePlanar
            {
                DWORD YOffsetVplane                            : BITFIELD_RANGE( 0, 13);
                DWORD _Unused                                : BITFIELD_RANGE( 14,15); // reserved
                DWORD XOffsetVplane                            : BITFIELD_RANGE( 16,29);
                DWORD _Unused2                                : BITFIELD_RANGE( 30,31); // reserved
            } SurfacePlanar;

            struct _SurfaceOther
            {
                DWORD       AuxiliarySurfaceBaseAddress     : BITFIELD_RANGE( 0, 31 );  // GTT[32:63]
            } SurfaceOther;
        } Gen9;
        DWORD   Value;
    } DW11;

    union _DW12
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen8
        {
            DWORD       HierarchicalDepthClearValue         : BITFIELD_RANGE(  0,31 );  // float
        } Gen8;

        struct _Gen9
        {
            DWORD       RedClearColor                       : BITFIELD_RANGE(  0,31 );  // float
        } Gen9;

        union _Gen10
        {
            struct _ClearValueAddressEnable
            {
                DWORD       _Unused                            : BITFIELD_RANGE(  0, 5 );  // Reserved
                DWORD       ClearColorAddress                : BITFIELD_RANGE(  6,31 );  // GraphicsAddress[31:6]
            } ClearValueAddressEnable;

            struct _ClearValueAddressDisble
            {
                DWORD       RedClearColor                       : BITFIELD_RANGE(  0,31 );  // float
            } ClearValueAddressDisble;
        } Gen10;

        DWORD   Value;
    } DW12;

    union _DW13
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen9
        {
            DWORD       GreenClearColor                     : BITFIELD_RANGE(  0,31 );  // float
        } Gen9;

        union _Gen10
        {
            struct _ClearValueAddressEnable
            {
                DWORD       _Unused                            : BITFIELD_RANGE(  0, 15 );  // Reserved
                DWORD       ClearColorAddressHigh            : BITFIELD_RANGE(  16,31 );  // GraphicsAddress
            } ClearValueAddressEnable;

            struct _ClearValueAddressDisble
            {
                DWORD       GreenClearColor                     : BITFIELD_RANGE(  0,31 );  // float
            } ClearValueAddressDisble;
        } Gen10;

        DWORD   Value;
    } DW13;

    union _DW14
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen9
        {
            DWORD       BlueClearColor                      : BITFIELD_RANGE(  0,31 );  // float
        } Gen9;

        union _Gen10
        {
            struct _ClearValueAddressEnable
            {
                DWORD       _Unused                            : BITFIELD_RANGE(  0, 31 );  // Reserved
            } ClearValueAddressEnable;

            struct _ClearValueAddressDisble
            {
                DWORD       BlueClearColor                      : BITFIELD_RANGE(  0,31 );  // float
            } ClearValueAddressDisble;
        } Gen10;

        DWORD   Value;
    } DW14;

    union _DW15
    {
        struct _All
        {
            DWORD       Reserved                            : BITFIELD_RANGE(  0,31 );  // Reserved
        } All;

        struct _Gen9
        {
            DWORD       AlphaClearColor                     : BITFIELD_RANGE(  0,31 );  // float
        } Gen9;

        union _Gen10
        {
            struct _ClearValueAddressEnable
            {
                DWORD       _Unused                            : BITFIELD_RANGE(  0, 31 );  // Reserved
            } ClearValueAddressEnable;

            struct _ClearValueAddressDisble
            {
                DWORD       AlphaClearColor                     : BITFIELD_RANGE(  0,31 );  // float
            } ClearValueAddressDisble;
        } Gen10;

        DWORD   Value;
    } DW15;
};

static_assert(SIZE32(SSharedStateSurfaceState) == 16);

/*****************************************************************************\
UNION: SSurfaceStateBufferLength
\*****************************************************************************/
union SSurfaceStateBufferLength
{
    struct _All
    {
        DWORD Width                                         : BITFIELD_RANGE(  0,6  );  // U7
        DWORD Height                                        : BITFIELD_RANGE(  7,19 );  // U13
        DWORD Depth                                         : BITFIELD_RANGE( 20,26 );  // U7
        DWORD _Unused                                       : BITFIELD_RANGE( 27,31 );
    } All;

    DWORD Length;
};

static_assert(SIZE32(SSurfaceStateBufferLength) == 1);

}  // namespace G6HWC

// Reset packing alignment to project default
#pragma pack()
