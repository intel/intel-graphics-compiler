/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_mi_enum_g6.h"

// Set packing alignment to a single byte
#pragma pack(1)

namespace G6HWC
{
/*****************************************************************************\
STRUCT: SMIARBCheck (MI_ARB_CHECK)
\*****************************************************************************/
struct SMIARBCheck
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};
static_assert(SIZE32(SMIARBCheck) == 1);

/*****************************************************************************\
STRUCT: SMIARBOnOff (MI_ARB_ON_OFF)
\*****************************************************************************/
struct SMIARBOnOff
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       ArbitrationEnable                   : BITFIELD_BIT(0);          // bool
            DWORD       _Unused                             : BITFIELD_RANGE(1,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};
static_assert(SIZE32(SMIARBOnOff) == 1);

/*****************************************************************************\
STRUCT: SMIBatchBufferEnd (MI_BATCH_BUFFER_END)
\*****************************************************************************/
struct SMIBatchBufferEnd
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIBatchBufferEnd) == 1);

/*****************************************************************************\
STRUCT: SMIBatchBufferStart (MI_BATCH_BUFFER_START)
\*****************************************************************************/
struct SMIBatchBufferStart
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused1                            : BITFIELD_RANGE(6,7);      //
            DWORD       BufferSecurityIndicator             : BITFIELD_BIT(8);          // MI_BUFFER_SECURITY_INDICATOR
            DWORD       _Unused2                            : BITFIELD_RANGE(9,22);
            //DWORD       CommandArbitrationControl           : BITFIELD_RANGE(9,10);     // MI_COMMAND_ARBITRATION_CONTROL
            //DWORD       ClearCommandBufferEnable            : BITFIELD_BIT(11);         // bool
            //DWORD       _Unused3                          : BITFIELD_RANGE(12,22);    //
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,1);
            DWORD       BufferStartAddress                  : BITFIELD_RANGE(2,31);     // GTT[31:6] | PHYS[31:6]
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMIBatchBufferStart) == 2);


/*****************************************************************************\
STRUCT: SMIDisplayFlip (MI_DISPLAY_FLIP)
\*****************************************************************************/
struct SMIDisplayFlip
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,19);
            DWORD       DisplayPlaneSelect                  : BITFIELD_RANGE(20,21);    // MI_DISPLAY_PLANE_SELECT
            DWORD       AsynchronousFlip                    : BITFIELD_BIT(22);         // MI_ASYNCHRONOUS_FLIP
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused1                            : BITFIELD_RANGE(0,2);
            DWORD       DisplayBufferPitch                  : BITFIELD_RANGE(3,14);     // U12
            DWORD       _Unused2                            : BITFIELD_RANGE(15,28);
            DWORD       FlipQueueSelect                     : BITFIELD_BIT(29);         // MI_FLIP_QUEUE_SELECT
            DWORD       _Unused3                            : BITFIELD_RANGE(30,31);
        } All;
        DWORD       Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       TileParameter                       : BITFIELD_BIT(0);          // MI_TILE_PARAMETER
            DWORD       _Unused                             : BITFIELD_RANGE(1,11);
            DWORD       DisplayBufferBaseAddress            : BITFIELD_RANGE(12,31);    // GTT[31:12]
        } All;
        DWORD       Value;
    } DW2;

    // DWORD 3
    union _DW3
    {
        struct _All
        {
            DWORD       PipeVerticalSourceImageResize       : BITFIELD_RANGE(0,11);     // U32 TODO: TODO: TODO: ?ok? really u32... how?
            DWORD       _Unused1                            : BITFIELD_RANGE(12,15);    // U32 TODO: TODO: TODO: ?ok? really u32... how?
            DWORD       PipeHorizontalSourceImageSize       : BITFIELD_RANGE(16,27);
            DWORD       _Unused2                            : BITFIELD_RANGE(28,29);
            DWORD       PanelFitterSelect                   : BITFIELD_BIT(30);         // MI_PANEL_FITTER
            DWORD       EnablePanelFitter                   : BITFIELD_BIT(31);         // bool
        } All;
        DWORD       Value;
    } DW3;
};

static_assert(SIZE32(SMIDisplayFlip) == 4);

/*****************************************************************************\
STRUCT: SMIFlush (MI_FLUSH)
\*****************************************************************************/
struct SMIFlush
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused1                            : BITFIELD_BIT(0);
            DWORD       StateCacheInvalidate                : BITFIELD_BIT(1);          // bool
            DWORD       RenderCacheFlushInhibit             : BITFIELD_BIT(2);          // bool
            DWORD       GlobalSnapshotCountReset            : BITFIELD_BIT(3);          // bool
            DWORD       GenericMediaStateClear              : BITFIELD_BIT(4);          // bool
            DWORD       IndirectStatePointerDisable         : BITFIELD_BIT(5);          // bool
            DWORD       ProtectedMemoryEnable               : BITFIELD_BIT(6);          // bool
            DWORD       _Unused2                            : BITFIELD_RANGE(7,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIFlush) == 1);

/*****************************************************************************\
STRUCT: SMILoadRegisterImmediate (MI_LOAD_REGISTER_IMM)
\*****************************************************************************/
struct SMILoadRegisterImmediate
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused1                            : BITFIELD_RANGE(6,7);
            DWORD       Byte0WriteDisable                   : BITFIELD_BIT(8);          // bool
            DWORD       Byte1WriteDisable                   : BITFIELD_BIT(9);          // bool
            DWORD       Byte2WriteDisable                   : BITFIELD_BIT(10);         // bool
            DWORD       Byte3WriteDisable                   : BITFIELD_BIT(11);         // bool
            DWORD       _Unused2                            : BITFIELD_RANGE(12,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,1);
            DWORD       RegisterOffset                      : BITFIELD_RANGE(2,31);     // MMIO_OFFSET[31:2]
        } All;
        DWORD       Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       DataDWord;
        } All;
        DWORD       Value;
    } DW2;
};

static_assert(SIZE32(SMILoadRegisterImmediate) == 3);

/*****************************************************************************\
STRUCT: SMILoadScanLinesExclusive (MI_LOAD_SCAN_LINES_EXCL)
\*****************************************************************************/
struct SMILoadScanLinesExclusive
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused1                            : BITFIELD_RANGE(6,19);
            DWORD       DisplayPipeSelect                   : BITFIELD_RANGE(20,21);    // MI_DISPLAY_PIPE_SELECT
            DWORD       _Unused2                            : BITFIELD_BIT(22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            WORD        EndScanLineNumber;                          // U16
            WORD        StartScanLineNumber;                        // U16
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMILoadScanLinesExclusive) == 2);

/*****************************************************************************\
STRUCT: SMILoadScanLinesInclusive (MI_LOAD_SCAN_LINES_INCL)
\*****************************************************************************/
struct SMILoadScanLinesInclusive
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused1                            : BITFIELD_RANGE(6,19);
            DWORD       DisplayPipeSelect                   : BITFIELD_RANGE(20,21);    // MI_DISPLAY_PIPE_SELECT
            DWORD       _Unused2                            : BITFIELD_BIT(22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            WORD        EndScanLineNumber;                          // U16
            WORD        StartScanLineNumber;                        // U16
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMILoadScanLinesInclusive) == 2);

/*****************************************************************************\
STRUCT: SMINoop (MI_NOOP)
\*****************************************************************************/
struct SMINoop
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       IdentificationNumber                    : BITFIELD_RANGE(0,21);     // DWORD
            DWORD       IdentificationNumberRegisterWriteEnable : BITFIELD_BIT(22);         // bool
            DWORD       InstructionOpcode                       : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                         : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMINoop) == 1);

/*****************************************************************************\
STRUCT: SMIOverlayFlip (MI_OVERLAY_FLIP)
\*****************************************************************************/
struct SMIOverlayFlip
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                                      : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                                     : BITFIELD_RANGE(6,20);
            DWORD       ModeFlags                                   : BITFIELD_RANGE(21,22);    // MI_MODE_FLAGS
            DWORD       InstructionOpcode                           : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                             : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       OverlayFilterCoefficientRegisterUpdateFlag  : BITFIELD_BIT(0);          // bool
            DWORD       _Unused                                     : BITFIELD_RANGE(1,11);
            DWORD       RegisterAndCoefficientUpdateAddress         : BITFIELD_RANGE(12,31);    // GTT[31:13]
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMIOverlayFlip) == 2);

/*****************************************************************************\
STRUCT: SMIProbeHeader (MI_PROBE)
\*****************************************************************************/
struct SMIProbeHeader
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                                      : BITFIELD_RANGE(0,9);      // OP_LENGTH
            DWORD       _Unused                                     : BITFIELD_RANGE(10,22);
            DWORD       InstructionOpcode                           : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                             : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};
static_assert(SIZE32(SMIProbeHeader) == 1);

/*****************************************************************************\
STRUCT: SMIProbeState (MI_PROBE)
\*****************************************************************************/
struct SMIProbeState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       SlotNumber                                  : BITFIELD_RANGE(0,9);      // bool
            DWORD       _Unused                                     : BITFIELD_RANGE(10,11);
            DWORD       SurfacePageBaseAddress                      : BITFIELD_RANGE(12,31);    // GTT[31:13]
        } All;
        DWORD       Value;
    } DW0;
};
static_assert(SIZE32(SMIProbeState) == 1);

/*****************************************************************************\
STRUCT: SMIProbe (MI_PROBE)
\*****************************************************************************/
struct SMIProbe
{
    SMIProbeHeader              Header;
    SMIProbeState               Probe[g_cNumProbes];
};

/*****************************************************************************\
STRUCT: SMIReportHead (MI_REPORT_HEAD)
\*****************************************************************************/
struct SMIReportHead
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIReportHead) == 1);

/*****************************************************************************\
STRUCT: SMIReportNonCE (MI_REPORT_NONCE)
\*****************************************************************************/
struct SMIReportNonCE
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       NonCEValue;     // U32
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMIReportNonCE) == 2);

/*****************************************************************************\
STRUCT: SMISemaphoreMBox (MI_SEMAPHORE_MBOX)
\*****************************************************************************/
struct SMISemaphoreMBox
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,18);
            DWORD       CommandBufferTerminateEnable        : BITFIELD_BIT(19);         // bool
            DWORD       CompareSemaphore                    : BITFIELD_BIT(20);         // bool
            DWORD       UpdateSemaphore                     : BITFIELD_BIT(21);         // bool
            DWORD       UseGlobalGTT                        : BITFIELD_BIT(22);         // MI_MEMORY_USE_GLOBAL_GTT
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       SemaphoreData;  // U32
        } All;
        DWORD       Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       _Unused                                     : BITFIELD_RANGE(0,1);
            DWORD       PointerBitFieldName                         : BITFIELD_RANGE(2,31);    // GTT[31:13]
        } All;
        DWORD       Value;
    } DW2;
};

static_assert(SIZE32(SMISemaphoreMBox) == 3);

/*****************************************************************************\
STRUCT: SMISetContext (MI_SET_CONTEXT)
\*****************************************************************************/
struct SMISetContext
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       RestoreInhibit                      : BITFIELD_BIT(0);          // bool
            DWORD       ForceRestore                        : BITFIELD_BIT(1);          // bool
            DWORD       ExtendedStateRestoreEnable          : BITFIELD_BIT(2);          // bool
            DWORD       ExtendedStateSaveEnable             : BITFIELD_BIT(3);          // bool
            DWORD       PhysicalStartAddressExtension       : BITFIELD_RANGE(4,7);      // PHYS[35:32]
            DWORD       MemorySpaceSelect                   : BITFIELD_BIT(8);          // MI_MEMORY_ADDRESS_TYPE
            DWORD       HDDVDContext                        : BITFIELD_BIT(9);          // MI_MEMORY_HD_DVD_CONTEXT
            DWORD       _Unused                             : BITFIELD_RANGE(10,11);
            DWORD       LogicalContextAddress               : BITFIELD_RANGE(12,31);    // GTT[31:11] | PHYS[31:11]
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMISetContext) == 2);


/*****************************************************************************\
STRUCT: SMIStoreDataImmediate (MI_STORE_DATA_IMM)
\*****************************************************************************/
struct SMIStoreDataImmediate
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,21);
            DWORD       UseGlobalGTT                        : BITFIELD_BIT(22);         // MI_MEMORY_USE_GLOBAL_GTT
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    union _QW1
    {
        // QWORD 1
        union _QW
        {
            struct _All
            {
                QWORD                                       : BITFIELD_RANGE(0,3);      //
                QWORD                                       : BITFIELD_RANGE(4,31);
                QWORD                                       : BITFIELD_RANGE(32,33);
                QWORD   Address                             : BITFIELD_RANGE(34,63);    // GTT[31:2] | PHYS[35:2]
            } All;
            QWORD   Value;
        } QW;

        // DWORD 1
        struct _DW1
        {
            DWORD   Value;
        } DW1;

        // DWORD 2
        struct _DW2
        {
            DWORD   Value;
        } DW2;
    } QW1;

    union _QW2
    {
        // QWORD 1
        union _QW
        {
            struct _All
            {
                QWORD       Data;
            } All;
            QWORD   Value;
        } QW;

        // DWORD 2
        struct _DW3
        {
            DWORD   Value;
        } DW3;

        // DWORD 3
        struct _DW4
        {
            DWORD   Value;
        } DW4;
    } QW2;
};

static_assert(SIZE32(SMIStoreDataImmediate) == 5);


/*****************************************************************************\
STRUCT: SMIStoreDataIndexed (MI_STORE_DATA_INDEX)
\*****************************************************************************/
struct SMIStoreDataIndexed
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused1                            : BITFIELD_RANGE(6,20);
            DWORD       UsePerProcessHardwareStatusPage     : BITFIELD_BIT(21);         // bool
            DWORD       _Unused2                            : BITFIELD_BIT(22);         //
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused1                            : BITFIELD_RANGE(0,1);
            DWORD       Offset                              : BITFIELD_RANGE(2,11);   // U10
            DWORD       _Unused2                            : BITFIELD_RANGE(12,31);
        } All;
        DWORD       Value;
    } DW1;

    union _QW2
    {
        // QWORD 1
        struct _All
        {
            QWORD       Data;
        } All;

        // DWORD 2
        struct _DW2
        {
            DWORD   Value;
        } DW2;

        // DWORD 3
        struct _DW3
        {
            DWORD   Value;
        } DW3;
    } QW2;
};

static_assert(SIZE32(SMIStoreDataIndexed) == 4);


/*****************************************************************************\
STRUCT: SMIStoreRegisterMemory (MI_STORE_REGISTER_MEM)
\*****************************************************************************/
struct SMIStoreRegisterMemory
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,21);
            DWORD       UseGlobalGTT                        : BITFIELD_BIT(22);         // MI_MEMORY_USE_GLOBAL_GTT
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused1                            : BITFIELD_RANGE(0,1);
            DWORD       RegisterAddress                     : BITFIELD_RANGE(2,25);     // OFFSET[25:2]
            DWORD       _Unused2                            : BITFIELD_RANGE(26,31);
        } All;
        DWORD       Value;
    } DW1;

    // DWORD 2
    union _DW2
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,1);
            DWORD       MemoryAddress                       : BITFIELD_RANGE(2,31);   // GTT[31:2]
        } All;
        DWORD       Value;
    } DW2;
};

static_assert(SIZE32(SMIStoreRegisterMemory) == 3);


/*****************************************************************************\
STRUCT: SMIUpdateGTTHeader (MI_UPDATE_GTT)
\*****************************************************************************/
struct SMIUpdateGTTHeader
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       Length                              : BITFIELD_RANGE(0,5);      // OP_LENGTH
            DWORD       _Unused                             : BITFIELD_RANGE(6,21);
            DWORD       EntryType                           : BITFIELD_BIT(22);         // MI_MEMORY_UPDATE_GTT_ENTRY
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;

    // DWORD 1
    union _DW1
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,11);
            DWORD       EntryAddress                        : BITFIELD_RANGE(12,31);  // GTT[31:2]
        } All;
        DWORD       Value;
    } DW1;
};

static_assert(SIZE32(SMIUpdateGTTHeader) == 2);

/*****************************************************************************\
STRUCT: SMIUpdateGTTState (MI_UPDATE_GTT)
\*****************************************************************************/
struct SMIUpdateGTTState
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       EntryData;  // PPGTT Table Entry
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIUpdateGTTState) == 1);

/*****************************************************************************\
STRUCT: SMIUpdateGTT (MI_UPDATE_GTT)
\*****************************************************************************/
struct SMIUpdateGTT
{
    SMIUpdateGTTHeader          Header;
    SMIUpdateGTTState           GTTState[g_cNumGTTUpdateEntries];
};

/*****************************************************************************\
STRUCT: SMIUnProbe (MI_UNPROBE)
\*****************************************************************************/
struct SMIUnProbe
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       SlotNumber                                  : BITFIELD_RANGE(0,9);      // ProbeSlotIndex
            DWORD       _Unused                                     : BITFIELD_RANGE(10,22);
            DWORD       InstructionOpcode                           : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                             : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};
static_assert(SIZE32(SMIUnProbe) == 1);

/*****************************************************************************\
STRUCT: SMIUserInterrupt (MI_USER_INTERRUPT)
\*****************************************************************************/
struct SMIUserInterrupt
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       _Unused                             : BITFIELD_RANGE(0,22);
            DWORD       InstructionOpcode                   : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                     : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIUserInterrupt) == 1);

/*****************************************************************************\
STRUCT: SMIWaitForEvent (MI_WAIT_FOR_EVENT)
\*****************************************************************************/
struct SMIWaitForEvent
{
    // DWORD 0
    union _DW0
    {
        struct _All
        {
            DWORD       WaitForBlitterEngine                    : BITFIELD_BIT(0);          // bool
            DWORD       WaitForVideoEngine                      : BITFIELD_BIT(1);          // bool
            DWORD       DisplayPlaneAFlipPendingWaitEnable      : BITFIELD_BIT(2);          // bool
            DWORD       DisplayPipeAVerticalBlankWaitEnable     : BITFIELD_BIT(3);          // bool
            DWORD       FrameBufferCompressionIdleWaitEnable    : BITFIELD_BIT(4);          // bool
            DWORD       DisplayPipeBScanLineWindowWaitEnable    : BITFIELD_BIT(5);          // bool
            DWORD       DisplayPlaneBFlipPendingWaitEnable      : BITFIELD_BIT(6);          // bool
            DWORD       DisplayPipeBVerticalBlankWaitEnable     : BITFIELD_BIT(7);          // bool
            DWORD       DisplaySpriteAFlipPendingWaitEnable     : BITFIELD_BIT(8);          // bool
            DWORD       ConditionCodeWaitSelect                 : BITFIELD_RANGE(9,12);     // MI_CONDITION_CODE_WAIT_SELECT
            DWORD       DisplayPipeAHBlankWaitEnable            : BITFIELD_BIT(13);         // bool
            DWORD       DisplayPipeBHBlankWaitEnable            : BITFIELD_BIT(14);         // bool
            DWORD       _Unused1                                : BITFIELD_BIT(15);
            DWORD       DisplaySpriteBFlipPendingWaitEnable     : BITFIELD_BIT(16);         // bool
            DWORD       DisplayPipeAStartVBlankWaitEnable       : BITFIELD_BIT(17);         // bool
            DWORD       DisplayPipeBStartVBlankWaitEnable       : BITFIELD_BIT(18);         // bool
            DWORD       _Unused2                                : BITFIELD_RANGE(19,22);
            DWORD       InstructionOpcode                       : BITFIELD_RANGE(23,28);    // MI_OPCODE
            DWORD       InstructionType                         : BITFIELD_RANGE(29,31);    // INSTRUCTION_TYPE
        } All;
        DWORD       Value;
    } DW0;
};

static_assert(SIZE32(SMIWaitForEvent) == 1);

} //namespace G6HWC

// Reset packing alignment to project default
#pragma pack()
