/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum.h"

namespace G6HWC
{

/*****************************************************************************\
MACRO: BITFIELD_RANGE
PURPOSE: Calculates the number of bits between the startbit and the endbit (0 based)
\*****************************************************************************/
#ifndef BITFIELD_RANGE
#define BITFIELD_RANGE(startbit,endbit)     ((endbit)-(startbit)+1)
#endif

/*****************************************************************************\
MACRO: BITFIELD_BIT
PURPOSE: Definition declared for clarity when creating structs
\*****************************************************************************/
#ifndef BITFIELD_BIT
#define BITFIELD_BIT(bit)                   1
#endif

/*****************************************************************************\
ENUM: INSTRUCTION_TYPE
\*****************************************************************************/
enum INSTRUCTION_TYPE
{
    INSTRUCTION_MI      = 0x0,
    INSTRUCTION_TRUSTED = 0x1,
    INSTRUCTION_2D      = 0x2,
    INSTRUCTION_GFX     = 0x3
};

/*****************************************************************************\
ENUM: INSTRUCTION_SUBTYPE
\*****************************************************************************/
enum INSTRUCTION_SUBTYPE
{
    PIPE_COMMON       = 0x0,
    PIPE_SINGLE_DWORD = 0x1,
    PIPE_MEDIA        = 0x2,
    PIPE_3D           = 0x3
};

/*****************************************************************************\
ENUM: GFX_OPCODE
\*****************************************************************************/
enum GFX_OPCODE
{
    GFXOP_MEDIA         = 0x0,
    GFXOP_PIPELINED     = 0x0,
    GFXOP_NONPIPELINED  = 0x1,
};

/*****************************************************************************\
ENUM: GFX_PIPELINED_SUBOPCODE
\*****************************************************************************/
enum GFX_PIPELINED_SUBOPCODE
{
    GFXSUBOP_STATE_POINTER_INVALIDATE   = 0x2,
    GFXSUBOP_STATE_PREFETCH             = 0x3
};

/*****************************************************************************\
ENUM: GFX_NONPIPELINED_SUBOPCODE
\*****************************************************************************/
enum GFX_NONPIPELINED_SUBOPCODE
{
    GFXSUBOP_STATE_BASE_ADDRESS = 0x1,
    GFXSUBOP_STATE_SIP          = 0x2,
    GFXSUBOP_PIPELINE_SELECT    = 0x4
};

/*****************************************************************************\
ENUM: GFXPIPELINE_SELECT
\*****************************************************************************/
enum GFXPIPELINE_SELECT
{
    GFXPIPELINE_3D      = 0x0,
    GFXPIPELINE_MEDIA   = 0x1,
    GFXPIPELINE_GPGPU   = 0x2
};

/*****************************************************************************\
ENUM: GFXPIPELINE_SELECT_MASK
\*****************************************************************************/
enum GFXPIPELINE_SELECT_MASK
{
    GFXPIPELINE_SELECT_DISABLE   = 0x0,
    GFXPIPELINE_SELECT_ENABLE    = 0x3
};

/*****************************************************************************\
ENUM: GFXSTATE_GRAPHICS_DATATYPE_SOURCE
\*****************************************************************************/
enum GFXSTATE_GRAPHICS_DATATYPE_SOURCE
{
    GFXSTATE_GFDT_SOURCE_GTT      = 0x0,
    GFXSTATE_GFDT_SOURCE_SURFACE  = 0x1
};

/*****************************************************************************\
ENUM: GFXSTATE_CACHEABILITY_CONTROL
\*****************************************************************************/
enum GFXSTATE_CACHEABILITY_CONTROL
{
    GFXSTATE_CACHEABILITY_CONTROL_USE_GTT_ENTRY       = 0x0,
    GFXSTATE_CACHEABILITY_CONTROL_NEITHER_LLC_NOR_MLC = 0x1,
    GFXSTATE_CACHEABILITY_CONTROL_LLC_NOT_MLC         = 0x2,
    GFXSTATE_CACHEABILITY_CONTROL_LLC_AND_MLC         = 0x3
};

/*****************************************************************************\
ENUM: GFXSTATE_SOURCE_AGE_CONTROL
\*****************************************************************************/
enum GFXSTATE_SOURCE_AGE_CONTROL
{
    GFXSTATE_SOURCE_AGE_CONTROL_POOR_HIT_CHANCE       = 0x0,
    GFXSTATE_SOURCE_AGE_CONTROL_DECENT_HIT_CHANCE     = 0x1,
    GFXSTATE_SOURCE_AGE_CONTROL_GOOD_HIT_CHANCE       = 0x2,
    GFXSTATE_SOURCE_AGE_CONTROL_BEST_HIT_CHANCE       = 0x3
};

/*****************************************************************************\
ENUM: GFXSTATE_TARGET_CACHE
\*****************************************************************************/
enum GFXSTATE_TARGET_CACHE
{
    GFXSTATE_TARGET_CACHE_ELLC_ONLY                   = 0x0,
    GFXSTATE_TARGET_CACHE_LLC_ONLY                    = 0x1,
    GFXSTATE_TARGET_CACHE_LLC_AND_ELLC                = 0x2,
    GFXSTATE_TARGET_CACHE_L3_AND_LLC_AND_ELLC         = 0x3
};

} // namespace G6HWC
