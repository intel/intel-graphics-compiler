/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _VISA_DEFINES_
#define _VISA_DEFINES_

#include <climits>
#include <cstring>

#define DIR_WIN32_SEPARATOR "\\"
#define DIR_UNIX_SEPARATOR "/"

/* declspecs */
#ifdef _WIN32
#define _THREAD __declspec(thread)
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define _THREAD __thread
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define VISA_BUILDER_API
#endif /* _VISA_DEFINES_ */
