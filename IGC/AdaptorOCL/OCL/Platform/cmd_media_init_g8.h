/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_media_enum_g8.h"
#include "cmd_media_def_g8.h"
#include "cmd_3d_enum_g8.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorData
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData g_cInitMediaStateInterfaceDescriptorData =
{
    // DW0
    {
        {
            0,                                              // _Unused
            0                                               // KernelStartPointer
        }
    },

    // DW1
    {
        {
            0,                                              // _Unused1
            false,                                          // SoftwareExceptionEnable
            0,                                              // _Unused2
            false,                                          // MaskStackExceptionEnable
            0,                                              // _Unused3
            false,                                          // IllegalOpcodeExceptionEnable
            0,                                              // _Unused4
            GFXMEDIASTATE_FLOATING_POINT_IEEE_754,          // FloatingPointMode
            GFXMEDIASTATE_THREAD_PRIORITY_NORMAL,           // ThreadPriority
            true,                                           // SingleProgramFlow
            0,                                              // _Unused5
            0,                                              // _Unused6
            0                                               // _Unused7
        }
    },

    // DW2
    {
        {
            0,                                              // _Unused
            0,                                              // SamplerCount
            0                                               // SamplerStatePointer
        }
    },

    // DW3
    {
        {
            0,                                              // BindingTableEntryCount
            0                                               // BindingTablePointer
        }
    },

    // DW4
    {
        {
            0,                                              // ConstantURBEntryReadOffset
            0                                               // ConstantURBEntryReadLength
        }
    },

    // DW5
    {
        {
            0,                                              // BarrierId
            0                                               // _Unused
        }
    },

    // DW6
    {
        {
            0,                                              // CrossThreadConstantDataReadLength
            0                                               // _Unused
        }
    },

    // DW7
    {
        {
            0,                                              // CrossThreadConstantDataReadLength
            0                                               // _Unused
        }
    }
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW5Gen7
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW5::_Gen7 g_cInitMediaStateInterfaceDescriptorDataDW5Gen7 =
{
    0,                                              // NumberOfThreadsInThreadGroup
    0,                                              // _Unused1
    0,                                              // SharedLocalMemorySize
    false,                                          // BarrierEnable
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEAREST_EVEN, // RoundingMode
    0                                               // _Unused2
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW1Gen8
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW1::_Gen8 g_cInitMediaStateInterfaceDescriptorDataDW1Gen8 =
{
    0,                                              // Kernel64bitStartingPointer
    0                                               // _Unused
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW2Gen8
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW2::_Gen8 g_cInitMediaStateInterfaceDescriptorDataDW2Gen8 =
{
    0,                                              // _Unused1
    false,                                          // SoftwareExceptionEnable
    0,                                              // _Unused2
    false,                                          // MaskStackExceptionEnable
    0,                                              // _Unused3
    false,                                          // IllegalOpcodeExceptionEnable
    0,                                              // _Unused4
    GFXMEDIASTATE_FLOATING_POINT_IEEE_754,          // FloatingPointMode
    GFXMEDIASTATE_THREAD_PRIORITY_NORMAL,           // ThreadPriority
    true,                                           // SingleProgramFlow
    0,                                              // DenormMode
    0                                               // _Unused5
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW3Gen8
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW3::_Gen8 g_cInitMediaStateInterfaceDescriptorDataDW3Gen8 =
{
    0,                                              // _Unused
    0,                                              // SamplerCount
    0                                               // SamplerStatePointer
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW4Gen8
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW4::_Gen8 g_cInitMediaStateInterfaceDescriptorDataDW4Gen8 =
{
    0,                                              // BindingTableEntryCount
    0,                                              // BindingTablePointer
    0                                               // _Unused
};

/*****************************************************************************\
CONST: g_cInitMediaStateInterfaceDescriptorDataDW5Gen8
\*****************************************************************************/
static const SMediaStateInterfaceDescriptorData::_DW5::_Gen8 g_cInitMediaStateInterfaceDescriptorDataDW5Gen8 =
{
    0,                                              // ConstantURBEntryReadOffset
    0                                               // ConstantURBEntryReadLength
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaInterfaceDescriptorLoad
\*****************************************************************************/
static const SMediaStateMediaInterfaceDescriptorLoad g_cInitMediaStateMediaInterfaceDescriptorLoad =
{
    // DW0
    {
        {
            OP_LENGTH( SIZE32( SMediaStateMediaInterfaceDescriptorLoad ) ),  // Length
            GFXSUBOP_MEDIA_INTERFACE_DESCRIPTOR_LOAD,                        // InstructionSubOpcode
            GFXOP_MEDIA,                                                     // InstructionOpcode
            PIPE_MEDIA,                                                      // InstructionSubType
            INSTRUCTION_GFX                                                  // InstructionType
        }
    },

    // DW1
    {
        0                                               // Reserved
    },

    // DW2
    {
        {
            0,                                              // InterfaceDescriptorTotalLength
            0                                               // _Unused
        }
    },

    // DW3
    {
        {
            0                                               // InterfaceDescriptorDataStartAddress
        }
    }

};


/*****************************************************************************\
CONST: g_cInitMediaStateMediaVFEState
\*****************************************************************************/
static const SMediaStateMediaVFEState g_cInitMediaStateMediaVFEState =
{
    // DW0
    {
        {
            OP_LENGTH( SIZE32( SMediaStateMediaVFEState ) ),// Length
            GFXSUBOP_MEDIA_VFE_STATE,                       // InstructionSubOpcode
            GFXOP_MEDIA,                                    // InstructionOpcode
            PIPE_MEDIA,                                     // InstructionSubType
            INSTRUCTION_GFX                                 // InstructionType
        }
    },

    // DW1
    {
        {
            0,                                          // PerThreadScratchSpace
            0,                                          // StackSize
            0,                                          // _Unused
            0                                           // ScratchSpaceBasePointer
        }
    },

    // DW2
    {
        {
            GFXMEDIASTATE_DEBUG_COUNTER_FREE_RUNNING,   // DebugCounterControl
            0,                                          // _Unused
            false,                                      // FastPreempt
            false,                                      // BypassGatewayControl
            false,                                      // ResetGatewayTimer
            0,                                          // NumberOfURBEntries
            0                                           // MaximumNumberOfThreads
        }
    },

    // DW3
    {
        {
            0,                                          // _Unused
            0                                           // ObjectId
        }
    },

    // DW4
    {
        {
            0,                                          // CURBEAllocationSize
            0                                           // URBEntryAllocationSize
        }
    },

    // DW5
    {
        {
            0,                                          // ScoreboardMask
            0,                                          // _Unused
            GFXMEDIASTATE_STALLING_SCOREBOARD,          // ScoreboardType
            false                                       // ScoreboardEnable
        }
    },

    // DW6
    {
        {
            0,                                          // ScoreboardDeltaX0
            0,                                          // ScoreboardDeltaY0
            0,                                          // ScoreboardDeltaX1
            0,                                          // ScoreboardDeltaY1
            0,                                          // ScoreboardDeltaX2
            0,                                          // ScoreboardDeltaY2
            0,                                          // ScoreboardDeltaX3
            0                                           // ScoreboardDeltaY3
        }
    },

    // DW7
    {
        {
            0,                                          // ScoreboardDeltaX4
            0,                                          // ScoreboardDeltaY4
            0,                                          // ScoreboardDeltaX5
            0,                                          // ScoreboardDeltaY5
            0,                                          // ScoreboardDeltaX6
            0,                                          // ScoreboardDeltaY6
            0,                                          // ScoreboardDeltaX7
            0                                           // ScoreboardDeltaY7
        }
    },

    // DW8
    {
        {
            0,                                          // ScoreboardDeltaX4
            0,                                          // ScoreboardDeltaY4
            0,                                          // ScoreboardDeltaX5
            0,                                          // ScoreboardDeltaY5
            0,                                          // ScoreboardDeltaX6
            0,                                          // ScoreboardDeltaY6
            0,                                          // ScoreboardDeltaX7
            0                                           // ScoreboardDeltaY7
        }
    }
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaVFEStateDW2Gen7
\*****************************************************************************/
static const SMediaStateMediaVFEState::_DW2::_Gen7 g_cInitMediaStateMediaVFEStateDW2Gen7 =
{
    GFXMEDIASTATE_DEBUG_COUNTER_FREE_RUNNING,   // DebugCounterControl
    GFXMEDIASTATE_GPGPU_MODE_GPGPU,             // GPGPUMode
    GFXMEDIASTATE_MMIO_ACCESS_CONTROL_ANY_READWRITE, // GatewayMMIOAccessControl
    false,                                      // FastPreempt
    false,                                      // BypassGatewayControl
    false,                                      // ResetGatewayTimer
    0,                                          // NumberOfURBEntries
    0                                           // MaximumNumberOfThreads
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaVFEStateDW3Gen9
\*****************************************************************************/
static const SMediaStateMediaVFEState::_DW3::_Gen9 g_cInitMediaStateMediaVFEStateDW3Gen9 =
{
    GFXMEDIASTATE_DEBUG_COUNTER_FREE_RUNNING,           // DebugCounterControl
    GFXMEDIASTATE_SLM_GRANULARITY_4K,                   // GFXMEDIASTATE_SLM_GRANULARITY
    GFXMEDIASTATE_MMIO_ACCESS_CONTROL_ANY_READWRITE,    // GatewayMMIOAccessControl
    0,                                                  // _Unused
    false,                                              // BypassGatewayControl
    false,                                              // ResetGatewayTimer
    0,                                                  // NumberOfURBEntries
    0                                                   // MaximumNumberOfThreads
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaCURBELoad
\*****************************************************************************/
static const SMediaStateMediaCURBELoad g_cInitMediaStateMediaCURBELoad =
{
    // DW0
    {
        {
            OP_LENGTH( SIZE32( SMediaStateMediaCURBELoad ) ),           // Length
            GFXSUBOP_MEDIA_CURBE_LOAD,                                  // InstructionSubOpcode
            GFXOP_MEDIA,                                                // InstructionOpcode
            PIPE_MEDIA,                                                 // InstructionSubType
            INSTRUCTION_GFX                                             // InstructionType
        }
    },

    // DW1
    {
        0
    },

    // DW2
    {
        {
            0,                                                          // CURBETotalDataLength
            0                                                           // _Unused
        }
    },

    // DW3
    {
        {
            0                                                           // CURBEDataStartAddress
        }
    },
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaStateFlush
\*****************************************************************************/
static const SMediaStateMediaStateFlush g_cInitMediaStateMediaStateFlush =
{
    // DW0
    {
        {
            OP_LENGTH( SIZE32( SMediaStateMediaStateFlush ) ),          // Length
            GFXSUBOP_MEDIA_STATE_FLUSH,                                 // InstructionSubOpcode
            GFXOP_MEDIA,                                                // InstructionOpcode
            PIPE_MEDIA,                                                 // InstructionSubType
            INSTRUCTION_GFX                                             // InstructionType
        }
    },

    // DW1
    {
        {
            0,                                                          // BarrierMask
            0,                                                          // ThreadCountWaterMark
            0                                                           // _Unused
        }
    }
};

/*****************************************************************************\
CONST: g_cInitMediaStateMediaStateFlushDW1Gen7
\*****************************************************************************/
static const SMediaStateMediaStateFlush::_DW1::_Gen7 g_cInitMediaStateMediaStateFlushDW1Gen7 =
{
    0,                                                          // InterfaceDescriptorOffset
    false,                                                      // ThreadCountWaterMark
    0,                                                          // _Unused
};


/*****************************************************************************\
CONST: g_cInitMediaStateGPGPUWalker
\*****************************************************************************/
static const SMediaStateGPGPUWalker g_cInitMediaStateGPGPUWalker =
{
    // DW0
    {
        {
            OP_LENGTH( SIZE32( SMediaStateGPGPUWalker ) ),              // Length
            false,                                                      // PredicateEnable
            0,                                                          // _Unused1
            false,                                                      // IndirectParameterEnable
            0,                                                          // _Unused2
            GFXSUBOP_MEDIA_GPGPU_WALKER,                                // InstructionSubOpcode
            GFXOP_NONPIPELINED,                                         // InstructionOpcode
            PIPE_MEDIA,                                                 // InstructionSubType
            INSTRUCTION_GFX                                             // InstructionType
        }
    },

    // DW1
    {
        {
            0,                                                          // InterfaceDescriptorOffset
            0,                                                          // _Unused
            0                                                           // ObjectId
        }
    },

    // DW2
    {
        {
            0,                                                          // ThreadWidthCounterMaximum
            0,                                                          // _Unused1
            0,                                                          // ThreadHeightCounterMaximum
            0,                                                          // _Unused2
            0,                                                          // ThreadDepthCounterMaximum
            0,                                                          // _Unused3
            GFXMEDIASTATE_GPGPU_WALKER_SIMD16                                                           // SIMDSize
        }
    },

    // DW3
    {
        {
            0                                                           // ThreadGroupIdStartingX
        }
    },

    // DW4
    {
        {
            0                                                           // ThreadGroupIdDimensionX
        }
    },

    // DW5
    {
        {
            0                                                           // ThreadGroupIdStartingY
        }
    },

    // DW6
    {
        {
            0                                                           // ThreadGroupIdDimensionY
        }
    },

    // DW7
    {
        {
            0                                                           // ThreadGroupIdStartingZ
        }
    },

    // DW8
    {
        {
            0                                                           // ThreadGroupIdDimensionZ
        }
    },

    // DW9
    {
        {
            0                                                           // RightExecutionMask
        }
    },

    // DW10
    {
        {
            0                                                           // BottomExecutionMask
        }
    },

    // DW11
    {
        {
            0                                                           // ThreadGroupIdStartingResumeZ
        }
    },

    // DW12
    {
        {
            0                                                           // ThreadGroupIdDimensionZ
        }
    },

    // DW13
    {
        {
            0                                                           // RightExecutionMask
        }
    },

    // DW14
    {
        {
            0                                                           // BottomExecutionMask
        }
    }
};
} // namespace G6HWC
