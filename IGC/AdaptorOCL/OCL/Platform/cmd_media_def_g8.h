/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_media_enum_g8.h"

// Set packing alignment to a single byte
#pragma pack(1)

namespace G6HWC
{

/*****************************************************************************\
STRUCT: SMediaStateInterfaceDescriptorData (INTERFACE_DESCRIPTOR_DATA)
\*****************************************************************************/
struct SMediaStateInterfaceDescriptorData
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  5 );
            DWORD       KernelStartPointer                  : BITFIELD_RANGE(  6, 31 ); // GTT[31:6]
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused1                            : BITFIELD_RANGE(  0,  6 );
            DWORD       SoftwareExceptionEnable             : BITFIELD_BIT(        7 ); // bool
            DWORD       _Unused2                            : BITFIELD_RANGE(  8, 10 );
            DWORD       MaskStackExceptionEnable            : BITFIELD_BIT(       11 ); // bool
            DWORD       _Unused3                            : BITFIELD_BIT(       12 );
            DWORD       IllegalOpcodeExceptionEnable        : BITFIELD_BIT(       13 ); // bool
            DWORD       _Unused4                            : BITFIELD_RANGE( 14, 15 );
            DWORD       FloatingPointMode                   : BITFIELD_BIT(       16 ); // GFXMEDIASTATE_FLOATING_POINT_MODE
            DWORD       ThreadPriority                      : BITFIELD_BIT(       17 ); // GFXMEDIASTATE_THREAD_PRIORITY
            DWORD       SingleProgramFlow                   : BITFIELD_BIT(       18 ); // GFXMEDIASTATE_SINGLE_PROGRAM_FLOW
            DWORD       _Unused5                            : BITFIELD_BIT(       19 );
            DWORD       _Unused6                            : BITFIELD_RANGE( 20, 25 ); // U6 [0,63]
            DWORD       _Unused7                            : BITFIELD_RANGE( 26, 31 ); // U6 [0,63]
        } All;

        struct _Gen8
        {
            DWORD       Kernel64bitStartPointer             : BITFIELD_RANGE(  0, 15 ); // GTT[47:32]
            DWORD       _Unused                             : BITFIELD_RANGE( 16, 31 ); // Reserved
        } Gen8;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  1 );
            DWORD       SamplerCount                        : BITFIELD_RANGE(  2,  4 ); // U3 [0,4]
            DWORD       SamplerStatePointer                 : BITFIELD_RANGE(  5, 31 ); // GTT[31:5]
        } All;

        struct _Gen8
        {
            DWORD       _Unused1                            : BITFIELD_RANGE(  0,  6 );
            DWORD       SoftwareExceptionEnable             : BITFIELD_BIT(        7 ); // bool
            DWORD       _Unused2                            : BITFIELD_RANGE(  8, 10 );
            DWORD       MaskStackExceptionEnable            : BITFIELD_BIT(       11 ); // bool
            DWORD       _Unused3                            : BITFIELD_BIT(       12 );
            DWORD       IllegalOpcodeExceptionEnable        : BITFIELD_BIT(       13 ); // bool
            DWORD       _Unused4                            : BITFIELD_RANGE( 14, 15 );
            DWORD       FloatingPointMode                   : BITFIELD_BIT(       16 ); // GFXMEDIASTATE_FLOATING_POINT_MODE
            DWORD       ThreadPriority                      : BITFIELD_BIT(       17 ); // GFXMEDIASTATE_THREAD_PRIORITY
            DWORD       SingleProgramFlow                   : BITFIELD_BIT(       18 ); // GFXMEDIASTATE_SINGLE_PROGRAM_FLOW
            DWORD       DenormMode                          : BITFIELD_BIT(       19 ); // U1 [0,1]
            DWORD       _Unused5                            : BITFIELD_RANGE( 20, 31 ); // Reserved
        } Gen8;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       BindingTableEntryCount              : BITFIELD_RANGE(  0,  4 ); // U5 [0,5]
            DWORD       BindingTablePointer                 : BITFIELD_RANGE(  5, 31 ); // GTT[31:5]
        } All;

        struct _Gen8
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  1 ); // Reserved
            DWORD       SamplerCount                        : BITFIELD_RANGE(  2,  4 ); // U3 [0,4]
            DWORD       SamplerStatePointer                 : BITFIELD_RANGE(  5, 31 ); // GTT[31:5]
        } Gen8;

        DWORD   Value;
    } DW3;

    // DWORD 4
    union _DW4
    {
        struct _All
        {
            DWORD       ConstantURBEntryReadOffset          : BITFIELD_RANGE(  0, 15 );
            DWORD       ConstantURBEntryReadLength          : BITFIELD_RANGE(  16, 31 );
        } All;

        struct _Gen8
        {
            DWORD       BindingTableEntryCount              : BITFIELD_RANGE(  0,  4 ); // U5 [0,31]
            DWORD       BindingTablePointer                 : BITFIELD_RANGE(  5, 15 ); // GTT[15:5]
            DWORD       _Unused                             : BITFIELD_RANGE( 16, 31 ); // Reserved
        } Gen8;

        DWORD   Value;
    } DW4;

    // DWORD 5
    union _DW5
    {
        struct _All
        {
            DWORD       BarrierId                           : BITFIELD_RANGE( 0, 3 );
            DWORD       _Unused                             : BITFIELD_RANGE( 4, 31);
        } All;

        struct _Gen7
        {
            DWORD       NumberOfThreadsInThreadGroup        : BITFIELD_RANGE(  0, 7  );  // U8
            DWORD       _Unused1                            : BITFIELD_RANGE(  8, 15 );
            DWORD       SharedLocalMemorySize               : BITFIELD_RANGE( 16, 20 );  // U5
            DWORD       BarrierEnable                       : BITFIELD_BIT( 21 );        // bool
            DWORD       RoundingMode                        : BITFIELD_RANGE( 22, 23 );  // U2
            DWORD       _Unused2                            : BITFIELD_RANGE( 24, 31 );
        } Gen7;

        struct _Gen8
        {
            DWORD       ConstantURBEntryReadOffset          : BITFIELD_RANGE(  0, 15 );
            DWORD       ConstantURBEntryReadLength          : BITFIELD_RANGE(  16, 31 );
        } Gen8;

        DWORD   Value;
    } DW5;

    // DWORD 6
    union _DW6
    {
        struct _Gen7_5
        {
            DWORD       CrossThreadConstantDataReadLength   : BITFIELD_RANGE(  0, 7  ); // U8
            DWORD       _Unused                             : BITFIELD_RANGE(  8, 31 );
        } Gen7_5;

        struct _Gen8
        {
            DWORD       NumberOfThreadsInThreadGroup        : BITFIELD_RANGE(  0,  9 );  // U9
            DWORD       _Unused1                            : BITFIELD_RANGE( 10, 14 );  // Reserved
            DWORD       GlobalBarrierEnable                 : BITFIELD_BIT(       15 );  // bool
            DWORD       SharedLocalMemorySize               : BITFIELD_RANGE( 16, 20 );  // U5
            DWORD       BarrierEnable                       : BITFIELD_BIT(       21 );  // bool
            DWORD       RoundingMode                        : BITFIELD_RANGE( 22, 23 );  // U2
            DWORD       _Unused2                            : BITFIELD_RANGE( 24, 31 );  // Reserved
        } Gen8;

        DWORD   Value;
    } DW6;

    // DWORD 7
    union _DW7
    {
        struct _Gen8
        {
            DWORD       CrossThreadConstantDataReadLength   : BITFIELD_RANGE(  0, 7  ); // U8
            DWORD       _Unused                             : BITFIELD_RANGE(  8, 31 ); // Reserved
        } Gen8;

        DWORD   Value;
    } DW7;
};

static_assert(SIZE32(SMediaStateInterfaceDescriptorData) == 8);

/*****************************************************************************\
STRUCT: SMediaStateMediaInterfaceDescriptorLoad (MEDIA_INTERFACE_DESCRIPTOR_LOAD)
\*****************************************************************************/
struct SMediaStateMediaInterfaceDescriptorLoad
{
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0, 15 ); // OP_LENGTH (exclude dw0,dw1)
            DWORD       InstructionSubOpcode                : BITFIELD_RANGE( 16, 23 ); // GFX_MEDIA_PIPELINED_SUBOPCODE
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26 ); // GFX_OPCODE
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28 ); // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31 ); // INSTRUCTION_TYPE
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       InterfaceDescriptorTotalLength      : BITFIELD_RANGE( 0, 16 );
            DWORD       _Unused                             : BITFIELD_RANGE( 17, 31 );
        } All;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       InterfaceDescriptorDataStartAddress : BITFIELD_RANGE( 0, 31 );
        } All;

        DWORD   Value;
    } DW3;
};

static_assert(SIZE32(SMediaStateMediaInterfaceDescriptorLoad) == 4);

/*****************************************************************************\
STRUCT: SMediaStateMediaVFEState (MEDIA_VFE_STATE)
\*****************************************************************************/
struct SMediaStateMediaVFEState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0, 15 ); // OP_LENGTH (exclude dw0,dw1)
            DWORD       InstructionSubOpcode                : BITFIELD_RANGE( 16, 23 ); // GFX_MEDIA_PIPELINED_SUBOPCODE
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26 ); // GFX_OPCODE
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28 ); // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31 ); // INSTRUCTION_TYPE
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       PerThreadScratchSpace               : BITFIELD_RANGE(  0,  3 ); // U4 [0, 11] -> [1k, 12k]
            DWORD       StackSize                           : BITFIELD_RANGE(  4,  7 ); // U4 [0, 11] -> [1k, 2MB]
            DWORD       _Unused                             : BITFIELD_RANGE(  8,  9 ); // reserved
            DWORD       ScratchSpaceBasePointer             : BITFIELD_RANGE( 10, 31 ); // GTT[31:10]
        } All;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       DebugCounterControl                 : BITFIELD_RANGE(  0,  1 ); // GFXMEDIASTATE_DEBUG_COUNTER_CONTROL
            DWORD       _Unused                             : BITFIELD_RANGE(  2,  4 );
            DWORD       FastPreempt                         : BITFIELD_BIT(        5 ); // bool
            DWORD       BypassGatewayControl                : BITFIELD_BIT(        6 ); // bool
            DWORD       ResetGatewayTimer                   : BITFIELD_BIT(        7 ); // bool
            DWORD       NumberOfURBEntries                  : BITFIELD_RANGE(  8, 15 ); // U8 [0,64]
            DWORD       MaximumNumberOfThreads              : BITFIELD_RANGE( 16, 31 ); // U16 = thread count -1
        } All;

        struct _Gen7
        {
            DWORD       DebugCounterControl                 : BITFIELD_RANGE(  0,  1 );  // GFXMEDIA_DEBUG_COUNTER_CONTROL
            DWORD       GPGPUMode                           : BITFIELD_BIT(        2 );  // GFXMEDIA_GPGPU_MODE
            DWORD       GatewayMMIOAccessControl            : BITFIELD_RANGE(  3,  4 );  // GFXMEDIA_MMIO_ACCESS_CONTROL
            DWORD       FastPreemptEnable                   : BITFIELD_BIT(        5 );  // bool
            DWORD       BypassGatewayControl                : BITFIELD_BIT(        6 );
            DWORD       ResetGatewayTimer                   : BITFIELD_BIT(        7 );
            DWORD       NumberofURBEntries                  : BITFIELD_RANGE(  8, 15 );  // U8
            DWORD       MaximumNumberOfThreads              : BITFIELD_RANGE( 16, 31 );  // U4
        }   Gen7;

        struct _Gen8
        {
            DWORD       ScratchSpace64bitBasePointer        : BITFIELD_RANGE(  0, 15 ); // GTT[47:32]
            DWORD       _Unused                             : BITFIELD_RANGE( 16, 31 ); // reserved
        } Gen8;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  7 );
            DWORD       ObjectId                            : BITFIELD_RANGE(  8, 31 );
        } All;

        struct _Gen8
        {
            DWORD       DebugCounterControl                 : BITFIELD_RANGE(  0,  1 );  // GFXMEDIA_DEBUG_COUNTER_CONTROL
            DWORD       GPGPUMode                           : BITFIELD_BIT(        2 );  // GPGPUMode
            DWORD       GatewayMMIOAccessControl            : BITFIELD_RANGE(  3,  4 );  // GFXMEDIA_MMIO_ACCESS_CONTROL
            DWORD       FastPreemptEnable                   : BITFIELD_BIT(        5 );  // bool
            DWORD       BypassGatewayControl                : BITFIELD_BIT(        6 );
            DWORD       ResetGatewayTimer                   : BITFIELD_BIT(        7 );
            DWORD       NumberofURBEntries                  : BITFIELD_RANGE(  8, 15 );  // U8
            DWORD       MaximumNumberOfThreads              : BITFIELD_RANGE( 16, 31 );  // U4
        }   Gen8;

        struct _Gen9
        {
            DWORD       DebugCounterControl                 : BITFIELD_RANGE(  0,  1 );  // GFXMEDIA_DEBUG_COUNTER_CONTROL
            DWORD       SLMGranularity                      : BITFIELD_BIT(        2 );  // GFXMEDIASTATE_SLMGranularity
            DWORD       GatewayMMIOAccessControl            : BITFIELD_RANGE(  3,  4 );  // GFXMEDIA_MMIO_ACCESS_CONTROL
            DWORD       _Unused                             : BITFIELD_BIT(        5 );  // reserved
            DWORD       BypassGatewayControl                : BITFIELD_BIT(        6 );
            DWORD       ResetGatewayTimer                   : BITFIELD_BIT(        7 );
            DWORD       NumberofURBEntries                  : BITFIELD_RANGE(  8, 15 );  // U8
            DWORD       MaximumNumberOfThreads              : BITFIELD_RANGE( 16, 31 );  // U4
        }   Gen9;

        DWORD   Value;
    } DW3;

    // DWORD 4
    union _DW4
    {
        struct _All
        {
            DWORD       CURBEAllocationSize                 : BITFIELD_RANGE(  0, 15 ); // U9 [0, 2048]
            DWORD       URBEntryAllocationSize              : BITFIELD_RANGE( 16, 31 ); // U9 [0, 2048]
        } All;

        struct _Gen8
        {
            DWORD       _Unused                             : BITFIELD_RANGE(  0,  7 ); // reserved
            DWORD       ObjectID                            : BITFIELD_RANGE(  8, 31 );
        } Gen8;

        struct _Gen10
        {
            DWORD       SliceDisable                        : BITFIELD_RANGE(  0,  1 );
            DWORD       FlushOnBarriers                     : BITFIELD_RANGE(  2,  3 );
            DWORD       _Unused                             : BITFIELD_RANGE(  4,  7 ); // reserved
            DWORD       ObjectID                            : BITFIELD_RANGE(  8, 31 );
        } Gen10;

        DWORD   Value;
    } DW4;

    // DWORD 5
    union _DW5
    {
        struct _All
        {
            DWORD       ScoreboardMask                      : BITFIELD_RANGE(  0,  7 ); // 8 bits for score 0 to 7
            DWORD       _Unused                             : BITFIELD_RANGE(  8, 29 );
            DWORD       ScoreBoardType                      : BITFIELD_BIT(       30 ); // GFXMEDIASTATE_SCOREBOARD_TYPE
            DWORD       ScoreBoardEnable                    : BITFIELD_BIT(       31 ); // bool
        } All;

        struct _Gen8
        {
            DWORD       CURBEAllocationSize                 : BITFIELD_RANGE(  0, 15 ); // U9 [0, 2048]
            DWORD       URBEntryAllocationSize              : BITFIELD_RANGE( 16, 31 ); // U9 [0, 2048]
        } Gen8;

        DWORD   Value;
    } DW5;

    // DWORD 6
    union _DW6
    {
        struct _All
        {
            DWORD       ScoreboardDeltaX0                   : BITFIELD_RANGE(  0,  3 ); // s3
            DWORD       ScoreboardDeltaY0                   : BITFIELD_RANGE(  4,  7 ); // s3
            DWORD       ScoreboardDeltaX1                   : BITFIELD_RANGE(  8, 11 ); // s3
            DWORD       ScoreboardDeltaY1                   : BITFIELD_RANGE( 12, 15 ); // s3
            DWORD       ScoreboardDeltaX2                   : BITFIELD_RANGE( 16, 19 ); // s3
            DWORD       ScoreboardDeltaY2                   : BITFIELD_RANGE( 20, 23 ); // s3
            DWORD       ScoreboardDeltaX3                   : BITFIELD_RANGE( 24, 27 ); // s3
            DWORD       ScoreboardDeltaY3                   : BITFIELD_RANGE( 28, 31 ); // s3
        } All;

        struct _Gen8
        {
            DWORD       ScoreboardMask                      : BITFIELD_RANGE(  0,  7 ); // 8 bits for score 0 to 7
            DWORD       _Unused                             : BITFIELD_RANGE(  8, 29 );
            DWORD       ScoreBoardType                      : BITFIELD_BIT(       30 ); // GFXMEDIASTATE_SCOREBOARD_TYPE
            DWORD       ScoreBoardEnable                    : BITFIELD_BIT(       31 ); // bool
        } Gen8;

        struct _Gen10
        {
            DWORD       ScoreboardMask                      : BITFIELD_RANGE(  0,  7 ); // 8 bits for score 0 to 7
            DWORD       NumMediaObjPerPreEmptionCheckpoint  : BITFIELD_RANGE(  8, 15 ); // how many MEDIA_OBJECT commands are executed between checkpoints for pre-emption.
            DWORD       _Unused                             : BITFIELD_RANGE(  16, 29 );
            DWORD       ScoreBoardType                      : BITFIELD_BIT(       30 ); // GFXMEDIASTATE_SCOREBOARD_TYPE
            DWORD       ScoreBoardEnable                    : BITFIELD_BIT(       31 ); // bool
        } Gen10;

        DWORD   Value;
    } DW6;

    // DWORD 7
    union _DW7
    {
        struct _All
        {
            DWORD       ScoreboardDeltaX4                   : BITFIELD_RANGE(  0,  3 ); // s3
            DWORD       ScoreboardDeltaY4                   : BITFIELD_RANGE(  4,  7 ); // s3
            DWORD       ScoreboardDeltaX5                   : BITFIELD_RANGE(  8, 11 ); // s3
            DWORD       ScoreboardDeltaY5                   : BITFIELD_RANGE( 12, 15 ); // s3
            DWORD       ScoreboardDeltaX6                   : BITFIELD_RANGE( 16, 19 ); // s3
            DWORD       ScoreboardDeltaY6                   : BITFIELD_RANGE( 20, 23 ); // s3
            DWORD       ScoreboardDeltaX7                   : BITFIELD_RANGE( 24, 27 ); // s3
            DWORD       ScoreboardDeltaY7                   : BITFIELD_RANGE( 28, 31 ); // s3
        } All;

        struct _Gen8
        {
            DWORD       ScoreboardDeltaX0                   : BITFIELD_RANGE(  0,  3 ); // s3
            DWORD       ScoreboardDeltaY0                   : BITFIELD_RANGE(  4,  7 ); // s3
            DWORD       ScoreboardDeltaX1                   : BITFIELD_RANGE(  8, 11 ); // s3
            DWORD       ScoreboardDeltaY1                   : BITFIELD_RANGE( 12, 15 ); // s3
            DWORD       ScoreboardDeltaX2                   : BITFIELD_RANGE( 16, 19 ); // s3
            DWORD       ScoreboardDeltaY2                   : BITFIELD_RANGE( 20, 23 ); // s3
            DWORD       ScoreboardDeltaX3                   : BITFIELD_RANGE( 24, 27 ); // s3
            DWORD       ScoreboardDeltaY3                   : BITFIELD_RANGE( 28, 31 ); // s3
        } Gen8;

        DWORD   Value;
    } DW7;

    // DWORD 8
    union _DW8
    {
        struct _Gen8
        {
            DWORD       ScoreboardDeltaX4                   : BITFIELD_RANGE(  0,  3 ); // s3
            DWORD       ScoreboardDeltaY4                   : BITFIELD_RANGE(  4,  7 ); // s3
            DWORD       ScoreboardDeltaX5                   : BITFIELD_RANGE(  8, 11 ); // s3
            DWORD       ScoreboardDeltaY5                   : BITFIELD_RANGE( 12, 15 ); // s3
            DWORD       ScoreboardDeltaX6                   : BITFIELD_RANGE( 16, 19 ); // s3
            DWORD       ScoreboardDeltaY6                   : BITFIELD_RANGE( 20, 23 ); // s3
            DWORD       ScoreboardDeltaX7                   : BITFIELD_RANGE( 24, 27 ); // s3
            DWORD       ScoreboardDeltaY7                   : BITFIELD_RANGE( 28, 31 ); // s3
        } Gen8;

        DWORD   Value;
    } DW8;
};

static_assert(SIZE32(SMediaStateMediaVFEState) == 9);

/*****************************************************************************\
STRUCT: SMediaStateMediaCURBELoad (MEDIA_CURBE_LOAD)
\*****************************************************************************/
struct SMediaStateMediaCURBELoad
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0, 15 ); // OP_LENGTH (exclude dw0,dw1)
            DWORD       InstructionSubOpcode                : BITFIELD_RANGE( 16, 23 ); // GFX_MEDIA_PIPELINED_SUBOPCODE
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26 ); // GFX_OPCODE
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28 ); // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31 ); // INSTRUCTION_TYPE
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       CURBETotalDataLength                : BITFIELD_RANGE(  0, 16 ); // U17
            DWORD       _Unused                             : BITFIELD_RANGE( 17, 31 );
        } All;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       CURBEDataStartAddress               : BITFIELD_RANGE(  0, 31 ); // U17
        } All;

        DWORD   Value;
    } DW3;
};

static_assert(SIZE32(SMediaStateMediaCURBELoad) == 4);

/*****************************************************************************\
STRUCT: SMediaStateMediaStateFlush (MEDIA_STATE_FLUSH)
\*****************************************************************************/
struct SMediaStateMediaStateFlush
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0, 15); // OP_LENGTH (exclude dw0,dw1)
            DWORD       InstructionSubOpcode                : BITFIELD_RANGE( 16, 23); // GFX_MEDIA_PIPELINED_SUBOPCODE
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26); // GFX_OPCODE
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28); // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31); // INSTRUCTION_TYPE
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       BarrierMask                         : BITFIELD_RANGE(  0, 15 ); // One bit for each barrier
            DWORD       ThreadCountWaterMark                : BITFIELD_RANGE( 16, 23 ); // U8
            DWORD       _Unused                             : BITFIELD_RANGE( 24, 31 );
        } All;

        struct _Gen7
        {
            DWORD       InterfaceDescriptorOffset           : BITFIELD_RANGE(  0, 5 ); // U6
            DWORD       ThreadCountWaterMark                : BITFIELD_BIT( 6 ); // one bit specify if stall waiting resources
            DWORD       _Unused                             : BITFIELD_RANGE( 7, 31 );
        } Gen7;

        DWORD   Value;
    } DW1;
};

static_assert(SIZE32(SMediaStateMediaStateFlush) == 2);

/*****************************************************************************\
STRUCT: SMediaStateGPGPUWalker (GPGPU_WALKER)
\*****************************************************************************/
struct SMediaStateGPGPUWalker
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(  0,  7 ); // OP_LENGTH (exclude dw0,dw1)
            DWORD       PredicateEnable                     : BITFIELD_BIT(        8 ); // bool
            DWORD       _Unused1                            : BITFIELD_BIT(        9 );
            DWORD       IndirectParameterEnable             : BITFIELD_BIT(       10 ); // bool
            DWORD       _Unused2                            : BITFIELD_RANGE( 11, 15 );
            DWORD       InstructionSubOpcodeA               : BITFIELD_RANGE( 16, 23 ); // GFX_MEDIA_NONPIPELINED_SUBOPCODE_A
            DWORD       InstructionOpcode                   : BITFIELD_RANGE( 24, 26 ); // GFX_OPCODE
            DWORD       InstructionSubType                  : BITFIELD_RANGE( 27, 28 ); // INSTRUCTION_SUBTYPE
            DWORD       InstructionType                     : BITFIELD_RANGE( 29, 31 ); // INSTRUCTION_TYPE
        } All;

        DWORD   Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       InterfaceDescriptorOffset           : BITFIELD_RANGE(  0,  5 ); // U5
            DWORD       _Unused                             : BITFIELD_RANGE(  6,  7 );
            DWORD       ObjectId                            : BITFIELD_RANGE(  8, 31 ); // U8
        } All;

        DWORD   Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       ThreadWidthCounterMaximum           : BITFIELD_RANGE(  0,  5 );
            DWORD       _Unused1                            : BITFIELD_RANGE(  6,  7 );
            DWORD       ThreadHeightCounterMaximum          : BITFIELD_RANGE(  8, 13 );
            DWORD       _Unused2                            : BITFIELD_RANGE( 14, 15 );
            DWORD       ThreadDepthCounterMaximum           : BITFIELD_RANGE( 16, 21 );
            DWORD       _Unused3                            : BITFIELD_RANGE( 22, 29 );
            DWORD       SIMDSize                            : BITFIELD_RANGE( 30, 31 ); // GFXMEDIASTATE_GPGPU_WALKER_SIMD_SIZE
        } All;

        struct _Gen8
        {
            DWORD       IndirectDataLength                  : BITFIELD_RANGE(  0, 16 ); // U17 in bytes
            DWORD       _Unused                             : BITFIELD_RANGE( 17, 31 ); // Reserved
        } Gen8;

        DWORD   Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       ThreadGroupIdStartingX              : BITFIELD_RANGE(  0, 31 ); // U31
        } All;

        struct _Gen8
        {
            DWORD       IndirectDataStartAddress            : BITFIELD_RANGE(  0, 31 ); // 64-byte aligned address
        } Gen8;

        DWORD   Value;
    } DW3;

    // DWORD 4
    union _DW4
    {
        struct _All
        {
            DWORD       ThreadGroupIdDimensionX             : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadWidthCounterMaximum           : BITFIELD_RANGE(  0,  5 );
            DWORD       _Unused1                            : BITFIELD_RANGE(  6,  7 ); // Reserved
            DWORD       ThreadHeightCounterMaximum          : BITFIELD_RANGE(  8, 13 );
            DWORD       _Unused2                            : BITFIELD_RANGE( 14, 15 ); // Reserved
            DWORD       ThreadDepthCounterMaximum           : BITFIELD_RANGE( 16, 21 );
            DWORD       _Unused3                            : BITFIELD_RANGE( 22, 29 ); // Reserved
            DWORD       SIMDSize                            : BITFIELD_RANGE( 30, 31 ); // GFXMEDIASTATE_GPGPU_WALKER_SIMD_SIZE
        } Gen8;

        DWORD   Value;
    } DW4;

    // DWORD 5
    union _DW5
    {
        struct _All
        {
            DWORD       ThreadGroupIdStartingY              : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdStartingX              : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW5;

    // DWORD 6
    union _DW6
    {
        struct _All
        {
            DWORD       ThreadGroupIdDimensionY             : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdResumeX                : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW6;

    // DWORD 7
    union _DW7
    {
        struct _All
        {
            DWORD       ThreadGroupIdStartingZ              : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdDimensionX             : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW7;

    // DWORD 8
    union _DW8
    {
        struct _All
        {
            DWORD       ThreadGroupIdDimensionZ             : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdStartingY              : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW8;

    // DWORD 9
    union _DW9
    {
        struct _All
        {
            DWORD       RightExecutionMask                  : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdResumeY                : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW9;

    // DWORD 10
    union _DW10
    {
        struct _All
        {
            DWORD       BottomExecutionMask                 : BITFIELD_RANGE(  0, 31 );
        } All;

        struct _Gen8
        {
            DWORD       ThreadGroupIdDimensionY             : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW10;

    // DWORD 11
    union _DW11
    {
        struct _Gen8
        {
            DWORD       ThreadGroupIdStartingResumeZ        : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW11;

    // DWORD 12
    union _DW12
    {
        struct _Gen8
        {
            DWORD       ThreadGroupIdDimensionZ             : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW12;

    // DWORD 13
    union _DW13
    {
        struct _Gen8
        {
            DWORD       RightExecutionMask                  : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW13;

    // DWORD 14
    union _DW14
    {
        struct _Gen8
        {
            DWORD       BottomExecutionMask                 : BITFIELD_RANGE(  0, 31 );
        } Gen8;

        DWORD   Value;
    } DW14;
};

static_assert(SIZE32(SMediaStateGPGPUWalker) == 15);

}  // namespace G6HWC

// Reset packing alignment to project default
#pragma pack()
