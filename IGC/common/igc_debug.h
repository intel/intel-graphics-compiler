/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "usc_debugControl.h"

#include "usc_config.h"

/*****************************************************************************\
GFXDBG legacy debug level macros
\*****************************************************************************/
#ifndef GFXDBG_OFF
#define GFXDBG_OFF                      USC_DBG_OFF
#define GFXDBG_CRITICAL                 USC_DBG_CRITICAL
#define GFXDBG_NORMAL                   USC_DBG_NORMAL
#define GFXDBG_VERBOSE                  USC_DBG_VERBOSE
#define GFXDBG_VERBOSE_VERBOSITY        USC_DBG_VERBOSE_VERBOSITY
#define GFXDBG_INTERFACE                USC_DBG_INTERFACE
#define GFXDBG_HARDWARE                 USC_DBG_HARDWARE
#define GFXDBG_COMPILER                 USC_DBG_COMPILER
#define GFXDBG_COMPILER_LIR_DUMP        USC_DBG_COMPILER_LIR_DUMP
#define GFXDBG_COMPILER_IR_TO_LIR_TRACE USC_DBG_COMPILER_IR_TO_LIR_TRACE
#define GFXDBG_FUNCTION                 USC_DBG_FUNCTION
#define GFXDBG_FUNCTION_ENTRY           USC_DBG_FUNCTION
#define GFXDBG_FUNCTION_EXIT            USC_DBG_FUNCTION
#endif

