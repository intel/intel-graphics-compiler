/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "cmd_enum_g8.h"

namespace G6HWC
{
/*****************************************************************************\
ENUM: GFX_MEDIA_PIPELINED_SUBOPCODE
\*****************************************************************************/
enum GFX_MEDIA_PIPELINED_SUBOPCODE
{
    GFXSUBOP_MEDIA_VFE_STATE                   = 0x0,
    GFXSUBOP_MEDIA_CURBE_LOAD                  = 0x1,
    GFXSUBOP_MEDIA_INTERFACE_DESCRIPTOR_LOAD   = 0x2,
    GFXSUBOP_MEDIA_GATEWAY_STATE               = 0x3,
    GFXSUBOP_MEDIA_STATE_FLUSH                 = 0x4
};

/*****************************************************************************\
ENUM: GFX_MEDIA_NONPIPELINED_SUBOPCODE
\*****************************************************************************/
enum GFX_MEDIA_NONPIPELINED_SUBOPCODE
{
    GFXSUBOP_MEDIA_OBJECT                       = 0x0,
    GFXSUBOP_MEDIA_OBJECT_PRT                   = 0x2,
    GFXSUBOP_MEDIA_OBJECT_WALKER                = 0x3
};

/*****************************************************************************\
ENUM: GFX_MEDIA_NONPIPELINED_SUBOPCODE_A
\*****************************************************************************/
enum GFX_MEDIA_NONPIPELINED_SUBOPCODE_A
{
    GFXSUBOP_MEDIA_GPGPU_WALKER                 = 0x5
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_DEBUG_COUNTER_CONTROL
\*****************************************************************************/
enum GFXMEDIASTATE_DEBUG_COUNTER_CONTROL
{
    GFXMEDIASTATE_DEBUG_COUNTER_FREE_RUNNING        = 0x0,
    GFXMEDIASTATE_DEBUG_COUNTER_FROZEN              = 0x1,
    GFXMEDIASTATE_DEBUG_COUNTER_INITIALIZED_ONCE    = 0x2,
    GFXMEDIASTATE_DEBUG_COUNTER_INITIALIZED_ALWAYS  = 0x3
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_FLOATING_POINT_MODE
\*****************************************************************************/
enum GFXMEDIASTATE_FLOATING_POINT_MODE
{
    GFXMEDIASTATE_FLOATING_POINT_IEEE_754       = 0x0,
    GFXMEDIASTATE_FLOATING_POINT_NON_IEEE_754   = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_THREAD_PRIORITY
\*****************************************************************************/
enum GFXMEDIASTATE_THREAD_PRIORITY
{
    GFXMEDIASTATE_THREAD_PRIORITY_NORMAL    = 0x0,
    GFXMEDIASTATE_THREAD_PRIORITY_HIGH      = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_VFE_MODE
\*****************************************************************************/
enum GFXMEDIASTATE_VFE_MODE
{
    GFXMEDIASTATE_VFE_MODE_GENERIC                  = 0x0,
    GFXMEDIASTATE_VFE_MODE_VLD                      = 0x1,
    GFXMEDIASTATE_VFE_MODE_IS                       = 0x2,
    GFXMEDIASTATE_VFE_MODE_AVC_MC                   = 0x4,
    GFXMEDIASTATE_VFE_MODE_AVC_IT                   = 0x7,
    GFXMEDIASTATE_VFE_MODE_VC1_IT                   = 0xB
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_GPGPU_MODE
\*****************************************************************************/
enum GFXMEDIASTATE_GPGPU_MODE
{
    GFXMEDIASTATE_GPGPU_MODE_MEDIA   = 0x0,
    GFXMEDIASTATE_GPGPU_MODE_GPGPU   = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_SLM_GRANULARITY
\*****************************************************************************/
enum GFXMEDIASTATE_SLM_GRANULARITY
{
    GFXMEDIASTATE_SLM_GRANULARITY_4K   = 0x0,
    GFXMEDIASTATE_SLM_GRANULARITY_1K   = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_MMIO_ACCESS_CONTROL
\*****************************************************************************/
enum GFXMEDIASTATE_MMIO_ACCESS_CONTROL
{
    GFXMEDIASTATE_MMIO_ACCESS_CONTROL_NO_READWRITE   = 0x0,
    GFXMEDIASTATE_MMIO_ACCESS_CONTROL_OA_READWRITE   = 0x1,
    GFXMEDIASTATE_MMIO_ACCESS_CONTROL_ANY_READWRITE  = 0x2
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_SCOREBOARD_TYPE
\*****************************************************************************/
enum GFXMEDIASTATE_SCOREBOARD_TYPE
{
    GFXMEDIASTATE_STALLING_SCOREBOARD               = 0x0,
    GFXMEDIASTATE_NONSTALLING_SCOREBOARD            = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_THREAD_SYNCHRONIZATION
\*****************************************************************************/
enum GFXMEDIASTATE_THREAD_SYNCHRONIZATION
{
    GFXMEDIASTATE_NO_THREAD_SYNCHRONIZATION           = 0x0,
    GFXMEDIASTATE_THREAD_DISPATCH_SYNCHRONIZED        = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_PRT_FENCE_TYPE
\*****************************************************************************/
enum GFXMEDIASTATE_PRT_FENCE_TYPE
{
    GFXMEDIASTATE_ROOT_THREAD_QUEUE                   = 0x0,
    GFXMEDIASTATE_VFE_STATE_FLUSH                     = 0x1
};

/*****************************************************************************\
ENUM: GFXMEDIASTATE_GPGPU_WALKER_SIMD_SIZE
\*****************************************************************************/
enum GFXMEDIASTATE_GPGPU_WALKER_SIMD_SIZE
{
    GFXMEDIASTATE_GPGPU_WALKER_SIMD8                   = 0x0,
    GFXMEDIASTATE_GPGPU_WALKER_SIMD16                  = 0x1,
    GFXMEDIASTATE_GPGPU_WALKER_SIMD32                  = 0x2
};

} // namespace G6HWC
