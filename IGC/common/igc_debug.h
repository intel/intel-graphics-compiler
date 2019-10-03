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

