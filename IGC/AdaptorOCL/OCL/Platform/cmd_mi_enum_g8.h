/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum_g6.h"

namespace G6HWC
{
/*****************************************************************************\
CONST: Caps
\*****************************************************************************/
const DWORD g_cNumProbes            = 1024;
const DWORD g_cNumGTTUpdateEntries  = 16;

/*****************************************************************************\
ENUM: MI_OPCODE
\*****************************************************************************/
enum MI_OPCODE
{
    MI_NOOP                     = 0x00,
    MI_USER_INTERRUPT           = 0x02,
    MI_WAIT_FOR_EVENT           = 0x03,
    MI_FLUSH                    = 0x04,
    MI_ARB_CHECK                = 0x05,
    MI_UNPROBE                  = 0x06,
    MI_REPORT_HEAD              = 0x07,
    MI_ARB_ON_OFF               = 0x08,
    MI_URB_WRITE                = 0x09,
    MI_BATCH_BUFFER_END         = 0x0A,
    MI_PREDICATE                = 0x0C,
    MI_TOPOLOGY_FILTER          = 0x0D,
    MI_OVERLAY_FLIP             = 0x11,
    MI_LOAD_SCAN_LINES_INCL     = 0x12,
    MI_LOAD_SCAN_LINES_EXCL     = 0x13,
    MI_DISPLAY_FLIP             = 0x14,
    MI_SEMAPHORE_MBOX           = 0x16,
    MI_SET_CONTEXT              = 0x18,
    MI_REPORT_NONCE             = 0x19,
    MI_STORE_DATA_IMM           = 0x20,
    MI_STORE_DATA_INDEX         = 0x21,
    MI_LOAD_REGISTER_IMM        = 0x22,
    MI_UPDATE_GTT               = 0x23,
    MI_STORE_REGISTER_MEM       = 0x24,
    MI_PROBE                    = 0x25,
    MI_LOAD_REGISTER_MEM        = 0x29,
    MI_BATCH_BUFFER_START       = 0x31
};

/*****************************************************************************\
ENUM: MI_ASYNCHRONOUS_FLIP
\*****************************************************************************/
enum MI_ASYNCHRONOUS_FLIP
{
    MI_SYNCHRONOUS_FLIP                 = 0x0,
    MI_ASYNCHRONOUS_FLIP                = 0x1
};

/*****************************************************************************\
ENUM: MI_BUFFER_SECURITY_INDICATOR
\*****************************************************************************/
enum MI_BUFFER_SECURITY_INDICATOR
{
    MI_BUFFER_SECURE                    = 0x0,
    MI_BUFFER_NONSECURE                 = 0x1
};

/*****************************************************************************\
ENUM: MI_COMMAND_ARBITRATION_CONTROL
\*****************************************************************************/
enum MI_COMMAND_ARBITRATION_CONTROL
{
    MI_ARBITRATE_AT_CHAIN_POINTS        = 0x0,
    MI_ARBITRATE_BETWEEN_INSTS          = 0x1,
    MI_NO_ARBITRATION                   = 0x3
};

/*****************************************************************************\
ENUM: MI_CONDITION_CODE_WAIT_SELECT
\*****************************************************************************/
enum MI_CONDITION_CODE_WAIT_SELECT
{
    MI_CONDITION_CODE_WAIT_DISABLED     = 0x0,
    MI_CONDITION_CODE_WAIT_0            = 0x1,
    MI_CONDITION_CODE_WAIT_1            = 0x2,
    MI_CONDITION_CODE_WAIT_2            = 0x3,
    MI_CONDITION_CODE_WAIT_3            = 0x4,
    MI_CONDITION_CODE_WAIT_4            = 0x5
};

/*****************************************************************************\
ENUM: MI_DISPLAY_PIPE_SELECT
\*****************************************************************************/
enum MI_DISPLAY_PIPE_SELECT
{
    MI_DISPLAY_PIPE_A                               = 0x0,
    MI_DISPLAY_PIPE_B                               = 0x1
};

/*****************************************************************************\
ENUM: MI_DISPLAY_PLANE_SELECT
\*****************************************************************************/
enum MI_DISPLAY_PLANE_SELECT
{
    MI_DISPLAY_PLANE_A                  = 0x0,
    MI_DISPLAY_PLANE_B                  = 0x1,
    MI_DISPLAY_PLANE_C                  = 0x2
};

/*****************************************************************************\
ENUM: MI_FLIP_QUEUE_SELECT
\*****************************************************************************/
enum MI_FLIP_QUEUE_SELECT
{
    MI_STANDARD_FLIP                                = 0x0,
    MI_ENQUEUE_FLIP_PERFORM_BASE_FRAME_NUMBER_LOAD  = 0x1,
    MI_ENQUEUE_FLIP_TARGET_FRAME_NUMBER_RELATIVE    = 0x2,
    MI_ENQUEUE_FLIP_ABSOLUTE_TARGET_FRAME_NUMBER    = 0x3
};

/*****************************************************************************\
ENUM: MI_MEMORY_HD_DVD_CONTEXT
\*****************************************************************************/
enum MI_MEMORY_HD_DVD_CONTEXT
{
    MI_REGULAR_CONTEXT  = 0x0,
    MI_HD_DVD_CONTEXT   = 0x1
};

/*****************************************************************************\
ENUM: MI_MEMORY_ADDRESS_TYPE
\*****************************************************************************/
enum MI_MEMORY_ADDRESS_TYPE
{
    MI_PHYSICAL_ADDRESS                 = 0x0,
    MI_VIRTUAL_ADDRESS                  = 0x1
};

/*****************************************************************************\
ENUM: MI_MEMORY_SPACE_SELECT
\*****************************************************************************/
enum MI_MEMORY_SPACE_SELECT
{
    MI_BUFFER_MEMORY_MAIN               = 0x0,
    MI_BUFFER_MEMORY_GTT                = 0x2,
    MI_BUFFER_MEMORY_PER_PROCESS_GTT    = 0x3
};

/*****************************************************************************\
ENUM: MI_MEMORY_UPDATE_GTT
\*****************************************************************************/
enum MI_MEMORY_UPDATE_GTT_ENTRY
{
    MI_MEMORY_GGTT_ENTRY_UPDATE   = 0x0,
    MI_MEMORY_PGTT_ENTRY_UPDATE   = 0x1
};

/*****************************************************************************\
ENUM: MI_MEMORY_USE_GLOBAL_GTT
\*****************************************************************************/
enum MI_MEMORY_USE_GLOBAL_GTT
{
    MI_MEMORY_PER_PROCESS_GRAPHICS_ADDRESS  = 0x0,
    MI_MEMORY_GLOBAL_GRAPHICS_ADDRESS       = 0x1
};

/*****************************************************************************\
ENUM: MI_MODE_FLAGS
\*****************************************************************************/
enum MI_MODE_FLAGS
{
    MI_FLIP_CONTINUE                 = 0x0,
    MI_FLIP_ON                       = 0x1,
    MI_FLIP_OFF                      = 0x2
};

/*****************************************************************************\
ENUM: MI_PANEL_FITTER
\*****************************************************************************/
enum MI_PANEL_FITTER
{
    MI_PANEL_7X5_CAPABLE            = 0x0,
    MI_PANEL_3X3_CAPABLE            = 0x1
};

/*****************************************************************************\
ENUM: MI_REGISTER_SPACE_SELECT
\*****************************************************************************/
enum MI_REGISTER_SPACE_SELECT
{
    MI_UNTRUSTED_REGISTER_SPACE                     = 0x0,
    MI_TRUSTED_REGISTER_SPACE                       = 0x1
};


/*****************************************************************************\
ENUM: MI_TILE_PARAMETER
\*****************************************************************************/
enum MI_TILE_PARAMETER
{
    MI_TILE_LINEAR                  = 0x0,
    MI_TILE_TILEDX                  = 0x1
};

/*****************************************************************************\
ENUM: MI_PREDICATE_COMPAREOP
\*****************************************************************************/
enum MI_PREDICATE_COMPAREOP
{
    MI_PREDICATE_COMPAREOP_TRUE         = 0x0,
    MI_PREDICATE_COMPAREOP_FALSE        = 0x1,
    MI_PREDICATE_COMPAREOP_SRCS_EQUAL   = 0x2,
    MI_PREDICATE_COMPAREOP_DELTAS_EQUAL = 0x3
};

/*****************************************************************************\
ENUM: MI_PREDICATE_COMBINEOP
\*****************************************************************************/
enum MI_PREDICATE_COMBINEOP
{
    MI_PREDICATE_COMBINEOP_SET  = 0x0,
    MI_PREDICATE_COMBINEOP_AND  = 0x1,
    MI_PREDICATE_COMBINEOP_OR   = 0x2,
    MI_PREDICATE_COMBINEOP_XOR  = 0x3
};

/*****************************************************************************\
ENUM: MI_PREDICATE_LOADOP
\*****************************************************************************/
enum MI_PREDICATE_LOADOP
{
    MI_PREDICATE_LOADOP_KEEP    = 0x0,
    MI_PREDICATE_LOADOP_LOAD    = 0x2,
    MI_PREDICATE_LOADOP_LOADINV = 0x3
};

} //namespace G6HWC
