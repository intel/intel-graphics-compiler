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
#if ((!defined _WIN32) && ( !defined __STDC_LIB_EXT1__ ))

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static inline int
strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
    strncpy(strDestination, strSource, numberOfElements);
    strDestination[numberOfElements - 1] = '\0';
    return 0;
}

static inline int
strncpy_s(char *strDestination, size_t numberOfElements, const char *strSource, size_t count)
{
    if( numberOfElements - 1 > count ) {
        strncpy(strDestination, strSource, count);
        strDestination[count] = '\0';
    } else {
        strncpy(strDestination, strSource, numberOfElements - 1);
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
        strncpy(strDestination + existingIndex, strSource, count);
        strDestination[existingIndex + count] = '\0';
    } else if( elementsLeft > 1 ) {
        strncpy(strDestination + existingIndex, strSource, elementsLeft - 1);
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
