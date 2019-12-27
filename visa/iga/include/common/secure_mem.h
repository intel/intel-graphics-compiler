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

#include <errno.h>
#include <string.h>
#include <stdio.h>

typedef int errno_t;
inline errno_t memcpy_s( void *dst, size_t numberOfElements, const void *src, size_t count )
{
    if( ( dst == NULL ) || ( src == NULL ) )
    {
        return EINVAL;
    }
    if( numberOfElements < count )
    {
        return ERANGE;
    }
    memcpy( dst, src, count );
    return 0;
}

inline errno_t fopen_s( FILE** pFile, const char* filename, const char *mode )
{
    if( pFile == NULL )
    {
        return EINVAL;
    }
    *pFile = fopen( filename, mode );
    if( *pFile == NULL )
    {
        return errno;
    }
    return 0;
}

#endif
