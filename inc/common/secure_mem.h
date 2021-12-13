/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if ((!defined _WIN32) && ( !defined __STDC_LIB_EXT1__ ))

#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifndef MEMCPY_S
#define MEMCPY_S
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
#endif

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
