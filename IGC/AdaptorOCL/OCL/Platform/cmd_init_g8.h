/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum_g6.h"
#include "cmd_def_g6.h"

namespace G6HWC
{

/*****************************************************************************\
CONST: g_cInitGfxPipelineSelect
\*****************************************************************************/
const SGfxPipelineSelect g_cInitGfxPipelineSelect =
{
    // DWORD 0
    {
        GFXPIPELINE_3D,                                     // PipelineSelect
        GFXSUBOP_PIPELINE_SELECT,                           // InstructionSubOpcode
        GFXOP_NONPIPELINED,                                 // InstructionOpcode
        PIPE_SINGLE_DWORD,                                  // InstructionSubType
        INSTRUCTION_GFX                                     // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitGfxPipelineSelectGen9
\*****************************************************************************/
const SGfxPipelineSelect::_DW0::_Gen9 g_cInitGfxPipelineSelectGen9 =
{
    GFXPIPELINE_GPGPU,                                  // PipelineSelect
    GFXPIPELINE_SELECT_ENABLE,                          // PipelineSelectMask
    GFXSUBOP_PIPELINE_SELECT,                           // InstructionSubOpcode
    GFXOP_NONPIPELINED,                                 // InstructionOpcode
    PIPE_SINGLE_DWORD,                                  // InstructionSubType
    INSTRUCTION_GFX                                     // InstructionType
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddress
\*****************************************************************************/
const SGfxIndirectStateBaseAddress g_cInitGfxIndirectStateBaseAddress =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SGfxIndirectStateBaseAddress ) ),// Length
        GFXSUBOP_STATE_BASE_ADDRESS,                        // InstructionSubOpcode
        GFXOP_NONPIPELINED,                                 // InstructionOpcode
        PIPE_COMMON,                                        // InstructionSubType
        INSTRUCTION_GFX                                     // InstructionType
    },

    // DWORD 1
    {
        true,                                               // GeneralStateBaseAddressModify
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // GeneralStateCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // GeneralStateGraphicsDataType
        false,                                              // GeneralStateEncryptedDataEnable
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // StatelessDataPortCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // StatelessDataPortGraphicsDataType
        false,                                              // StatelessDataPortEncryptedDataEnable
        0                                                   // GeneralStateBaseAddress
    },

    // DWORD 2
    {
        true,                                               // SurfaceStateBaseAddressModify
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // SurfaceStateCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // SurfaceStateGraphicsDataType
        false,                                              // SurfaceStateEncryptedDataEnable
        0                                                   // SurfaceStateBaseAddress
    },

    // DWORD 3
    {
        true,                                               // DynamicStateBaseAddressModify
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // DynamicStateCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // DynamicStateGraphicsDataType
        false,                                              // DynamicStateEncryptedDataEnable
        0                                                   // DynamicStateBaseAddress
    },

    // DWORD 4
    {
        true,                                               // IndirectObjectBaseAddressModify
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // IndirectObjectCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // IndirectObjectGraphicsDataType
        false,                                              // IndirectObjectEncryptedDataEnable
        0                                                   // IndirectObjectBaseAddress
    },

    // DWORD 5
    {
        true,                                               // InstructionBaseAddressModify
        GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // InstructionMemoryCacheabilityControl
        GFXSTATE_GFDT_SOURCE_GTT,                           // InstructionMemoryGraphicsDataType
        false,                                              // InstructionMemoryEncryptedDataEnable
        0                                                   // InstructionBaseAddress
    },

    // DWORD 6
    {
        true,                                               // GeneralStateAccessUpperBoundModify
        0                                                   // GeneralStateAccessUpperBound
    },

    // DWORD 7
    {
        true,                                               // DynamicStateAccessUpperBoundModify
        0                                                   // DynamicStateAccessUpperBound
    },

    // DWORD 8
    {
        true,                                               // IndirectObjectStateAccessUpperBoundModify
        0                                                   // IndirectObjectStateAccessUpperBound
    },

    // DWORD 9
    {
        true,                                               // InstructionAccessUpperBoundModify
        0                                                   // InstructionAccessUpperBound
    }
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW4Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW4::_Gen8 g_cInitGfxIndirectStateBaseAddressDW4Gen8 =
{
    true,                                               // SurfaceStateBaseAddressModifyEnable
    GFXSTATE_SOURCE_AGE_CONTROL_POOR_HIT_CHANCE,        // SurfaceStateAgeControl
    false,                                              // SurfaceStateEncryptedDataEnable
    GFXSTATE_TARGET_CACHE_ELLC_ONLY,                    // SurfaceStateTargetCache
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,        // SurfaceStateCacheabilityControl
    0                                                   // SurfaceStateBaseAddress
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW6Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW6::_Gen8 g_cInitGfxIndirectStateBaseAddressDW6Gen8 =
{
    true,
    GFXSTATE_SOURCE_AGE_CONTROL_POOR_HIT_CHANCE,
    false,
    GFXSTATE_TARGET_CACHE_ELLC_ONLY,
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW8Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW8::_Gen8 g_cInitGfxIndirectStateBaseAddressDW8Gen8 =
{
    true,
    GFXSTATE_SOURCE_AGE_CONTROL_POOR_HIT_CHANCE,
    false,
    GFXSTATE_TARGET_CACHE_ELLC_ONLY,
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW10Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW10::_Gen8 g_cInitGfxIndirectStateBaseAddressDW10Gen8 =
{
    true,
    GFXSTATE_SOURCE_AGE_CONTROL_POOR_HIT_CHANCE,
    false,
    GFXSTATE_TARGET_CACHE_ELLC_ONLY,
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW12Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW12::_Gen8 g_cInitGfxIndirectStateBaseAddressDW12Gen8 =
{
    true,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW13Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW13::_Gen8 g_cInitGfxIndirectStateBaseAddressDW13Gen8 =
{
    true,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW14Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW14::_Gen8 g_cInitGfxIndirectStateBaseAddressDW14Gen8 =
{
    true,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxIndirectStateBaseAddressDW15Gen8
\*****************************************************************************/
const SGfxIndirectStateBaseAddress::_DW15::_Gen8 g_cInitGfxIndirectStateBaseAddressDW15Gen8 =
{
    true,
    0
};

/*****************************************************************************\
CONST: g_cInitGfxStatePointerInvalidate
\*****************************************************************************/
const SGfxStatePointerInvalidate g_cInitGfxStatePointerInvalidate =
{
    // DWORD 0
    {
        false,                                              // MediaStatePointerInvalidate
        false,                                              // ConstantBufferInvalidate
        false,                                              // PipelinedStatePointersInvalidate
        GFXSUBOP_STATE_POINTER_INVALIDATE,                  // InstructionSubOpcode
        GFXOP_PIPELINED,                                    // InstructionOpcode
        PIPE_SINGLE_DWORD,                                  // InstructionSubType
        INSTRUCTION_GFX                                     // InstructionType
    }
};

/*****************************************************************************\
CONST: g_cInitGfxStatePrefetch
\*****************************************************************************/
const SGfxStatePrefetch g_cInitGfxStatePrefetch =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SGfxStatePrefetch ) ),           // Length
        GFXSUBOP_STATE_PREFETCH,                            // InstructionSubOpcode
        GFXOP_PIPELINED,                                    // InstructionOpcode
        PIPE_COMMON,                                        // InstructionSubType
        INSTRUCTION_GFX                                     // InstructionType
    },

    // DWORD 1
    {
        0,                                                  // PrefetchCount
        0                                                   // PrefetchPointer
    }
};

/*****************************************************************************\
CONST: g_cInitGfxSystemInstructionPointer
\*****************************************************************************/
const SGfxSystemInstructionPointer g_cInitGfxSystemInstructionPointer =
{
    // DWORD 0
    {
        OP_LENGTH( SIZE32( SGfxSystemInstructionPointer ) ),// Length
        GFXSUBOP_STATE_SIP,                                 // InstructionSubOpcode
        GFXOP_NONPIPELINED,                                 // InstructionOpcode
        PIPE_COMMON,                                        // InstructionSubType
        INSTRUCTION_GFX                                     // InstructionType
    },

    // DWORD 1
    {
        0                                                   // SystemInstructionPointer
    }
};

} // namespace G6HWC
