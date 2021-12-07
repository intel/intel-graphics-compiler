/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_API_CALLS_H
#define IGC_API_CALLS_H

#include "../../common/EmUtils.h"

#if defined(_WIN32)
    #define IGC_API_CALL_TYPE __stdcall
    #ifdef IGC_EXPORTS
        #define IGC_API_CALL __declspec(dllexport) IGC_API_CALL_TYPE
    #else
        #define IGC_API_CALL __declspec(dllimport) IGC_API_CALL_TYPE
    #endif
#else
        #define IGC_API_CALL_TYPE
    #define IGC_API_CALL __attribute__ ((visibility("default"))) IGC_API_CALL_TYPE
#endif

#endif // IGC_API_CALLS_H
