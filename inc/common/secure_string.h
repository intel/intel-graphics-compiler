/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if ((!defined _WIN32) && ( !defined __STDC_LIB_EXT1__ ))

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static inline int
strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    // As WA for issue in Fedora build
    // need to rework this
#endif
    strncpy(strDestination, strSource, numberOfElements);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
    strDestination[numberOfElements - 1] = '\0';
    return 0;
}

static inline int
strncpy_s(char *strDestination, size_t numberOfElements, const char *strSource, size_t count)
{
    if( numberOfElements - 1 > count ) {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
        // As WA for issue in Fedora build
        // need to rework this
#endif
        strncpy(strDestination, strSource, count);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
        strDestination[count] = '\0';
    } else {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
        // As WA for issue in Fedora build
        // need to rework this
#endif
        strncpy(strDestination, strSource, numberOfElements - 1);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
        strDestination[numberOfElements - 1] = '\0';
    }
    return 0;
}

static inline int
strncat_s(char *strDestination, size_t numberOfElements, const char *strSource, size_t count)
{
    unsigned int existingIndex = numberOfElements < strlen(strDestination) ?
                            numberOfElements:strlen(strDestination);
    unsigned int elementsLeft = numberOfElements - existingIndex;

    if( elementsLeft > count ) {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
        // As WA for issue in Fedora build
        // need to rework this
#endif
        strncpy(strDestination + existingIndex, strSource, count);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
        strDestination[existingIndex + count] = '\0';
    } else if( elementsLeft > 1 ) {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
        // As WA for issue in Fedora build
        // need to rework this
#endif
        strncpy(strDestination + existingIndex, strSource, elementsLeft - 1);
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
        strDestination[existingIndex + elementsLeft] = '\0';
    }
    return 0;
}

static inline int
strcat_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
    strncat_s(strDestination, numberOfElements, strSource, strlen(strSource));

    return 0;
}

static inline size_t strnlen_s(const char *str, size_t count)
{
    if (str == NULL)
    {
        return 0;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (str[i] == '\0')
            return i;
    }
    return count;
}

// sscanf_s is a visual studio specific function that requires additional parameters
// (on the __VA_ARGS__ list) when strings and chars are being searched for (s, S, c, C, [ flags).
// For now, only %d formatting is being used, and there is no point in rewriting the whole function.
#define sscanf_s(buffer, format, ...)               sscanf((buffer), (format), __VA_ARGS__)

static inline int
sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);

    int retVal = vsnprintf(buffer, sizeOfBuffer, format, arglist);
    buffer[sizeOfBuffer - 1] = '\0';

    va_end(arglist);

    return retVal;
}

static inline int
_vsnprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, va_list argptr)
{
    if( sizeOfBuffer + 1 >  count ) {
        return vsnprintf(buffer, count, format, argptr);
    } else {
        int retVal = vsnprintf(buffer, sizeOfBuffer, format, argptr);
        buffer[sizeOfBuffer - 1] = '\0';
        return retVal;
    }
}

static inline int
_snprintf_s(char *buffer, size_t sizeOfBuffer, size_t count, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int result = _vsnprintf_s(buffer, sizeOfBuffer, count, format, ap);
    va_end(ap);
    return result;
}

#endif
