/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <string>
#include "IGC/common/igc_debug.h"

namespace iOpenCL
{

/*****************************************************************************\
Function: DebugMessage
\*****************************************************************************/

void DebugMessageStr(std::string& output, unsigned int ulDebugLevel, const char* fmt, ...);

/*****************************************************************************\
MACRO: ICBE_DPF_STR
\*****************************************************************************/
#ifndef ICBE_DPF_STR
    #if defined(_DEBUG) || defined(_INTERNAL) || defined(_RELEASE_INTERNAL)
        #define ICBE_DPF_STR iOpenCL::DebugMessageStr
    #else
        #if defined(ICBE_LHDM) || defined(_WIN32)
            #define ICBE_DPF_STR(output, format, args, ...)
        #else
            #if defined(ICBE_LINUX) || defined(_LINUX) || defined(LINUX)
                #define ICBE_DPF_STR iOpenCL::DebugMessageStr
            #else
                #define ICBE_DPF_STR(output, format, args...)
            #endif
        #endif
    #endif // _DEBUG
#endif // ICBE_DPF_STR

} // namespace iOpenCL
