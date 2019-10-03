/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
