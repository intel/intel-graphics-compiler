/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cstdint>

#if defined(_WIN32)
    typedef unsigned __int64    QWORD;          //  64-bits,    8-bytes
    typedef unsigned long       DWORD;          //  32-bits,    4-bytes
    typedef signed __int64      INT64;          //  64-bits,    8-bytes
    typedef unsigned __int64    UINT64;         //  64-bits,    8-bytes
    typedef unsigned long       ULONG;          //  32-bits,    4-bytes

#else // if !defined(_WIN32)

    typedef std::uint64_t            QWORD;          //  64-bits,    8-bytes
    typedef std::uint32_t            DWORD;          //  32-bits,    4-bytes
    typedef std::int64_t             INT64;          //  64-bits,    8-bytes
    typedef std::uint64_t            UINT64;         //  64-bits,    8-bytes
    typedef unsigned int        ULONG;

#endif

typedef unsigned int            UINT32;         //  32-bits,    4-bytes
typedef unsigned int            UINT;           //  32-bits,    4-bytes
typedef int                     INT;            //  32-bits,    4-bytes

typedef unsigned char           BYTE;           //   8-bits,    1-byte
typedef unsigned short          WORD;           //  16-bits,    2-bytes

typedef BYTE                    KILOBYTE[1024]; //           1024-bytes
typedef KILOBYTE                MEGABYTE[1024]; //           1024-kilobytes
typedef MEGABYTE                GIGABYTE[1024]; //           1024-megabytes

typedef BYTE                    PAGE[4096];     //           4096-bytes
typedef unsigned short          HEXWORD[16];    // 256-bits,   32-bytes

/*****************************************************************************\
MACRO: BITFIELD_RANGE
PURPOSE: Calculates the number of bits between the startbit and the endbit (0 based)
\*****************************************************************************/
#ifndef BITFIELD_RANGE
#define BITFIELD_RANGE( startbit, endbit )     ((endbit)-(startbit)+1)
#endif

/*****************************************************************************\
MACRO: BITFIELD_BIT
PURPOSE: Definition declared for clarity when creating structs
\*****************************************************************************/
#ifndef BITFIELD_BIT
#define BITFIELD_BIT( bit )                   1
#endif

/*****************************************************************************\
MACRO: SIZE32
\*****************************************************************************/
#ifndef SIZE32
#define SIZE32( x )         ((DWORD)( sizeof(x) / sizeof(DWORD) ))
#endif
