/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _VISA_DEFINES_
#define _VISA_DEFINES_

#ifdef __cplusplus
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#include <wchar.h>
}
#include <climits>
#include <cstring>
#else
#include <limits.h>
#include <string.h>
#include <wchar.h>
#endif

#define DIR_WIN32_SEPARATOR "\\"
#define DIR_UNIX_SEPARATOR "/"

/* declspecs */
#ifdef _WIN32
#define _THREAD __declspec(thread)
#define DLL_EXPORT extern "C" __declspec(dllexport)
#define _PACKED
#else
#ifdef ANDROID
#define _THREAD
#else
#define _THREAD __thread
#endif

#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define _PACKED
#endif

#if defined(_MSC_VER)
#define __restrict__ __restrict
#else
#define __restrict__
#endif

#if defined(DLL_MODE) && defined(vISA_LINK_DLL)
#ifdef _WIN32
#define VISA_BUILDER_API __declspec(dllexport)
#else
#define VISA_BUILDER_API __attribute__((visibility("default")))
#endif
#else
#define VISA_BUILDER_API
#endif

#ifndef SNPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define SNPRINTF(dst, size, ...) snprintf((dst), (size), __VA_ARGS__)
#else
#define SNPRINTF(dst, size, ...) sprintf_s((dst), (size), __VA_ARGS__)
#endif
#endif

#endif /* _VISA_DEFINES_ */
