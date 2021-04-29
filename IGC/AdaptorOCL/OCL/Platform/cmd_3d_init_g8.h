/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_3d_enum_g6.h"
#include "cmd_3d_def_g6.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: g_cInit3DPipeControl
\*****************************************************************************/
static const S3DPipeControl g_cInit3DPipeControl =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( S3DPipeControl ) ),          // Length
        0,                                              // _Unused
        GFX3DSUBOP_3DCONTROL,                           // InstructionSubOpcode
        GFX3DOP_PIPECONTROL,                            // InstructionOpcode
        PIPE_3D,                                        // InstructionSubType
        INSTRUCTION_GFX                                 // InstructionType
    },

    // DWORD 1
    {
        false,                                          // DepthCacheFlushInhibitEnable
        false,                                          // StallAtPixelScoreboardEnable
        false,                                          // StateCacheInvalidationEnable
        false,                                          // ConstantCacheInvalidationEnable
        false,                                          // VertexFetchCacheInvalidationEnable
        0,                                              // _Unused1
        false,                                          // ProtectedMemoryApplicationId
        0,                                              // _Unused2
        false,                                          // NotifyEnable
        false,                                          // IndirectStatePointersDisable
        false,                                          // TextureCacheInvalidationEnable
        false,                                          // InstructionCacheInvalidationEnable
        false,                                          // RenderTargetCacheFlushEnable
        false,                                          // DepthStallEnable
        GFX3DCONTROLOP_NOWRITE,                         // PostSyncOperation
        false,                                          // MediaPrtComplete
        false,                                          // SynchronizeGfdtSurfaceEnable
        false,                                          // TlbInvalidateEnable
        false,                                          // GlobalSnapshotCountReset
        false,                                          // CommandStreamStallEnable
        false,                                          // StoreDataIndexEnable
        false,                                          // ProtectedMemoryEnable
        0,                                              // _Unused3
    },

    // DWORD 2
    {
        0,                                              // _Unused
        GFX3DCONTROL_GTTWRITE_GLOBAL,                   // DestinationAddressType
        0                                               // DestinationAddress
    },

    // DWORD 3
    {
        0                                               // ImmediateData
    },

    // DWORD 4
    {
        0                                               // ImmediateData
    },

    // DWORD 5
    {
        0                                               // ImmediateData
    }
};

} // namespace G6HWC
