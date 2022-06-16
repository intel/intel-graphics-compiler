/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IGC/common/igc_debug.h"
#include "../sp/sp_types.h"

#include "../sp/sp_debug.h"
#include "../inc/common/igfxfmid.h"

#include "cmd_shared_def_g8.h"
#include "cmd_media_def_g8.h"
#include "cmd_media_caps_g8.h"
#include "cmd_shared_enum_g8.h"
#include "cmd_media_init_g8.h"

#include "IGC/common/igc_regkeys.hpp"
#include "Probe/Assertion.h"

namespace G6HWC
{

    template <class Type>
__forceinline float FixedToFloat(
      Type fixed,
      const int whole,
      const int fractional )
{
    IGC_ASSERT( fractional + whole <= 32 );

    float value = (float)fixed / (float)( 1 << fractional );

    return value;
}

/*****************************************************************************\
Helper functions
\*****************************************************************************/
inline bool IsPlatformValid(
    const PLATFORM productID)
{
    if (productID.eRenderCoreFamily <= IGFX_XE_HPC_CORE)
    {
        return true;
    }

    return false;
}

/*****************************************************************************\
\*****************************************************************************/
void DebugSurfaceStateCommand(
      const void* pLinearAddress,
      const PLATFORM productID,
      std::string &output )
{
#if ( defined( _DEBUG ) || defined( _INTERNAL ) || defined( _RELEASE_INTERNAL )   || defined(ICBE_LINUX) || defined(_LINUX) || defined(LINUX))
    if (IGC_IS_FLAG_ENABLED(DumpPatchTokens)) {
        SSharedStateSurfaceState* p3DStateSurfaceState =
            (SSharedStateSurfaceState*)pLinearAddress;

        if (productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
            productID.eRenderCoreFamily == IGFX_GEN7_5_CORE)
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "SURFACE_STATE = { %08x, %08x, %08x, %08x, %08x, %08x, %08X, %08X }\n",
                p3DStateSurfaceState->DW0.Value,
                p3DStateSurfaceState->DW1.Value,
                p3DStateSurfaceState->DW2.Value,
                p3DStateSurfaceState->DW3.Value,
                p3DStateSurfaceState->DW4.Value,
                p3DStateSurfaceState->DW5.Value,
                p3DStateSurfaceState->DW6.Value,
                p3DStateSurfaceState->DW7.Value);
        }
        else if (productID.eRenderCoreFamily == IGFX_GEN8_CORE)
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "SURFACE_STATE = { %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x }\n",
                p3DStateSurfaceState->DW0.Value,
                p3DStateSurfaceState->DW1.Value,
                p3DStateSurfaceState->DW2.Value,
                p3DStateSurfaceState->DW3.Value,
                p3DStateSurfaceState->DW4.Value,
                p3DStateSurfaceState->DW5.Value,
                p3DStateSurfaceState->DW6.Value,
                p3DStateSurfaceState->DW7.Value,
                p3DStateSurfaceState->DW8.Value,
                p3DStateSurfaceState->DW9.Value,
                p3DStateSurfaceState->DW10.Value,
                p3DStateSurfaceState->DW11.Value,
                p3DStateSurfaceState->DW12.Value);
        }
        else if (productID.eRenderCoreFamily >= IGFX_GEN9_CORE)
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "SURFACE_STATE = { %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x}\n",
                p3DStateSurfaceState->DW0.Value,
                p3DStateSurfaceState->DW1.Value,
                p3DStateSurfaceState->DW2.Value,
                p3DStateSurfaceState->DW3.Value,
                p3DStateSurfaceState->DW4.Value,
                p3DStateSurfaceState->DW5.Value,
                p3DStateSurfaceState->DW6.Value,
                p3DStateSurfaceState->DW7.Value,
                p3DStateSurfaceState->DW8.Value,
                p3DStateSurfaceState->DW9.Value,
                p3DStateSurfaceState->DW10.Value,
                p3DStateSurfaceState->DW11.Value,
                p3DStateSurfaceState->DW12.Value,
                p3DStateSurfaceState->DW13.Value,
                p3DStateSurfaceState->DW14.Value,
                p3DStateSurfaceState->DW15.Value);
        }
        else
        {
            // Unsupported platform
            IGC_ASSERT(0);
        }

        // DWORD 0
        if (productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
            productID.eRenderCoreFamily == IGFX_GEN7_5_CORE)
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceType                     : %x\n",
                p3DStateSurfaceState->DW0.Gen7.SurfaceType);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceArray                    : %x\n",
                p3DStateSurfaceState->DW0.Gen7.SurfaceArray);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceFormat                   : %x\n",
                p3DStateSurfaceState->DW0.Gen7.SurfaceFormat);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVerticalLineStride              : %u\n",
                p3DStateSurfaceState->DW0.Gen7.VerticalLineStride);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVerticalLineStrideOffset        : %u\n",
                p3DStateSurfaceState->DW0.Gen7.VerticalLineStrideOffset);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRenderCacheReadWriteMode        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.RenderCacheReadWriteMode);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMediaBoundaryPixelMode          : %x\n",
                p3DStateSurfaceState->DW0.Gen7.MediaBoundaryPixelMode);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeX        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesNegativeX);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveX        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesPositiveX);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeY        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesNegativeY);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveY        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesPositiveY);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeZ        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesNegativeZ);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveZ        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.CubeFaceEnablesPositiveZ);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceArraySpacing             : %u\n",
                p3DStateSurfaceState->DW0.Gen7.SurfaceArraySpacing);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTiledSurface                    : %x\n",
                p3DStateSurfaceState->DW0.Gen7.TiledSurface);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTileWalk                        : %x\n",
                p3DStateSurfaceState->DW0.Gen7.TileWalk);
        }
        else if ((productID.eRenderCoreFamily >= IGFX_GEN8_CORE) &&
            IsPlatformValid(productID))
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceType                     : %x\n",
                p3DStateSurfaceState->DW0.Gen8.SurfaceType);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceArray                    : %x\n",
                p3DStateSurfaceState->DW0.Gen8.SurfaceArray);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceFormat                   : %x\n",
                p3DStateSurfaceState->DW0.Gen8.SurfaceFormat);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceVerticalAlignment        : %u\n",
                p3DStateSurfaceState->DW0.Gen8.SurfaceVerticalAlignment);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceHorizontalAlignment      : %u\n",
                p3DStateSurfaceState->DW0.Gen8.SurfaceHorizontalAlignment);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTileMode                        : %u\n",
                p3DStateSurfaceState->DW0.Gen8.TileMode);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVerticalLineStride              : %u\n",
                p3DStateSurfaceState->DW0.Gen8.VerticalLineStride);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVerticalLineStrideOffset        : %u\n",
                p3DStateSurfaceState->DW0.Gen8.VerticalLineStrideOffset);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRenderCacheReadWriteMode        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.RenderCacheReadWriteMode);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMediaBoundaryPixelMode          : %x\n",
                p3DStateSurfaceState->DW0.Gen8.MediaBoundaryPixelMode);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeX        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesNegativeX);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveX        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesPositiveX);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeY        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesNegativeY);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveY        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesPositiveY);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesNegativeZ        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesNegativeZ);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeFaceEnablesPositiveZ        : %x\n",
                p3DStateSurfaceState->DW0.Gen8.CubeFaceEnablesPositiveZ);

            if (productID.eRenderCoreFamily >= IGFX_GEN9_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tASTCEnable                      : %x\n",
                    p3DStateSurfaceState->DW0.Gen9.ASTCEnable);
            }
        }
        else
        {
            // Unsupported platform
            IGC_ASSERT(0);
        }

        // DWORD 1
        if (productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
            productID.eRenderCoreFamily == IGFX_GEN7_5_CORE)
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceBaseAddress              : %x\n",
                p3DStateSurfaceState->DW1.All.SurfaceBaseAddress);
        }
        else if ((productID.eRenderCoreFamily >= IGFX_GEN8_CORE) &&
            IsPlatformValid(productID))
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceQPitch                   : %x\n",
                p3DStateSurfaceState->DW1.Gen8.SurfaceQPitch);
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceObjectAgeControl         : %x\n",
                p3DStateSurfaceState->DW1.Gen8.SurfaceObjectAgeControl);
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceObjectEncryptedDataEnable: %x\n",
                p3DStateSurfaceState->DW1.Gen8.SurfaceObjectEncryptedDataEnable);
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceObjectTargetCache        : %x\n",
                p3DStateSurfaceState->DW1.Gen8.SurfaceObjectTargetCache);
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceObjectCacheabilityControl: %x\n",
                p3DStateSurfaceState->DW1.Gen8.SurfaceObjectCacheabilityControl);
        }
        else
        {
            // Unsupported platform
            IGC_ASSERT(0);
        }

        if (productID.eRenderCoreFamily >= IGFX_GEN7_CORE   &&
            IsPlatformValid(productID))
        {
            // DWORD 2
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tHeight                          : %u\n",
                p3DStateSurfaceState->DW2.Gen7.Height);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tWidth                           : %u\n",
                p3DStateSurfaceState->DW2.Gen7.Width);

            // DWORD 3
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tDepth                           : %u\n",
                p3DStateSurfaceState->DW3.Gen7.Depth);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfacePitch                    : %u\n",
                p3DStateSurfaceState->DW3.Gen7.SurfacePitch);

            // DWORD 4
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMinimumArrayElement             : %u\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.MinimumArrayElement);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfacePitch                    : %x\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.MultisampledSurfaceStorageFormat);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMultisamplePositionPaletteIndex : %u\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.MultisamplePositionPaletteIndex);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNumMultisamples                 : %u\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.NumMultisamples);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRenderTargetRotation            : %x\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.RenderTargetRotation);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRenderTargetViewExtent          : %x\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceAll.RenderTargetViewExtent);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceStrBufMinimumArrayElement: %u\n",
                p3DStateSurfaceState->DW4.Gen7.SurfaceStrBuf.MinimumArrayElement);

            // DWORD 5
            if (productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
                productID.eRenderCoreFamily == IGFX_GEN7_5_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMipCountLOD                     : %u\n",
                    p3DStateSurfaceState->DW5.Gen7.MipCountLOD);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceMinLOD                   : %u\n",
                    p3DStateSurfaceState->DW5.Gen7.SurfaceMinLOD);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tXOffset                         : %u\n",
                    p3DStateSurfaceState->DW5.Gen7.XOffset);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tYOffset                         : %u\n",
                    p3DStateSurfaceState->DW5.Gen7.YOffset);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceEncryptedDataEnable      : %x\n",
                    p3DStateSurfaceState->DW5.Gen7.SurfaceEncryptedDataEnable);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceGraphicsDataType         : %u\n",
                    p3DStateSurfaceState->DW5.Gen7.SurfaceGraphicsDataType);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCacheabilityControlL3           : %x\n",
                    p3DStateSurfaceState->DW5.Gen7.CacheabilityControlL3);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCacheabilityControlLLC          : %x\n",
                    p3DStateSurfaceState->DW5.Gen7.CacheabilityControlLLC);
            }
            else if (productID.eRenderCoreFamily >= IGFX_GEN8_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMipCountLOD                     : %u\n",
                    p3DStateSurfaceState->DW5.Gen8.MipCountLOD);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceMinLOD                   : %u\n",
                    p3DStateSurfaceState->DW5.Gen8.SurfaceMinLOD);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tYOffset                         : %u\n",
                    p3DStateSurfaceState->DW5.Gen8.YOffset);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tXOffset                         : %u\n",
                    p3DStateSurfaceState->DW5.Gen8.XOffset);

                if (productID.eRenderCoreFamily >= IGFX_GEN9_CORE)
                {
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTiledResourceEnable             : %u\n",
                        p3DStateSurfaceState->DW5.Gen9.TiledResourceEnable);

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTiledResourceHorizontalAlignment: %u\n",
                        p3DStateSurfaceState->DW5.Gen9.TiledResourceHorizontalAlignment);

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTiledResourceVerticalAlignment  : %u\n",
                        p3DStateSurfaceState->DW5.Gen9.TiledResourceVerticalAlignment);

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMipTailStartLOD                 : %u\n",
                        p3DStateSurfaceState->DW5.Gen9.MipTailStartLOD);

                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCoherencyType                   : %u\n",
                        p3DStateSurfaceState->DW5.Gen9.CoherencyType);
                }
            }

            // DWORD6
            if (productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
                productID.eRenderCoreFamily == IGFX_GEN7_5_CORE ||
                productID.eRenderCoreFamily == IGFX_GEN8_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMCSEnable                       : %x\n",
                    p3DStateSurfaceState->DW6.Gen7.SurfaceMCS.MCSEnable);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMCSBaseAddress                  : %u\n",
                    p3DStateSurfaceState->DW6.Gen7.SurfaceMCS.MCSBaseAddress);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMCSSurfacePitch                 : %u\n",
                    p3DStateSurfaceState->DW6.Gen7.SurfaceMCS.MCSSurfacePitch);
            }
            else if (productID.eRenderCoreFamily >= IGFX_GEN9_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAuxiliarySurfaceMode            : %x\n",
                    p3DStateSurfaceState->DW6.Gen9.SurfaceOther.AuxiliarySurfaceMode);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRenderTargetCompressionEnable   : %x\n",
                    p3DStateSurfaceState->DW6.Gen9.SurfaceOther.RenderTargetCompressionEnable);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAuxiliarySurfacePitch           : %x\n",
                    p3DStateSurfaceState->DW6.Gen9.SurfaceOther.AuxiliarySurfacePitch);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAuxilarySurfaceQPitch           : %x\n",
                    p3DStateSurfaceState->DW6.Gen9.SurfaceOther.AuxilarySurfaceQPitch);
            }

            // DWORD7
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tResourceMinLOD                  : %u\n",
                p3DStateSurfaceState->DW7.Gen7.ResourceMinLOD);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tClearColorRed                   : %x\n",
                p3DStateSurfaceState->DW7.Gen7.ClearColorRed);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tClearColorGreen                 : %x\n",
                p3DStateSurfaceState->DW7.Gen7.ClearColorGreen);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tClearColorBlue                  : %x\n",
                p3DStateSurfaceState->DW7.Gen7.ClearColorBlue);

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tClearColorAlpha                 : %x\n",
                p3DStateSurfaceState->DW7.Gen7.ClearColorAlpha);

            if (productID.eRenderCoreFamily >= IGFX_GEN7_5_CORE)
            {
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShaderChannelSelectAlpha        : %x\n",
                    p3DStateSurfaceState->DW7.Gen7_5.ShaderChannelSelectAlpha);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShaderChannelSelectBlue         : %x\n",
                    p3DStateSurfaceState->DW7.Gen7_5.ShaderChannelSelectBlue);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShaderChannelSelectGreen        : %x\n",
                    p3DStateSurfaceState->DW7.Gen7_5.ShaderChannelSelectGreen);

                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShaderChannelSelectRed          : %x\n",
                    p3DStateSurfaceState->DW7.Gen7_5.ShaderChannelSelectRed);
            }

            if (productID.eRenderCoreFamily >= IGFX_GEN8_CORE)
            {
                // DWORD 8
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurfaceBaseAddress              : %x\n",
                    p3DStateSurfaceState->DW8.Gen8.SurfaceBaseAddress);

                // DWORD 9
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSurface64bitBaseAddress         : %x\n",
                    p3DStateSurfaceState->DW9.Gen8.Surface64bitBaseAddress);

                // DWORD 10
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAuxiliarySurfaceBaseAddress     : %x\n",
                    p3DStateSurfaceState->DW10.Gen8.AuxiliarySurfaceBaseAddress);

                // DWORD 11
                ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAuxiliary64bitBaseAddress       : %x\n",
                    p3DStateSurfaceState->DW11.Gen8.Auxiliary64bitBaseAddress);

                if (productID.eRenderCoreFamily == IGFX_GEN8_CORE)
                {
                    // DWORD 12
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tHierarchicalDepthClearValue     : %x\n",
                        p3DStateSurfaceState->DW12.Gen8.HierarchicalDepthClearValue);
                }
                else
                {
                    // DWORD 12
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRedClearColor                   : %x\n",
                        p3DStateSurfaceState->DW12.Gen9.RedClearColor);

                    // DWORD 13
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tGreenClearColor                 : %x\n",
                        p3DStateSurfaceState->DW13.Gen9.GreenClearColor);

                    // DWORD 14
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBlueClearColor                  : %x\n",
                        p3DStateSurfaceState->DW14.Gen9.BlueClearColor);

                    // DWORD 15
                    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAlphaClearColor                 : %x\n",
                        p3DStateSurfaceState->DW15.Gen9.AlphaClearColor);
                }
            }
        }
        else
        {
            // Unsupported platform
            IGC_ASSERT(0);
        }
    }

#endif
}

/*****************************************************************************\
\*****************************************************************************/
void DebugInterfaceDescriptorDataCommand(
    const void* pLinearAddress,
    const PLATFORM productID,
    std::string &output )
{
#if ( defined( _DEBUG ) || defined( _INTERNAL ) || defined( _RELEASE_INTERNAL )   || defined(ICBE_LINUX) || defined(_LINUX) || defined(LINUX))
    SMediaStateInterfaceDescriptorData* pInterfaceDescriptorData =
        (SMediaStateInterfaceDescriptorData*)pLinearAddress;

    if( productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
        productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "INTERFACE_DESCRIPTOR_DATA = { %08x, %08x, %08x, %08x, %08x, %08x }\n",
            pInterfaceDescriptorData->DW0.Value,
            pInterfaceDescriptorData->DW1.Value,
            pInterfaceDescriptorData->DW2.Value,
            pInterfaceDescriptorData->DW3.Value,
            pInterfaceDescriptorData->DW4.Value,
            pInterfaceDescriptorData->DW5.Value );
    }
    else if( productID.eRenderCoreFamily >= IGFX_GEN8_CORE &&
             IsPlatformValid( productID ) )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "INTERFACE_DESCRIPTOR_DATA = { %08x, %08x, %08x, %08x, %08x, %08x, %08x, %08x }\n",
            pInterfaceDescriptorData->DW0.Value,
            pInterfaceDescriptorData->DW1.Value,
            pInterfaceDescriptorData->DW2.Value,
            pInterfaceDescriptorData->DW3.Value,
            pInterfaceDescriptorData->DW4.Value,
            pInterfaceDescriptorData->DW5.Value,
            pInterfaceDescriptorData->DW6.Value,
            pInterfaceDescriptorData->DW7.Value );
    }
    else
    {
        // Unsupported platform
        IGC_ASSERT(0);
    }

    // DWORD 0
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tKernelStartPointer =               : %u\n",
        pInterfaceDescriptorData->DW0.All.KernelStartPointer);

    if( productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
        productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
    {
        // DWORD 1
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSoftwareExceptionEnable =          : %u\n",
            pInterfaceDescriptorData->DW1.All.SoftwareExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMaskStackExceptionEnable =         : %u\n",
            pInterfaceDescriptorData->DW1.All.MaskStackExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tIllegalOpcodeExceptionEnable =     : %u\n",
            pInterfaceDescriptorData->DW1.All.IllegalOpcodeExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFloatingPointMode =                : %x\n",
            pInterfaceDescriptorData->DW1.All.FloatingPointMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tThreadPriority =                   : %x\n",
            pInterfaceDescriptorData->DW1.All.ThreadPriority );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSingleProgramFlow =                : %x\n",
            pInterfaceDescriptorData->DW1.All.SingleProgramFlow );

        // DWORD 2
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSamplerCount =                     : %u\n",
            pInterfaceDescriptorData->DW2.All.SamplerCount );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSamplerStatePointer =              : %x\n",
            pInterfaceDescriptorData->DW2.All.SamplerStatePointer );

        // DWORD 3
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBindingTableEntryCount =           : %u\n",
            pInterfaceDescriptorData->DW3.All.BindingTableEntryCount );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBindingTablePointer =              : %x\n",
            pInterfaceDescriptorData->DW3.All.BindingTablePointer );

        // DWORD 4
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tConstantURBEntryReadOffset =       : %u\n",
            pInterfaceDescriptorData->DW4.All.ConstantURBEntryReadOffset );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tConstantURBEntryReadLength =       : %u\n",
            pInterfaceDescriptorData->DW4.All.ConstantURBEntryReadLength );

        // DWORD 5
        if ( productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
            productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNumberOfThreadsInThreadGroup =     : %u\n",
                pInterfaceDescriptorData->DW5.Gen7.NumberOfThreadsInThreadGroup );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSharedLocalMemorySize =            : %u\n",
                pInterfaceDescriptorData->DW5.Gen7.SharedLocalMemorySize );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBarrierEnable =                    : %x\n",
                pInterfaceDescriptorData->DW5.Gen7.BarrierEnable );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRoundingMode =                     : %x\n",
                pInterfaceDescriptorData->DW5.Gen7.RoundingMode );
        }

        // DWORD 6
        if( productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCrossThreadConstantDataReadLength  : %u\n",
                pInterfaceDescriptorData->DW6.Gen7_5.CrossThreadConstantDataReadLength );
        }

    }
    else if( productID.eRenderCoreFamily >= IGFX_GEN8_CORE &&
             IsPlatformValid( productID ) )
    {
        // DWORD 1
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tKernel64bitStartPointer =          : %u\n",
            pInterfaceDescriptorData->DW1.Gen8.Kernel64bitStartPointer);

        // DWORD 2
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSoftwareExceptionEnable =          : %u\n",
            pInterfaceDescriptorData->DW2.Gen8.SoftwareExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMaskStackExceptionEnable =         : %u\n",
            pInterfaceDescriptorData->DW2.Gen8.MaskStackExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tIllegalOpcodeExceptionEnable =     : %u\n",
            pInterfaceDescriptorData->DW2.Gen8.IllegalOpcodeExceptionEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFloatingPointMode =                : %x\n",
            pInterfaceDescriptorData->DW2.Gen8.FloatingPointMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tThreadPriority =                   : %x\n",
            pInterfaceDescriptorData->DW2.Gen8.ThreadPriority );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSingleProgramFlow =                : %x\n",
            pInterfaceDescriptorData->DW2.Gen8.SingleProgramFlow );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tDenormMode =                       : %u\n",
            pInterfaceDescriptorData->DW2.Gen8.DenormMode );

        // DWORD 3
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSamplerCount =                     : %u\n",
            pInterfaceDescriptorData->DW3.Gen8.SamplerCount );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSamplerStatePointer =              : %u\n",
            pInterfaceDescriptorData->DW3.Gen8.SamplerStatePointer );

        // DWORD 4
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBindingTableEntryCount =           : %u\n",
            pInterfaceDescriptorData->DW4.Gen8.BindingTableEntryCount );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBindingTablePointer =              : %u\n",
            pInterfaceDescriptorData->DW4.Gen8.BindingTablePointer );

        // DWORD 5
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tConstantURBEntryReadOffset =       : %u\n",
            pInterfaceDescriptorData->DW5.Gen8.ConstantURBEntryReadOffset );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tConstantURBEntryReadLength =       : %u\n",
            pInterfaceDescriptorData->DW5.Gen8.ConstantURBEntryReadLength );

        // DWORD 6
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNumberOfThreadsInThreadGroup =     : %u\n",
            pInterfaceDescriptorData->DW6.Gen8.NumberOfThreadsInThreadGroup );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tGlobalBarrierEnable =              : %u\n",
            pInterfaceDescriptorData->DW6.Gen8.GlobalBarrierEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSharedLocalMemorySize =            : %u\n",
            pInterfaceDescriptorData->DW6.Gen8.SharedLocalMemorySize );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBarrierEnable =                    : %u\n",
            pInterfaceDescriptorData->DW6.Gen8.BarrierEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRoundingMode =                     : %u\n",
            pInterfaceDescriptorData->DW6.Gen8.RoundingMode );

        // DWORD 7
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCrossThreadConstantDataReadLength  : %u\n",
            pInterfaceDescriptorData->DW7.Gen8.CrossThreadConstantDataReadLength );
    }
    else
    {
        // Unsupported platform
        IGC_ASSERT(0);
    }

#endif
}

/*****************************************************************************\
\*****************************************************************************/
void DebugSamplerIndirectStateCommand(
    const void* pLinearAddress,
    const PLATFORM productID,
    std::string &output )
{
#if ( defined( _DEBUG ) || defined( _INTERNAL ) || defined( _RELEASE_INTERNAL ) )
    SGfxSamplerIndirectState* pSamplerBorderColor =
        (SGfxSamplerIndirectState*)pLinearAddress;

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "SAMPLER_INDIRECT_STATE\n" );
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBorderColorRed      : %f\n",
        pSamplerBorderColor->BorderColorRed);
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBorderColorGreen    : %f\n",
        pSamplerBorderColor->BorderColorGreen);
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBorderColorBlue     : %f\n",
        pSamplerBorderColor->BorderColorBlue);
    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBorderColorAlpha    : %f\n",
        pSamplerBorderColor->BorderColorAlpha);

#endif
}

/*****************************************************************************\
\*****************************************************************************/
void DebugSamplerStateCommand(
    const void* pLinearAddress,
    const PLATFORM productID,
    std::string &output )
{
#if ( defined( _DEBUG ) || defined( _INTERNAL ) || defined( _RELEASE_INTERNAL ) )
    SSharedStateSamplerState* p3DStateSamplerState =
        (SSharedStateSamplerState*)pLinearAddress;

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "SAMPLER_STATE = { %08x, %08x, %08x, %08x }\n",
        p3DStateSamplerState->DW0.Value,
        p3DStateSamplerState->DW1.Value,
        p3DStateSamplerState->DW2.Value,
        p3DStateSamplerState->DW3.Value );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSamplerDisable                          : %x\n",
        p3DStateSamplerState->DW0.All.SamplerDisable );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTextureBorderColorMode                  : %x\n",
        p3DStateSamplerState->DW0.All.TextureBorderColorMode );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tLODPreClampEnable                       : %x\n",
        p3DStateSamplerState->DW0.All.LODPreClampEnable );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMipModeFilter                           : %x\n",
        p3DStateSamplerState->DW0.All.MipModeFilter );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMagModeFilter                           : %x\n",
        p3DStateSamplerState->DW0.All.MagModeFilter );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMinModeFilter                           : %x\n",
        p3DStateSamplerState->DW0.All.MinModeFilter );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTextureLODBias                          : %f\n",
        FixedToFloat<signed>( p3DStateSamplerState->DW0.All.TextureLODBias, 4, 4 ) );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShadowFunction                          : %x\n",
        p3DStateSamplerState->DW0.All.ShadowFunction );

    if ( productID.eRenderCoreFamily >= IGFX_GEN9_CORE &&
         IsPlatformValid( productID ) )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tAnisotropicAlgorithm                    : %x\n",
            p3DStateSamplerState->DW0.Gen9.AnisotropicAlgorithm );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCoarseLODQualityMode                    : %f\n",
            FixedToFloat<unsigned>( p3DStateSamplerState->DW0.Gen9.CoarseLODQualityMode, 5, 5) );
    }
    else
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBaseMipLevel                            : %f\n",
            FixedToFloat<unsigned>( p3DStateSamplerState->DW0.All.BaseMipLevel, 4, 1 ) );
    }

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMinLOD                                  : %f\n",
        FixedToFloat<unsigned>( p3DStateSamplerState->DW1.All.MinLOD, 4, 6 ) );

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMaxLOD                                  : %f\n",
        FixedToFloat<unsigned>( p3DStateSamplerState->DW1.All.MaxLOD, 4, 6 ) );

    if( productID.eRenderCoreFamily >= IGFX_GEN7_CORE &&
        IsPlatformValid( productID ) )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tCubeSurfaceControlMode                  : %x\n",
            p3DStateSamplerState->DW1.Gen7.CubeSurfaceControlMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMaxLOD                                  : %f\n",
            FixedToFloat<unsigned>(p3DStateSamplerState->DW1.Gen7.MaxLOD, 4, 8 ) );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMinLOD                                  : %f\n",
            FixedToFloat<unsigned>(p3DStateSamplerState->DW1.Gen7.MinLOD, 4, 8 ) );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tShadowFunction                          : %x\n",
            p3DStateSamplerState->DW1.Gen7.ShadowFunction );

        if( productID.eRenderCoreFamily >= IGFX_GEN8_CORE )
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyMode                           : %x\n",
                p3DStateSamplerState->DW1.Gen8.ChromaKeyMode );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyIndex                          : %x\n",
                p3DStateSamplerState->DW1.Gen8.ChromaKeyIndex );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyEnable                         : %x\n",
                p3DStateSamplerState->DW1.Gen8.ChromaKeyEnable );
        }
    }
    else
    {
        // Unsupported platform
        IGC_ASSERT(0);
    }

    if( productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
        productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tBorderColorPointer                          : %x\n",
            p3DStateSamplerState->DW2.All.BorderColorPointer );
    }
    else if( productID.eRenderCoreFamily >= IGFX_GEN8_CORE &&
             IsPlatformValid( productID ) )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tLODClampMagnificationMode               : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.LODClampMagnificationMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFlexibleFilterVerticalAlignment         : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.FlexibleFilterVerticalAlignment );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFlexibleFilterHorizontalAlignment       : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.FlexibleFilterHorizontalAlignment );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFlexibleFilterCoefficientSize           : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.FlexibleFilterCoefficientSize );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tFlexibleFilterCoefficientSize           : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.FlexibleFilterMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tIndirectStatePointer                    : %x\n",
            p3DStateSamplerState->DW2.Gen8.All.IndirectStatePointer );

        switch( p3DStateSamplerState->DW2.Gen8.All.FlexibleFilterMode )
        {
        case G6HWC::GFXSHAREDSTATE_FLEXFILTERMODE_SEP:
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSeparableFilterHeight                   : %x\n",
                p3DStateSamplerState->DW2.Gen8.FlexibleFilterSeparable.SeparableFilterHeight );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSeparableFilterWidth                    : %x\n",
                p3DStateSamplerState->DW2.Gen8.FlexibleFilterSeparable.SeparableFilterWidth );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tSeparableFilterCoefficientTableSize     : %x\n",
                p3DStateSamplerState->DW2.Gen8.FlexibleFilterSeparable.SeparableFilterCoefficientTableSize );
            break;
        case G6HWC::GFXSHAREDSTATE_FLEXFILTERMODE_NONSEP:
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNonSeparableFilterFootprintMask         : %x\n",
                p3DStateSamplerState->DW2.Gen8.FlexibleFilterNonSeparable.NonSeparableFilterFootprintMask );
            break;
        default:
            // Unknown flexible filter mode.
            IGC_ASSERT(0);
            break;
        }
    }
    else
    {
        // Unsupported platform
        IGC_ASSERT(0);
    }

    if( productID.eRenderCoreFamily >= IGFX_GEN7_CORE &&
        IsPlatformValid( productID ) )
    {
        if( productID.eRenderCoreFamily == IGFX_GEN7_CORE ||
            productID.eRenderCoreFamily == IGFX_GEN7_5_CORE )
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyEnable                         : %x\n",
                p3DStateSamplerState->DW3.Gen7.ChromaKeyEnable );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyIndex                          : %u\n",
                p3DStateSamplerState->DW3.Gen7.ChromaKeyIndex );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tChromaKeyMode                           : %x\n",
                p3DStateSamplerState->DW3.Gen7.ChromaKeyMode );
        }
        else
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNonSeparableFilterFootprintMask         : %x\n",
                p3DStateSamplerState->DW3.Gen8.NonSeparableFilterFootprintMask );
        }

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tMaximumAnisotropy                       : %x\n",
            p3DStateSamplerState->DW3.Gen7.MaximumAnisotropy );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tUAddressMagFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.UAddressMagFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tUAddressMinFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.UAddressMinFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVAddressMagFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.VAddressMagFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tVAddressMinFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.VAddressMinFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRAddressMagFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.RAddressMagFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tRAddressMinFilterAddressRoundingEnable  : %x\n",
            p3DStateSamplerState->DW3.Gen7.RAddressMinFilterAddressRoundingEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tNonNormalizedCoordinatesEnable          : %x\n",
            p3DStateSamplerState->DW3.Gen7.NonNormalizedCoordinateEnable );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTrilinearFilterQuality                  : %x\n",
            p3DStateSamplerState->DW3.Gen7.TrilinearFilterQuality );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTCXAddressControlMode                   : %x\n",
            p3DStateSamplerState->DW3.Gen7.TCXAddressControlMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTCYAddressControlMode                   : %x\n",
            p3DStateSamplerState->DW3.Gen7.TCYAddressControlMode );

        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tTCZAddressControlMode                   : %x\n",
            p3DStateSamplerState->DW3.Gen7.TCZAddressControlMode );

        if ( productID.eRenderCoreFamily >= IGFX_GEN9_CORE )
        {
            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tReductionTypeEnable                     : %x\n",
                p3DStateSamplerState->DW3.Gen9.ReductionTypeEnable );

            ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tReductionType                           : %x\n",
                p3DStateSamplerState->DW3.Gen9.ReductionType );
        }
    }
    else
    {
        // Unsupported platform
        IGC_ASSERT(0);
    }
#endif
}

void DebugBindingTableStateCommand(
    const void* pLinearAddress,
    const DWORD numBindingTableEntries,
    const PLATFORM productID,
    std::string &output)
{
#if ( defined( _DEBUG ) || defined( _INTERNAL ) || defined( _RELEASE_INTERNAL )   || defined(ICBE_LINUX) || defined(_LINUX) || defined(LINUX))
    SSharedStateBindingTableState* pBindingTableState =
        (SSharedStateBindingTableState*)pLinearAddress;

    ICBE_DPF_STR( output, GFXDBG_HARDWARE, "BINDING_TABLE_STATE\n" );

    for( DWORD i = 0; i < numBindingTableEntries; ++i )
    {
        ICBE_DPF_STR( output, GFXDBG_HARDWARE, "\tEntry[ %3d ]        : %08x (%x)\n",
            i,
            pBindingTableState->DW0.Value,
            pBindingTableState->DW0.All.SurfaceStatePointer );

        pBindingTableState++;
    }
#endif
}

}
