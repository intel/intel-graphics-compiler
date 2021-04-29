/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "cmd_3d_enum_g6.h"

// Set packing alignment to a single byte
#pragma pack(1)

namespace G6HWC
{

/*****************************************************************************\
STRUCT: S3DPipeControl (PIPE_CONTROL)
\*****************************************************************************/
struct S3DPipeControl
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0,  7 );  // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(  8, 15 );
            DWORD       InstructionSubOpcode                : BITFIELD_RANGE( 16, 23 );  // GFX3DCONTROL_SUBOPCODE
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26 );  // GFX3D_OPCODE:PIPE_CONTROL
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28 );  // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31 );  // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       DepthCacheFlushEnable               : BITFIELD_BIT(        0 );  // bool
            DWORD       StallAtPixelScoreboardEnable        : BITFIELD_BIT(        1 );  // bool
            DWORD       StateCacheInvalidationEnable        : BITFIELD_BIT(        2 );  // bool
            DWORD       ConstantCacheInvalidationEnable     : BITFIELD_BIT(        3 );  // bool
            DWORD       VertexFetchCacheInvalidationEnable  : BITFIELD_BIT(        4 );  // bool
            DWORD       _Unused1                            : BITFIELD_BIT(        5 );  // Reserved
            DWORD       ProtectedMemoryApplicationId        : BITFIELD_BIT(        6 );  // bool
            DWORD       _Unused2                            : BITFIELD_BIT(        7 );  // Reserved
            DWORD       NotifyEnable                        : BITFIELD_BIT(        8 );  // bool
            DWORD       IndirectStatePointersDisable        : BITFIELD_BIT(        9 );  // bool
            DWORD       TextureCacheInvalidationEnable      : BITFIELD_BIT(       10 );  // bool
            DWORD       InstructionCacheInvalidationEnable  : BITFIELD_BIT(       11 );  // bool
            DWORD       RenderTargetCacheFlushEnable        : BITFIELD_BIT(       12 );  // bool
            DWORD       DepthStallEnable                    : BITFIELD_BIT(       13 );  // bool
            DWORD       PostSyncOperation                   : BITFIELD_RANGE( 14, 15 );  // GFX3DCONTROL_OPERATION
            DWORD       MediaPrtComplete                    : BITFIELD_BIT(       16 );  // bool
            DWORD       SynchronizeGfdtSurfaceEnable        : BITFIELD_BIT(       17 );  // bool
            DWORD       TlbInvalidateEnable                 : BITFIELD_BIT(       18 );  // bool
            DWORD       GlobalSnapshotCountReset            : BITFIELD_BIT(       19 );  // bool
            DWORD       CommandStreamStallEnable            : BITFIELD_BIT(       20 );  // bool
            DWORD       StoreDataIndexEnable                : BITFIELD_BIT(       21 );  // bool
            DWORD       ProtectedMemoryEnable               : BITFIELD_BIT(       22 );  // bool
            DWORD       _Unused3                            : BITFIELD_RANGE( 23, 31 );
        } All;

        struct _Gen7
        {
            DWORD       DCFlushEnable                       : BITFIELD_BIT(        5 );  // bool
            DWORD       PipeControlFlushEnable              : BITFIELD_BIT(        7 );  // bool
            DWORD       _Unused                             : BITFIELD_BIT(       17 );  // Reserved
            DWORD       LRIPostSyncOperation                : BITFIELD_BIT(       23 );  // U1
            DWORD       DestinationAddressType              : BITFIELD_BIT(       24 );  // U1
        } Gen7;

        struct _Gen7_5
        {
            DWORD       CoreModeEnable                      : BITFIELD_BIT(       25 );  // bool
        } Gen7_5;

        struct _Gen8
        {
            DWORD       ProtectedModeDisable                : BITFIELD_BIT(       27 );  // bool
        } Gen8;

        DWORD       Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  1 );
            DWORD       DestinationAddressType              : BITFIELD_BIT(        2 );  // GFX3DCONTROL_GTTWRITE_MODE
            DWORD       DestinationAddress                  : BITFIELD_RANGE(  3, 31 );  // GTT[31:3]
        } All;

        DWORD       Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       Data;
        } All;

        struct _Gen8
        {
            DWORD       Destination64bitAddress             : BITFIELD_RANGE(  0, 15 );  // GTT[47:32]
            DWORD       _Unused                             : BITFIELD_RANGE( 16, 31 );  // Reserved
        } Gen8;

        DWORD       Value;
    } DW3;

    union _DW4
    {
        struct _All
        {
            DWORD       Data;
        } All;

        DWORD       Value;
    } DW4;

    union _DW5
    {
        struct _All
        {
            DWORD       Data;
        } All;

        DWORD       Value;
    } DW5;

};

static_assert(SIZE32(S3DPipeControl) == 6);

}  // namespace G6HWC

// Reset packing alignment to project default
#pragma pack()
