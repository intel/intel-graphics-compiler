/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cstring>

#include "cmd_media_caps_g8.h"
#include "common/secure_mem.h"

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
CONST: g_cGen8HwCaps_GT4
\*****************************************************************************/
const SMediaHardwareCapabilities g_cGen8HwCaps_GT4 =
{
    g_cNumSamplersPerProgram,   // NumSamplersPerProgram
    g_cNumSurfacesPerProgram,   // NumSurfacesPerProgram
    {                           // SamplerAnistropyRange
        1,                      // Min
        16,                     // Max
    },
    16384,                      // MaxSurface1DWidth
    0,                          // MaxSurface1DHeight
    2048,                       // MaxSurface1DDepth
    16384,                      // MaxSurface2DWidth
    16384,                      // MaxSurface2DHeight
    2048,                       // MaxSurface2DDepth
    2048,                       // MaxSurface3DWidth
    2048,                       // MaxSurface3DHeight
    2048,                       // MaxSurface3DDepth
    16384,                      // MaxSurfaceCubeWidth
    16384,                      // MaxSurfaceCubeHeight
    0,                          // MaxSurfaceCubeDepth
    14,                         // MaxSurfaceLODs
    67108863,                   // MaxBufferLength
    {                           // SampleLOD
        0.0f,                   // Min
        14.0f                   // Max
    },
    {                           // MipMapLODBias
        -16.0f,                 // Min
        16.0f                   // Max
    },
    {                           // URBEntryReadOffset
        0,                      // Min
        63                      // Max
    },
    {                           // URBEntryReadLength
        1,                      // Min
        63                      // Max
    },
    31,                         // MaxURBPayloadStartRegister
    {                           // URBEntriesSize
        4,                      // Min
        256                     // Max
    },
    32 * sizeof(DWORD),         // URBEntrySize ( 128bytes = 1024bits )
    8 * sizeof(DWORD),          // URBRowSize   ( 32bytes = 256bits => 4 rows/entry )
    256 * sizeof(KILOBYTE),     // URBSize      ( 262144bytes * 1row/32bytes = 8192rows )
    0,                          // URBAllocationGranularitySize ( Used for L3 Allocation Gen7 )
    64 * sizeof(BYTE),          // L2CacheLineSize
    128 * sizeof(BYTE),         // InstructionCachePrefetchSize
    32 * sizeof(BYTE),          // SurfaceStatePointerAlignSize
    32 * sizeof(BYTE),          // BindingTableStatePointerAlignSize
    32 * sizeof(BYTE),          // SamplerStatePointerAlignSize
    64 * sizeof(BYTE),          // KernelPointerAlignSize
     1 * sizeof(KILOBYTE),      // ScratchPointerAlignSize
    64 * sizeof(BYTE),          // DefaultColorPointerAlignSize
    32 * sizeof(BYTE),          // ConstantBufferPointerAlignSize
    64 * sizeof(BYTE),          // InterfaceDescriptorDataAlignSize
    4 * sizeof(KILOBYTE),       // GeneralStateBaseAddressAlignSize
    4 * sizeof(KILOBYTE),       // SurfaceStateBaseAddressAlignSize
    4 * sizeof(KILOBYTE),       // DynamicStateBaseAddressAlignSize
    4 * sizeof(KILOBYTE),       // IndirectObjectBaseAddressAlignSize
    4 * sizeof(KILOBYTE),       // InstructionBaseAddressAlignSize
    16 * sizeof(BYTE),          // SIPPointerAlignSize
    {                           // KernelHardwareCapabilities
        8,                          // NumUserClipPlanes

        7 * 128,                    // NumHardwareGRFRegisters
        128,                        // NumSoftwareGRFRegisters
        //GT has fixed 128 GRFs per thread, and the allocation granularity
        //should also be 128 instead of 16.
        128,                        // NumGRFRegistersPerBlock
        0,                          // NumMRFRegisters; // MRF registers not used since gen7
        2,                          // NumFlagRegisters

        3,                          // URBRowsPerSetupWrite

        71,                         // EUCount;
        {                           // EUCountPerSubSlice
            7,                          // Min
            8                           // Max
        },
        9,                          // SubSliceCount
        0,                          // EUIDCount; // Not used on gen8

        7,                          // EUThreadsPerEU
        0,                          // EUReservedGrfRegister
        4,                          // EUReservedGrfSubRegister
        0,                          // EUGrfMrfRegisterSizeInBytes; // Not used on gen8
        0x01000000,                 // EUIStackUnderflowException
        0x02000000,                 // EUIStackOverflowException
        0x04000000,                 // EULStackUnderflowException
        0x08000000,                 // EULStackOverflowException
        0x10000000,                 // EUIllegalOpcodeException

        0x20000000,                 // EUSoftwareExceptionControl
        14,                         // EUMaxSoftwareStackValue
        15,                         // EUMaxHardwareStackValue

        0x00000000,                 // EUStackSoftwareOverflow;
        0x01000000,                 // EUStackSoftwareUnderflow;
        0x02000000,                 // EUStackSoftwareI;
        0x04000000,                 // EUStackSoftwareL;
        0x08000000,                 // EUStackSoftwareFunctionCall;

        2,                          // EUJumpCountPerInstruction
        {                           // EUInstructionJumpLimit
            -32768,                     // Min
            32767                       // Max
        },
        {                           // MediaScratchSpacePerThread
            sizeof(KILOBYTE),           // Min
            2 * sizeof(MEGABYTE)        // Max
        },
        48 * sizeof(KILOBYTE),      // InstructionCacheSize
        16 * sizeof(KILOBYTE),      // RenderCacheSize
        64 * sizeof(BYTE),          // KernelPointerAlignSize
        252,                        // MaxAssignableBindingTableIndex;
        255,                        // StatelessModelBindingTableIndex;
        true,                       // HasSharedLocalMemory
        254,                        // SharedLocalMemoryBindingTableIndex
        true,                       // HasCoherentIAAccess
        253,                        // CoherentIAAccessBindingTableIndex;
        {                           // RenderUnit
            true,                      // IsBarycentricInterpolationSupported
            true,                       // IsInstructionCompactionSupported
        },
        {                           // DataPort
            3,                          // OwordBlockTypes
            {
                2,                      // OwordBlockCount[0]
                4,                      // OwordBlockCount[1]
                8                       // OwordBlockCount[2]
            },
            2,                          // OwordDualBlockTypes
            {
                1,                      // OwordDualBlockCount[0]
                4                       // OwordDualBlockCount[1]
            }
        },
        false                       // HasDbgReg
    }
};

void InitializeCapsGen8(
    SMediaHardwareCapabilities* pCaps )
{
    memcpy_s(
        pCaps,
        sizeof(SMediaHardwareCapabilities),
        &g_cGen8HwCaps_GT4,
        sizeof(SMediaHardwareCapabilities) );
}

}
