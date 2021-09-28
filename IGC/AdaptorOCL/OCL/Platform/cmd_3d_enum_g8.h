/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum_g8.h"

namespace G6HWC
{

/*****************************************************************************\
ENUM: GFX3D_OPCODE
\*****************************************************************************/
enum GFX3D_OPCODE
{
    GFX3DOP_3DSTATE_PIPELINED       = 0x0,
    GFX3DOP_3DSTATE_NONPIPELINED    = 0x1,
    GFX3DOP_PIPECONTROL             = 0x2,
    GFX3DOP_3DPRIMITIVE             = 0x3
};

/*****************************************************************************\
ENUM: GFX3DCONTROL_GTTWRITE_MODE
\*****************************************************************************/
enum GFX3DCONTROL_GTTWRITE_MODE
{
    GFX3DCONTROL_GTTWRITE_PROCESS_LOCAL = 0x00,
    GFX3DCONTROL_GTTWRITE_GLOBAL        = 0x01
};

/*****************************************************************************\
ENUM: GFX3DCONTROL_SUBOPCODE
\*****************************************************************************/
enum GFX3DCONTROL_SUBOPCODE
{
    GFX3DSUBOP_3DCONTROL    = 0x00
};

/*****************************************************************************\
ENUM: GFX3DCONTROL_OPERATION
\*****************************************************************************/
enum GFX3DCONTROL_OPERATION
{
    GFX3DCONTROLOP_NOWRITE          = 0x00,
    GFX3DCONTROLOP_WRITEIMMEDIATE   = 0x01,
    GFX3DCONTROLOP_WRITEDEPTH       = 0x02,
    GFX3DCONTROLOP_WRITETIMESTAMP   = 0x03
};

/*****************************************************************************\
ENUM: GFX3DSTATE_ROUNDING_MODE
\*****************************************************************************/
enum GFX3DSTATE_ROUNDING_MODE
{
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEAREST_EVEN  = 0x0,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_POS_INF       = 0x1,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_NEG_INF       = 0x2,
    GFX3DSTATE_ROUNDING_MODE_ROUND_TO_ZERO          = 0x3
};
} // namespace G6HWC
