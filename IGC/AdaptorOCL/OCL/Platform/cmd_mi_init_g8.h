/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_mi_enum_g6.h"
#include "cmd_mi_def_g6.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: g_cInitMIARBCheck
\*****************************************************************************/
const SMIARBCheck g_cInitMIARBCheck =
{
    // DWORD 0
    {
        0,                                                  // _Unused
        MI_ARB_CHECK,                                       // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIArbOnOff
\*****************************************************************************/
const SMIARBOnOff g_cInitMIArbOnOff =
{
    // DWORD 0
    {
        false,                                              // ArbitrationEnable
        0,                                                  // _Unused
        MI_ARB_ON_OFF,                                      // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIBatchBufferEnd
\*****************************************************************************/
const SMIBatchBufferEnd g_cInitMIBatchBufferEnd =
{
    // DWORD 0
    {
        0,                                                  // _Unused
        MI_BATCH_BUFFER_END,                                // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIBatchBufferStart
\*****************************************************************************/
const SMIBatchBufferStart g_cInitMIBatchBufferStart =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIBatchBufferStart) ),          // Length
        0,                                                  // _Unused1
        MI_BUFFER_SECURE,                                   // BufferSecurityIndicator
        0,                                                  // _Unused2
        //MI_NO_ARBITRATION,                                  // CommandArbitrationControl
        //false,                                              // ClearCommandBufferEnable
        //0,                                                  // _Unused3
        MI_BATCH_BUFFER_START,                              // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // _Unused
        0                                                   // BufferStartAddress
    }
};

/*****************************************************************************\
CONST: g_cInitMIDisplayFlip
\*****************************************************************************/
const SMIDisplayFlip g_cInitMIDisplayFlip =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIDisplayFlip ) ),              // Length
        0,                                                  // _Unused
        MI_DISPLAY_PLANE_A,                                 // DisplayPlaneSelect
        MI_SYNCHRONOUS_FLIP,                                // AsynchronousFlip
        MI_DISPLAY_FLIP,                                    // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // _Unused1
        0,                                                  // DisplayBufferPitch
        0,                                                  // _Unused2
        MI_STANDARD_FLIP,                                   // FlipQueueSelect
        0                                                   // _Unused3
    },

    // DWORD 2
    {
        MI_TILE_LINEAR,                                     // TileParameter
        0,                                                  // _Unused
        0                                                   // DisplayBufferBaseAddress
    },

    // DWORD 3
    {
        0,                                                  // PipeVerticalSourceImageResize
        0,                                                  // _Unused1
        0,                                                  // PipeHorizontalSourceImageSize
        0,                                                  // _Unused2
        MI_PANEL_7X5_CAPABLE,                               // PanelFitterSelect
        false,                                              // EnablePanelFitter
    }
};

/*****************************************************************************\
CONST: g_cInitMIFlush
\*****************************************************************************/
const SMIFlush g_cInitMIFlush =
{
    // DWORD 0
    {
        0,                                                  // _Unused1
        false,                                              // StateCacheInvalidate
        true,                                               // RenderCacheFlushInhibit
        false,                                              // GlobalSnapshotCountReset
        false,                                              // MediaPersistentRootThreadComplete
        false,                                              // IndirectStatePointersDisable
        true,                                               // ProtectedMemory
        0,                                                  // _Unused2
        MI_FLUSH,                                           // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMILoadRegisterImmediate
\*****************************************************************************/
const SMILoadRegisterImmediate g_cInitMILoadRegisterImmediate =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMILoadRegisterImmediate ) ),    // Length
        0,                                                  // _Unused1
        false,                                              // Byte0WriteDisable
        false,                                              // Byte1WriteDisable
        false,                                              // Byte2WriteDisable
        false,                                              // Byte3WriteDisable
        0,                                                  // _Unused2
        MI_LOAD_REGISTER_IMM,                               // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // _Unused
        0                                                   // RegisterOffset
    },

    // DWORD 2
    {
        0                                                   // DataDWord
    }
};

/*****************************************************************************\
CONST: g_cInitMILoadScanLinesExcl
\*****************************************************************************/
const SMILoadScanLinesExclusive g_cInitMILoadScanLinesExcl =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMILoadScanLinesExclusive ) ),   // Length
        0,                                                  // _Unused1
        MI_DISPLAY_PIPE_A,                                  // DisplayPipeSelect
        0,                                                  // _Unused2
        MI_LOAD_SCAN_LINES_EXCL,                            // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // EndScanLineNumber
        0                                                   // StartScanLineNumber
    }
};

/*****************************************************************************\
CONST: g_cInitMILoadScanLinesIncl
\*****************************************************************************/
const SMILoadScanLinesInclusive g_cInitMILoadScanLinesIncl =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMILoadScanLinesInclusive ) ),   // Length
        0,                                                  // _Unused1
        MI_DISPLAY_PIPE_A,                                  // DisplayPipeSelect
        0,                                                  // _Unused2
        MI_LOAD_SCAN_LINES_INCL,                            // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // EndScanLineNumber
        0                                                   // StartScanLineNumber
    }
};

/*****************************************************************************\
CONST: g_cInitMINoop
\*****************************************************************************/
const SMINoop g_cInitMINoop =
{
    // DWORD 0
    {
        0,                                                  // IdentificationNumber
        false,                                              // IdentificationNumberRegisterWriteEnable
        MI_NOOP,                                            // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitOverlayFlip
\*****************************************************************************/
const SMIOverlayFlip g_cInitOverlayFlip =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIOverlayFlip ) ),              // Length
        0,                                                  // _Unused
        MI_FLIP_CONTINUE,                                   // ModeFlags
        MI_OVERLAY_FLIP,                                    // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        false,                                              // OverlayFilterCoefficientRegisterUpdateFlag
        0,                                                  // _Unused
        0                                                   // RegisterAndCoefficientUpdateAddress
    }
};

/*****************************************************************************\
CONST: g_cInitMIProbeHeader
\*****************************************************************************/
const SMIProbeHeader g_cInitMIProbeHeader =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIProbeHeader ) ),              // Length               //TODO TODO: ?ok? length = smiprobeheader + smiprobestate * num probes?
        0,                                                  // _Unused
        MI_PROBE,                                           // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIReportHead
\*****************************************************************************/
const SMIReportHead g_cInitMIReportHead =
{
    // DWORD 0
    {
        0,                                                  // _Unused
        MI_REPORT_HEAD,                                     // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIReportNonCE
\*****************************************************************************/
const SMIReportNonCE g_cInitMIReportNonCE =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIReportNonCE ) ),              // Length
        0,                                                  // _Unused
        MI_REPORT_NONCE,                                    // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0                                                   // NonCEValue
    }
};

/*****************************************************************************\
CONST: g_cInitMISemaphoreMBox
\*****************************************************************************/
const SMISemaphoreMBox g_cInitMISemaphoreMBox =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMISemaphoreMBox ) ),            // Length
        0,                                                  // _Unused
        false,                                              // CommandBufferTerminateEnable
        false,                                              // CompareSemaphore
        false,                                              // UpdateSemaphore
        MI_MEMORY_PER_PROCESS_GRAPHICS_ADDRESS,             // UseGlobalGTT
        MI_SEMAPHORE_MBOX,                                  // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0                                                   // SemaphoreData
    },

    // DWORD 2
    {
        0,                                                  // _Unused
        0                                                   // PointerBitFieldName
    }
};

/*****************************************************************************\
CONST: g_cInitMISetContext
\*****************************************************************************/
const SMISetContext g_cInitMISetContext =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMISetContext ) ),               // Length
        0,                                                  // _Unused
        MI_SET_CONTEXT,                                     // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        false,                                              // RestoreInhibit
        false,                                              // ForceRestore
        false,                                              // ExtendedStateRestoreEnable
        false,                                              // ExtendedStateSaveEnable
        0,                                                  // PhysicalStartAddressExtension
        MI_PHYSICAL_ADDRESS,                                // MemorySpaceSelect
        MI_REGULAR_CONTEXT,                                 // HDDVDContext
        0,                                                  // _Unused
        0                                                   // LogicalContextAddress
    }
};

/*****************************************************************************\
CONST: g_cInitMIStoreDataImm
\*****************************************************************************/
const SMIStoreDataImmediate g_cInitMIStoreDataImm =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIStoreDataImmediate ) ),       // Length
        0,                                                  // _Unused
        MI_VIRTUAL_ADDRESS,                                 // MemoryAddressType
        MI_STORE_DATA_IMM,                                  // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // QWORD 1
    {
        0                                                   // Address
    },

    // QWORD 2
    {
        0                                                   // Data
    }
};

/*****************************************************************************\
CONST: g_cInitMIStoreDataIndex
\*****************************************************************************/
const SMIStoreDataIndexed g_cInitMIStoreDataIndex =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIStoreDataIndexed ) ),         // Length
        0,                                                  // _Unused1
        false,                                              // UsePerProcessHardwareStatusPage
        0,                                                  // _Unused2
        MI_STORE_DATA_INDEX,                                // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // _Unused1
        0,                                                  // Offset
        0                                                   // _Unused2
    },

    // QWORD 1
    {
        0                                                   // Data
    }
};

/*****************************************************************************\
CONST: g_cInitMIStoreRegisterMem
\*****************************************************************************/
const SMIStoreRegisterMemory g_cInitMIStoreRegisterMem =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIStoreRegisterMemory ) ),    // Length
        0,                                                  // _Unused
        MI_MEMORY_PER_PROCESS_GRAPHICS_ADDRESS,             // UseGlobalGTT
        MI_STORE_REGISTER_MEM,                              // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // _Unused1
        0,                                                  // RegisterAddress
        0                                                   // _Unused2
    },

    // DWORD 2
    {
        0,                                                  // _Unused
        0                                                   // MemoryAddress
    }
};

/*****************************************************************************\
CONST: g_cInitMIUpdateGTTHeader
\*****************************************************************************/
const SMIUpdateGTTHeader g_cInitMIUpdateGTTHeader =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SMIUpdateGTTHeader )
                 + SIZE32(SMIUpdateGTTState)
                 * g_cNumGTTUpdateEntries),                 // Length
        0,                                                  // _Unused
        MI_MEMORY_GGTT_ENTRY_UPDATE,                        // EntryType
        MI_UPDATE_GTT,                                      // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    },

    //DWORD 1
    {
        0,                                                  // _Unused
        0                                                   // EntryAddress
    }
};

/*****************************************************************************\
CONST: g_cInitMIUnProbe
\*****************************************************************************/
const SMIUnProbe g_cInitMIUnProbe =
{
    // DWORD 0
    {
        0,                                                  // SlotNumber
        0,                                                  // _Unused
        MI_UNPROBE,                                         // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIUserInterrupt
\*****************************************************************************/
const SMIUserInterrupt g_cInitMIUserInterrupt =
{
    // DWORD 0
    {
        0,                                                  // _Unused
        MI_USER_INTERRUPT,                                  // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitMIWaitForEvent
\*****************************************************************************/
const SMIWaitForEvent g_cInitMIWaitForEvent =
{
    // DWORD 0
    {
        false,                                              // WaitForBlitterEngine
        false,                                              // WaitForVideoEngine
        false,                                              // DisplayPlaneAFlipPendingWaitEnable
        false,                                              // DisplayPipeAVerticalBlankWaitEnable
        false,                                              // FrameBufferCompressionIdleWaitEnable
        false,                                              // DisplayPipeBScanLineWindowWaitEnable
        false,                                              // DisplayPlaneBFlipPendingWaitEnable
        false,                                              // DisplayPipeBVerticalBlankWaitEnable
        false,                                              // DisplaySpriteAFlipPendingWaitEnable
        MI_CONDITION_CODE_WAIT_DISABLED,                    // ConditionCodeWaitSelect
        false,                                              // DisplayPipeBHBlankWaitEnable
        false,                                              // DisplayPipeAHBlankWaitEnable
        false,                                              // DisplayPipeBHBlankWaitEnable
        0,                                                  // _Unused1
        false,                                              // DisplaySpriteBFlipPendingWaitEnable
        0,                                                  // _Unused2
        false,                                              // DisplayPipeBStartVBlankWaitEnable
        MI_WAIT_FOR_EVENT,                                  // InstructionOpcode
        INSTRUCTION_MI                                      // InstructionType
    }
};


} // namespace G6HWC
