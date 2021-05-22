/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifndef ISTDLIB_TYPES_H_INCLUDED
#define ISTDLIB_TYPES_H_INCLUDED

#include "cppcompiler.h"
#include "osinlines.h"

/*****************************************************************************\
standard TYPEDEFs (common)
\*****************************************************************************/
#if defined(_WIN32)
    typedef unsigned __int64    QWORD;          //  64-bits,    8-bytes
    typedef unsigned long       DWORD;          //  32-bits,    4-bytes
    typedef signed __int64      INT64;          //  64-bits,    8-bytes
    typedef unsigned __int64    UINT64;         //  64-bits,    8-bytes
    typedef unsigned long       ULONG;          //  32-bits,    4-bytes
    typedef unsigned int        UINT32;         //  32-bits,    4-bytes
    typedef unsigned int        UINT;           //  32-bits,    4-bytes
    typedef int                 INT;            //  32-bits,    4-bytes
    typedef unsigned char       BYTE;           //   8-bits,    1-byte
    typedef unsigned short      WORD;           //  16-bits,    2-bytes
#else // if !defined(_WIN32)
    #include <UFO/portable_windef.h>
#endif // if !defined(_WIN32)


typedef BYTE                    KILOBYTE[1024]; //           1024-bytes
typedef KILOBYTE                MEGABYTE[1024]; //           1024-kilobytes
typedef MEGABYTE                GIGABYTE[1024]; //           1024-megabytes

typedef BYTE                    PAGE[4096];     //           4096-bytes
typedef unsigned short          HEXWORD[16];    // 256-bits,   32-bytes

/*****************************************************************************\
STRUCT: SRange
\*****************************************************************************/
#ifdef __cplusplus
template<class Type>
struct SRange
{
    Type    Min;
    Type    Max;
};

template<class Type>
struct SRangeA
{
    Type    Min;
    Type    Max;

    SRangeA( const Type min, const Type max )
    {
        Min = min;
        Max = max;
    };
};
#endif


#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4201 ) // warning C4201: nonstandard extension used : nameless struct/union
#endif


/*****************************************************************************\
UNION: VALUE8
\*****************************************************************************/
union VALUE8
{
    unsigned char      u;
    signed char        s;

    struct
    {
        unsigned char   l : 4;
        unsigned char   h : 4;
    };
};

/*****************************************************************************\
UNION: VALUE16
\*****************************************************************************/
union VALUE16
{
    unsigned short      u;
    signed short        s;

    struct
    {
        VALUE8          l;
        VALUE8          h;
    };
};

/*****************************************************************************\
UNION: VALUE32
\*****************************************************************************/
union VALUE32
{
    unsigned int        u;
    signed int          s;
    float               f;
    DWORD               d;  // Allows creation of VALUE32 from DWORD.

    struct
    {
        VALUE16         l;
        VALUE16         h;
    };
};

/*****************************************************************************\
UNION: VALUE64
\*****************************************************************************/
union VALUE64
{
    UINT64              u;
    INT64               s;
    double              f;

    struct
    {
        VALUE32         l;
        VALUE32         h;
    };
};

/*****************************************************************************\
UNION: FLOAT8
\*****************************************************************************/
union FLOAT8
{
    struct
    {
        DWORD fraction  : 4;
        DWORD exponent  : 3;
        DWORD sign      : 1;
    };

    VALUE8 value;

    FLOAT8()
    {
        value.u = 0;
    };
    FLOAT8( unsigned char uVal )
    {
        value.u =  uVal;
    };
    FLOAT8( signed char sVal )
    {
        value.s = sVal;
    };

};

/*****************************************************************************\
UNION: FLOAT16
\*****************************************************************************/
union FLOAT16
{
    struct
    {
        DWORD fraction  : 10;
        DWORD exponent  : 5;
        DWORD sign      : 1;
    };

    VALUE16 value;
};

/*****************************************************************************\
UNION: FLOAT32
\*****************************************************************************/
union FLOAT32
{
    struct
    {
        DWORD   fraction    : 23;
        DWORD   exponent    : 8;
        DWORD   sign        : 1;
    };

    VALUE32 value;

    // constructors to perform trivial conversions from
    // long, ulong and float to FLOAT32 union
    FLOAT32()
    {
        value.f = 0.0f;
    };
    FLOAT32( unsigned long uVal )
    {
        value.u =  uVal;
    };
    FLOAT32( signed long sVal )
    {
        value.s = sVal;
    };
    FLOAT32( float fVal )
    {
        value.f = fVal;
    };
#if defined(__GNUC__)
    // Curently types DWORD and unsigned long differs when using gcc and msvc.
    FLOAT32( DWORD val)
    {
        value.d = val;
    }
#endif // __GNUC__
};

/*****************************************************************************\
UNION: FLOAT64
\*****************************************************************************/
union FLOAT64
{
    struct
    {
        QWORD   fraction    : 52;
        QWORD   exponent    : 11;
        QWORD   sign        : 1;
    };

    VALUE64 value;

    // constructors to perform trivial conversions from
    // long, ulong and double float to FLOAT64 union
    FLOAT64()
    {
        value.f = 0.0;
    };
    FLOAT64( UINT64 uVal )
    {
        value.u =  uVal;
    };
    FLOAT64( INT64 sVal )
    {
        value.s = sVal;
    };
    FLOAT64( double fVal )
    {
        value.f = fVal;
    };
};


#ifdef _MSC_VER
#pragma warning( pop ) // warning C4201: nonstandard extension used : nameless struct/union
#endif



// TO DO - SRange, SRectangle is GHAL3D-specific, move to separate header?

/*****************************************************************************\
STRUCT: SRectangle
\*****************************************************************************/
#ifdef __cplusplus
template<class Type>
struct SRectangle
{
    SRange<Type>    X;
    SRange<Type>    Y;
};

template<class Type>
struct SRectangleA
{
    SRangeA<Type>   X;
    SRangeA<Type>   Y;

    SRectangleA( const Type xmin, const Type xmax, const Type ymin, const Type ymax )
        : X( xmin, xmax ),
        Y( ymin, ymax )
    {
    };
};
#endif

/*****************************************************************************\
MACRO: SIZE8
\*****************************************************************************/
#ifndef SIZE8
#define SIZE8( x )         ((DWORD)( sizeof(x) ))
#endif

/*****************************************************************************\
MACRO: SIZE16
\*****************************************************************************/
#ifndef SIZE16
#define SIZE16( x )         ((DWORD)( sizeof(x) / sizeof(WORD) ))
#endif

/*****************************************************************************\
MACRO: SIZE32
\*****************************************************************************/
#ifndef SIZE32
#define SIZE32( x )         ((DWORD)( sizeof(x) / sizeof(DWORD) ))
#endif

/*****************************************************************************\
MACRO: SIZE64
\*****************************************************************************/
#ifndef SIZE64
#define SIZE64( x )         ((QWORD)( sizeof(x) / sizeof(QWORD) ))
#endif


/*****************************************************************************\
MACRO: OP_LENGTH
\*****************************************************************************/
#ifndef OP_LENGTH
#define OP_LENGTH( x )      ((DWORD)(x) - 2 )
#endif

/*****************************************************************************\
CONST: BITS_PER_BYTE, BITS_PER_WORD, BITS_PER_DWORD, BITS_PER_QWORD
\*****************************************************************************/
const DWORD BITS_PER_BYTE   = 8;
const DWORD BITS_PER_WORD   = BITS_PER_BYTE * sizeof(WORD);
const DWORD BITS_PER_DWORD  = BITS_PER_BYTE * sizeof(DWORD);
const DWORD BITS_PER_QWORD  = BITS_PER_BYTE * sizeof(QWORD);

#endif
