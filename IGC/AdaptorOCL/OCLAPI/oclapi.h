/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OCL_API_CALLS_H
#define OCL_API_CALLS_H

#if defined(_WIN32)
#define OCL_API_CALL
#else
#define OCL_API_CALL __attribute__((visibility("default")))
#endif

#endif // OCL_API_CALLS_H
