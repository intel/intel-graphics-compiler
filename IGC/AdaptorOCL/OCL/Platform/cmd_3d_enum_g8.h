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