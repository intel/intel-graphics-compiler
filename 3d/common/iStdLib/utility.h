/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include "Debug.h"
#include <stdlib.h>

#if defined _WIN32
#   include <BaseTsd.h>
#   include <intrin.h>
#endif

#if !defined(_WIN32)
#   include "../../inc/common/secure_mem.h"
#   include "../../inc/common/secure_string.h"
#endif

namespace iSTD
{

/*****************************************************************************\
MACRO: BIT
\*****************************************************************************/
#ifndef BIT
#define BIT( n )    ( 1 << (n) )
#endif

/*****************************************************************************\
MACRO: MASKED_BIT
\*****************************************************************************/
#ifndef MASKED_BIT
#define MASKED_BIT( n, enable ) ( 1 << (n + 16) | ((enable) ? 1 : 0) << (n) )
#endif

/*****************************************************************************\
MACRO: QWBIT
\*****************************************************************************/
#ifndef QWBIT
#define QWBIT( n )    ( 1ll << (n) )
#endif

/*****************************************************************************\
MACRO: BITMASK
PURPOSE: Creates a mask of n bits
\*****************************************************************************/
#ifndef BITMASK
#define BITMASK( n )    ( ~( (0xffffffff) << (n) ) )
#endif
#ifndef BITMASK_RANGE
#define BITMASK_RANGE( startbit, endbit )   ( BITMASK( (endbit)+1 ) & ~BITMASK( startbit ) )
#endif

/*****************************************************************************\
MACRO: QWBITMASK
PURPOSE: Creates a mask of n bits
\*****************************************************************************/
#ifndef QWBITMASK
#define QWBITMASK( n ) ((n) >= 64 ? 0xffffffffffffffffull : (~((0xffffffffffffffffull) << (n))))
#endif

#ifndef QWBITMASK_RANGE
#define QWBITMASK_RANGE( startbit, endbit )  ( QWBITMASK( (endbit)+1 ) & ~QWBITMASK( startbit ) )
#endif

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
MACRO: GETMSB
PURPOSE: Checks MSB
\*****************************************************************************/
#ifndef GETMSB
#define GETMSB( n ) ( \
    ( (n) & BIT(31) ) ? 31 : \
    ( (n) & BIT(30) ) ? 30 : \
    ( (n) & BIT(29) ) ? 29 : \
    ( (n) & BIT(28) ) ? 28 : \
    ( (n) & BIT(27) ) ? 27 : \
    ( (n) & BIT(26) ) ? 26 : \
    ( (n) & BIT(25) ) ? 25 : \
    ( (n) & BIT(24) ) ? 24 : \
    ( (n) & BIT(23) ) ? 23 : \
    ( (n) & BIT(22) ) ? 22 : \
    ( (n) & BIT(21) ) ? 21 : \
    ( (n) & BIT(20) ) ? 20 : \
    ( (n) & BIT(19) ) ? 19 : \
    ( (n) & BIT(18) ) ? 18 : \
    ( (n) & BIT(17) ) ? 17 : \
    ( (n) & BIT(16) ) ? 16 : \
    ( (n) & BIT(15) ) ? 15 : \
    ( (n) & BIT(14) ) ? 14 : \
    ( (n) & BIT(13) ) ? 13 : \
    ( (n) & BIT(12) ) ? 12 : \
    ( (n) & BIT(11) ) ? 11 : \
    ( (n) & BIT(10) ) ? 10 : \
    ( (n) & BIT(9)  ) ?  9 : \
    ( (n) & BIT(8)  ) ?  8 : \
    ( (n) & BIT(7)  ) ?  7 : \
    ( (n) & BIT(6)  ) ?  6 : \
    ( (n) & BIT(5)  ) ?  5 : \
    ( (n) & BIT(4)  ) ?  4 : \
    ( (n) & BIT(3)  ) ?  3 : \
    ( (n) & BIT(2)  ) ?  2 : \
    ( (n) & BIT(1)  ) ?  1 : \
    ( (n) & BIT(0)  ) ?  0 : \
    (-1) )
#endif

/*****************************************************************************\
MACRO: BITCOUNT
PURPOSE: Determines the number of bits needed in a bitmask, given the number
of elements to be stored in the mask
\*****************************************************************************/
#ifndef BITCOUNT
#define BITCOUNT( n ) ( \
    ( ((n)-1) & BIT(31) ) ? 32 : \
    ( ((n)-1) & BIT(30) ) ? 31 : \
    ( ((n)-1) & BIT(29) ) ? 30 : \
    ( ((n)-1) & BIT(28) ) ? 29 : \
    ( ((n)-1) & BIT(27) ) ? 28 : \
    ( ((n)-1) & BIT(26) ) ? 27 : \
    ( ((n)-1) & BIT(25) ) ? 26 : \
    ( ((n)-1) & BIT(24) ) ? 25 : \
    ( ((n)-1) & BIT(23) ) ? 24 : \
    ( ((n)-1) & BIT(22) ) ? 23 : \
    ( ((n)-1) & BIT(21) ) ? 22 : \
    ( ((n)-1) & BIT(20) ) ? 21 : \
    ( ((n)-1) & BIT(19) ) ? 20 : \
    ( ((n)-1) & BIT(18) ) ? 19 : \
    ( ((n)-1) & BIT(17) ) ? 18 : \
    ( ((n)-1) & BIT(16) ) ? 17 : \
    ( ((n)-1) & BIT(15) ) ? 16 : \
    ( ((n)-1) & BIT(14) ) ? 15 : \
    ( ((n)-1) & BIT(13) ) ? 14 : \
    ( ((n)-1) & BIT(12) ) ? 13 : \
    ( ((n)-1) & BIT(11) ) ? 12 : \
    ( ((n)-1) & BIT(10) ) ? 11 : \
    ( ((n)-1) & BIT(9)  ) ? 10 : \
    ( ((n)-1) & BIT(8)  ) ?  9 : \
    ( ((n)-1) & BIT(7)  ) ?  8 : \
    ( ((n)-1) & BIT(6)  ) ?  7 : \
    ( ((n)-1) & BIT(5)  ) ?  6 : \
    ( ((n)-1) & BIT(4)  ) ?  5 : \
    ( ((n)-1) & BIT(3)  ) ?  4 : \
    ( ((n)-1) & BIT(2)  ) ?  3 : \
    ( ((n)-1) & BIT(1)  ) ?  2 : \
    ( ((n)-1) & BIT(0)  ) ?  1 : \
    0 )
#endif

/*****************************************************************************\
MACRO: MIN
\*****************************************************************************/
#ifndef MIN
#define MIN( x, y ) (((x)<=(y))?(x):(y))
#endif

/*****************************************************************************\
MACRO: MAX
\*****************************************************************************/
#ifndef MAX
#define MAX( x, y ) (((x)>=(y))?(x):(y))
#endif

/*****************************************************************************\
MACRO: CEIL_DIV
\*****************************************************************************/
#ifndef CEIL_DIV
#define CEIL_DIV( x, y ) ( 1 + ( ( ( x ) - 1 ) / ( y ) ) )
#endif

/*****************************************************************************\
MACRO: STRCAT
\*****************************************************************************/
#ifndef STRCAT
#define STRCAT( dst, size, src ) strcat_s( (dst), (size), (src) )
#endif

/*****************************************************************************\
MACRO: STRNCAT
\*****************************************************************************/
#ifndef STRNCAT
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define STRNCAT( dst, size, src, len ) strncat( (dst), (src), (len) )
#else
#define STRNCAT( dst, size, src, len ) strncat_s( (dst), (size), (src), (len) )
#endif
#endif

/*****************************************************************************\
MACRO: WCSNCAT
\*****************************************************************************/
#ifndef WCSNCAT
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define WCSNCAT( dst, size, src, len ) wcsncat( (dst), (src), (len) )
#else
#define WCSNCAT( dst, size, src, len ) wcsncat_s( (dst), (size), (src), (len) )
#endif
#endif

/*****************************************************************************\
MACRO: STRCPY
\*****************************************************************************/
#ifndef STRCPY
#define STRCPY( dst, size, src ) strcpy_s( (dst), (size), (src) )
#endif

/*****************************************************************************\
MACRO: SPRINTF
\*****************************************************************************/
#ifndef SPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define SPRINTF( dst, size, src, args ) sprintf( (dst), (src), (args) )
#else
#define SPRINTF( dst, size, src, args ) sprintf_s( (dst), (size), (src), (args) )
#endif
#endif

/*****************************************************************************\
MACRO: VSNPRINTF
\*****************************************************************************/
#ifndef VSNPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define VSNPRINTF( dst, size, len, src, args ) _vsnprintf( (dst), (len), (src), (args) )
#else
#define VSNPRINTF( dst, size, len, src, args ) _vsnprintf_s( (dst), (size), (len), (src), (args) )
#endif
#endif

/*****************************************************************************\
MACRO: VSPRINTF
\*****************************************************************************/
#ifndef VSPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define VSPRINTF( dst, size, src, args ) vsprintf( (dst), (src), (args) )
#else
#define VSPRINTF( dst, size, src, args ) vsprintf_s( (dst), (size), (src), (args) )
#endif
#endif

/*****************************************************************************\
MACRO: MEMCPY
\*****************************************************************************/
#ifndef MEMCPY
#if defined(__ANDROID__)
#define  MEMCPY( dst, size, src, args ) memcpy( (dst), (src), (args) )
#elif defined(ISTDLIB_KMD) || !defined(_MSC_VER)
#define MEMCPY( dst, size, src, args ) memcpy( (dst), (src), (args) )
#else
#define MEMCPY( dst, size, src, args ) memcpy_s( (dst), (size), (src), (args) )
#endif
#endif

/*****************************************************************************\
MACRO: ARRAY_COUNT
\*****************************************************************************/
#ifndef ARRAY_COUNT
#define ARRAY_COUNT( x ) ( sizeof( x ) / sizeof( x[ 0 ] ) )
#endif

/*****************************************************************************\
Inline Template Function:
    Swap

Description:
    Swaps the values of two variables of the same type
\*****************************************************************************/
template <class Type>
inline void Swap( Type &var0, Type &var1 )
{
    Type tmp = var0;
    var0 = var1;
    var1 = tmp;
}

/*****************************************************************************\
Inline Template Function:
    Min

Description:
    Returns the min of the two values
\*****************************************************************************/
template <class Type>
__forceinline Type Min( const Type var0, const Type var1 )
{
    return ( var0 <= var1 ) ? var0 : var1;
}

/*****************************************************************************\
Inline Template Function:
    Max

Description:
    Returns the max of the two values
\*****************************************************************************/
template <class Type>
__forceinline Type Max( const Type var0, const Type var1 )
{
    return ( var0 >= var1 ) ? var0 : var1;
}

/*****************************************************************************\
Inline Template Function:
    ClampMax

Description:
    Checks the value for Greater than the maximum value.  If the value is
    greater then the maximum then it returns the maximum value.  Otherwise, it
    returns the value.
\*****************************************************************************/
template<class Type>
__forceinline Type ClampMax( const Type value, const Type max )
{
    return ( ( (value) > (max) ) ? (max) : (value) );
}

/*****************************************************************************\
Inline Template Function:
    ClampMin

Description:
    Checks the value for less than the minimum value.  If the value is less
    then the minimum then it returns the minimum value.  Otherwise, it returns
    the value.
\*****************************************************************************/
template<class Type>
__forceinline Type ClampMin( const Type value, const Type min )
{
    return ( ( (value) < (min) ) ? (min) : (value) );
}

/*****************************************************************************\
Inline Template Function:
    Clamp

Description:
    Checks the value for less than the minimum value or greater than the
    maximum value.  If the value is less then the minimum then it returns the
    minimum value.  If the value is greater then the maximum then it returns
    the maximum value. Otherwise, it returns the value.
\*****************************************************************************/
template<class Type>
__forceinline Type Clamp( const Type value, const Type min, const Type max )
{
    return ClampMin<Type>( ClampMax<Type>( value, max ), min );
}

/*****************************************************************************\
Inline Template Function:
    CheckLimits

Description:
    Determines if the value is within the specified range
\*****************************************************************************/
template <class Type>
__forceinline bool CheckLimits( const Type value, const Type min, const Type max )
{
    if( ( value < min ) || ( value > max ) )
    {
        ASSERT(0);
        return false;
    }

    return true;
}

/*****************************************************************************\
Inline Template Function:
    emul

Description:
    Upconversion Multiply, used for checking overflow.
\*****************************************************************************/
template <typename t1, typename t2>
__forceinline t2 emul( t1 a, t1 b )
{
    return (t2)a*b;
}

/*****************************************************************************\
Inline Function:
    bsr64

Description:
    Intrinsic definition of bit scan reverse for 64bit values.
\*****************************************************************************/
#if defined( _WIN64 ) || defined( __x86_64__ )
__forceinline DWORD bsr64( const unsigned long long int mask )
{
#if defined _WIN32
    DWORD index;
    _BitScanReverse64( &index, static_cast<_int64>( mask ) );
    return static_cast<DWORD>( index );

#elif defined __linux__
    return static_cast<unsigned int>( 63 - __builtin_clzll( mask ) );

#else
    DWORD bit = 0;
    if( mask != 0 )
    {
        bit = 63;
        while( ( mask & QWBIT(bit) ) == 0 )
        {
            --bit;
        }
    }
    return bit;

#endif
}
#endif // defined( _WIN64 ) || defined( __x86_64__ )

/*****************************************************************************\
Inline Function:
    bsr

Description:
    Intrinsic definition when not compiler-defined
\*****************************************************************************/
__forceinline DWORD bsr( const DWORD mask )
{
#if defined _WIN32
    DWORD index;
    _BitScanReverse( &index, mask );
    return static_cast<DWORD>(index);

#elif defined __linux__
    return static_cast<unsigned int>( 31 - __builtin_clz( mask ) );

#else
    DWORD bit = 0;
    if( mask != 0 )
    {
        bit = 31;
        while( ( mask & BIT(bit) ) == 0 )
        {
            --bit;
        }
    }
    return bit;

#endif
}

/*****************************************************************************\
Inline Function:
    bsr64

    Description:
    Intrinsic definition of bit scan forward for 64bit values.
\*****************************************************************************/
#if defined( _WIN64 ) || defined( __x86_64__ )
__forceinline DWORD bsf64( const unsigned long long int mask )
{
#if defined _WIN32
    DWORD index;
    _BitScanForward64( &index, static_cast<_int64>( mask ) );
    return static_cast<DWORD>( index );

#elif defined __linux__
    return static_cast<unsigned int>( __builtin_ffsll( mask ) - 1 );

#else
    DWORD bit = 0;
    if( mask != 0 )
    {
        while( ( mask & QWBIT(bit) ) == 0 )
        {
            ++bit;
        }
    }
    return bit;

#endif
}
#endif // defined( _WIN64 ) || defined( __x86_64__ )

/*****************************************************************************\
Inline Function:
    bsf

Description:
    Intrinsic definition when not compiler-defined
\*****************************************************************************/
__forceinline DWORD bsf( const DWORD mask )
{
#if defined _WIN32
    DWORD index;
    _BitScanForward( &index, mask );
    return index;

#elif defined __linux__
    return static_cast<unsigned int>( __builtin_ffsl( mask ) - 1 );

#else
    DWORD bit = 0;
    if( mask != 0 )
    {
        while( ( mask & BIT(bit) ) == 0 )
        {
            ++bit;
        }
    }
    return bit;

#endif
}

/*****************************************************************************\
Description:
    Find first zero which identifies the index of the least significant zero bit
    mask - mask to be checked
\*****************************************************************************/
#ifndef FIND_FIRST_0_LSB
#define FIND_FIRST_0_LSB( mask )    ( iSTD::bsf(~mask) )
#endif

/*****************************************************************************\
Inline Function:
    clz

Description:
    Count number of leading zeros of the mask
\*****************************************************************************/
__forceinline DWORD clz( const DWORD mask )
{
    DWORD retValue = 32;

    // bsr returns 0 if the mask is 0 and sets a the ZF flag so handle
    // 0 special.
    if( mask != 0 )
    {
        retValue = 31 - bsr( mask );
    }

    return retValue;
}

/*****************************************************************************\
Inline Function:
    IsPowerOfTwo

Description:
    Determines if the given value is a power of two.
\*****************************************************************************/
template< typename Type >
__forceinline bool IsPowerOfTwo( const Type number )
{
    return ( ( number & ( number - 1 ) ) == 0 );
}

/*****************************************************************************\
Inline Function:
    Round

Description:
    Rounds an unsigned integer to the next multiple of (power-2) size
\*****************************************************************************/
template< typename Type1, typename Type2 >
__forceinline Type1 Round( const Type1 value, const Type2 size )
{
    ASSERT( IsPowerOfTwo(size) );
    Type1 mask = (Type1)size - 1;
    Type1 roundedValue = ( value + mask ) & ~( mask );
    return roundedValue;
}

/*****************************************************************************\
Inline Function:
    RoundDown

Description:
    Rounds an unsigned integer to the previous multiple of (power-2) size
\*****************************************************************************/
template< typename Type1, typename Type2 >
__forceinline DWORD RoundDown( const Type1 value, const Type2 size )
{
    ASSERT( IsPowerOfTwo(size) );
    Type1 mask = (Type1)size - 1;
    Type1 roundedValue = value & ~( mask );
    return roundedValue;
}

/*****************************************************************************\
Inline Function:
    RoundNonPow2

Description:
    Rounds up to an unsigned integer to the next multiple of size (nonpow2)
\*****************************************************************************/
template< typename Type1, typename Type2 >
__forceinline Type1 RoundNonPow2( const Type1 value, const Type2 size )
{
    const Type1 size1 = (Type1)size;
    const Type1 remainder = ( value % size1 );

    Type1 roundedValue = value;
    if( remainder )
    {
        roundedValue += size1 - remainder;
    }
    return roundedValue;
}

/*****************************************************************************\
Inline Function:
    RoundDownNonPow2

Description:
    Rounds an unsigned integer to the previous multiple of size (nonpow2)
\*****************************************************************************/
template< typename Type1, typename Type2 >
__forceinline DWORD RoundDownNonPow2( const Type1 value, const Type2 size )
{
    const Type1 size1 = (Type1)size;
    return (DWORD)(( value / size1 ) * size1);
}

/*****************************************************************************\
Inline Function:
    RoundPower2

Description:
    Rounds an unsigned 32-bit integer to the next power of 2
\*****************************************************************************/
inline DWORD RoundPower2( const DWORD value )
{
    return IsPowerOfTwo( value ) ? value : 2ul << bsr( value );
}

/*****************************************************************************\
Inline Function:
    RoundPower2

Description:
    Rounds an unsigned 64-bit integer to the next power of 2
\*****************************************************************************/
inline QWORD RoundPower2( const QWORD value )
{
    VALUE64 v64 = { value };

    if( v64.h.u || ( v64.l.u & BIT(31) ) )
    {
        v64.h.u = RoundPower2( (DWORD)(( v64.l.u ) ? v64.h.u + 1 : v64.h.u) );
        v64.l.u = 0;
    }
    else
    {
        v64.l.u = RoundPower2( (DWORD)(v64.l.u) );
    }

    return v64.u;
}

/*****************************************************************************\
Inline Function:
    Log2

Description:
    Returns the logarithm base two of the passed in number by returning
    floor( log2( number ) ).  Also in the case of Log2(0) the function
    will return 0.
\*****************************************************************************/
inline DWORD Log2( const DWORD value )
{
    ASSERT( IsPowerOfTwo(value) );

    DWORD power2 = 0;
    while( value && value != (DWORD)BIT(power2) )
    {
        ++power2;
    }

    return power2;
}

/*****************************************************************************\
Inline Function:
    IsAligned

Description:
    Determines if the given pointer is aligned to the given size
\*****************************************************************************/
template< typename Type >
__forceinline bool IsAligned( Type * ptr, const size_t alignSize )
{
    return ( ( (size_t)ptr % alignSize ) == 0 );
}

/*****************************************************************************\
Inline Function:
    IsAligned

Description:
    Determines if the given size is aligned to the given size
\*****************************************************************************/
template< typename Type >
__forceinline bool IsAligned( Type size, const size_t alignSize )
{
    return ( ( size % alignSize ) == 0 );
}

/*****************************************************************************\
Inline Function:
    Align

Description:
    Type-safe (power-2) alignment of a pointer.
\*****************************************************************************/
template<typename Type>
__forceinline Type* Align( Type* const ptr, const size_t alignment )
{
    ASSERT( IsPowerOfTwo(alignment) );

    return (Type*)( ( ((size_t)ptr) + alignment-1 ) & ~( alignment-1 ) );
}

/*****************************************************************************\
Inline Function:
    Align

Description:
    Type-safe (power-2) alignment of a value.
\*****************************************************************************/
template<typename Type>
__forceinline Type Align( const Type value, const size_t alignment )
{
    ASSERT( IsPowerOfTwo(alignment) );

    Type mask = static_cast<Type>(alignment) - 1;
    return (value + mask) & ~mask;
}

/*****************************************************************************\
Inline Function:
    GetAlignmentOffset

Description:
    Returns the size in bytes needed to align the given pointer to the
    given alignment size
\*****************************************************************************/
template<typename Type>
__forceinline DWORD GetAlignmentOffset( Type* const ptr, const size_t alignSize )
{
    ASSERT( alignSize );

    DWORD offset = 0;

    if( IsPowerOfTwo(alignSize) )
    {   // can recast 'ptr' to DWORD, since offset is DWORD
        offset = DWORD( UINT_PTR( Align(ptr, alignSize) ) - (UINT_PTR)(ptr) );
    }
    else
    {
        const DWORD modulo = (DWORD)(UINT_PTR(ptr) % alignSize);

        if( modulo )
        {
            offset = (DWORD)alignSize - modulo;
        }
    }

    return offset;
}

/*****************************************************************************\
Inline Function:
    GetAlignmentOffset

Description:
    Returns the size in bytes needed to align the given size to the
    given alignment size
\*****************************************************************************/
template<typename Type>
__forceinline Type GetAlignmentOffset( const Type size, const size_t alignSize )
{
    ASSERT( alignSize );

    Type offset = 0;

    if( IsPowerOfTwo(alignSize) )
    {
        offset = Align(size, alignSize) - size;
    }
    else
    {
        const Type modulo = (Type)( size % alignSize );

        if( modulo )
        {
            offset = (Type)alignSize - modulo;
        }
    }

    return offset;
}

/*****************************************************************************\
Inline Function:
    MemCompare

Description:
    Templated Exception Handler Memory Compare function
\*****************************************************************************/
template <size_t size>
inline bool MemCompare( const void* dst, const void* src )
{
    const UINT64*   pSrc    = reinterpret_cast<const UINT64*>(src);
    const UINT64*   pDst    = reinterpret_cast<const UINT64*>(dst);
    size_t          cmpSize = size;

    // align for sizes larger than 128 due to double clock penalty for mov
    //  if one of the memory access is not 64 bit aligned. See Intel Programming
    //  manual Volume 1, Section 4.1.1
#ifdef _WIN64
    if( size > DUAL_CACHE_SIZE )
    {
        // align data to 64 bit if necessary, calculate number of bytes to offset
        size_t alignSrc = (size_t)( (UINT_PTR)pSrc & ( sizeof(QWORD) - 1 ) );
        size_t alignDst = (size_t)( (UINT_PTR)pDst & ( sizeof(QWORD) - 1 ) );

        // alignments are power of 2 : 1 byte, 2 bytes, 4 bytes
        if( alignSrc > 0 && alignDst > 0 )
        {
            cmpSize -= alignDst; // take off our alignment

            const UINT32* uSrc = reinterpret_cast<const UINT32*>(pSrc);
            const UINT32* uDst = reinterpret_cast<const UINT32*>(pDst);

            if( alignDst >= sizeof(UINT32) )
            {
                if( (*uSrc - *uDst) != 0 )
                {
                    return false;
                }

                alignDst    -= sizeof(UINT32);
                uSrc        += 1;
                uDst        += 1;
            }

            const WORD* wSrc = reinterpret_cast<const WORD*>(uSrc);
            const WORD* wDst = reinterpret_cast<const WORD*>(uDst);

            if( alignDst >= sizeof(WORD) )
            {

                if( (*wSrc - *wDst) != 0 )
                {
                    return false;
                }

                alignDst    -= sizeof(WORD);
                wSrc        += 1;
                wDst        += 1;
            }

            const BYTE* bSrc = reinterpret_cast<const BYTE*>(wSrc);
            const BYTE* bDst = reinterpret_cast<const BYTE*>(wDst);

            if( alignDst >= sizeof(BYTE) )
            {
                if( (*bSrc - *bDst) != 0 )
                {
                    return false;
                }

                alignDst    -= sizeof(BYTE);
                bSrc        += 1;
                bDst        += 1;
            }

            pSrc    = reinterpret_cast<const UINT64*>(bSrc);
            pDst    = reinterpret_cast<const UINT64*>(bDst);
        }
    }
#endif

    // compare memory by tier until we find a difference
    size_t cnt = cmpSize >> 3;

    for( size_t i = 0; i < cnt; i++ )
    {
        if( (*pSrc - *pDst) != 0 )
        {
            return false;
        }

        pSrc += 1;
        pDst += 1;
    }

    cmpSize -= (cnt * sizeof(UINT64));

    if( cmpSize == 0 )
    {
        return true;
    }

    const UINT32* dSrc   = reinterpret_cast<const UINT32*>(pSrc);
    const UINT32* dDst   = reinterpret_cast<const UINT32*>(pDst);

    if( cmpSize >= sizeof(UINT32) )
    {
        if( (*dSrc - *dDst) != 0 )
        {
            return false;
        }

        dSrc    += 1;
        dDst    += 1;
        cmpSize -= sizeof(UINT32);
    }

    if( cmpSize == 0 )
    {
        return true;
    }

    const WORD* wSrc  = reinterpret_cast<const WORD*>(dSrc);
    const WORD* wDst  = reinterpret_cast<const WORD*>(dDst);

    if( cmpSize >= sizeof(WORD) )
    {
        if( (*wSrc - *wDst) != 0 )
        {
            return false;
        }

        wSrc    += 1;
        wDst    += 1;
        cmpSize -= sizeof(WORD);
    }

    if (cmpSize == 0 )
    {
        return true;
    }

    const BYTE* bSrc  = reinterpret_cast<const BYTE*>(wSrc);
    const BYTE* bDst  = reinterpret_cast<const BYTE*>(wDst);

    if( (*bSrc - *bDst) != 0 )
    {
        return false;
    }

    return true;
}

template <>
inline bool MemCompare<1>( const void* dst, const void* src )
{
    return (*(BYTE*)dst == *(BYTE*)src);
}

template <>
inline bool MemCompare<2>( const void* dst, const void* src )
{
    return (*(WORD*)dst == *(WORD*)src);
}

template <>
inline bool MemCompare<4>( const void* dst, const void* src )
{
    return (*(UINT32*)dst == *(UINT32*)src);
}

template <>
inline bool MemCompare<8>( const void* dst, const void* src )
{
    return (*(UINT64*)dst == *(UINT64*)src);
}

/*****************************************************************************\
Inline Function:
    IsEqual

Description:
    Compares two values for equality
\*****************************************************************************/
template <class Type>
__forceinline bool IsEqual( const Type& a, const Type& b )
{
    return iSTD::MemCompare<sizeof(Type)>( &a, &b );
}

/*****************************************************************************\
Inline Function:
    IsTagComplete

Description:
    Determines is the surface tag has reached completion
\*****************************************************************************/
template <class Type>
__forceinline bool IsTagComplete( const Type hwTag, const Type swTag, const Type resTag )
{
    return ( ( resTag == hwTag ) || ( ( resTag - hwTag ) > ( swTag - hwTag ) ) );
}

/*****************************************************************************\

Inline Function:
    Hash

Description:
    Calculates hash from sequence of 32-bit values.

    Jenkins 96-bit mixing function with 32-bit feedback-loop and 64-bit state.

    All magic values are DWORDs of SHA2-256 mixing data:
    0x428a2f98 0x71374491 0xb5c0fbcf 0xe9b5dba5
    0x3956c25b 0x59f111f1 0x923f82a4 0xab1c5ed5

    Could be speed-up by processing 2 or 3 DWORDs at time.

\*****************************************************************************/
#define HASH_JENKINS_MIX(a,b,c)    \
{                                  \
    a -= b; a -= c; a ^= (c>>13);  \
    b -= c; b -= a; b ^= (a<<8);   \
    c -= a; c -= b; c ^= (b>>13);  \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16);  \
    c -= a; c -= b; c ^= (b>>5);   \
    a -= b; a -= c; a ^= (c>>3);   \
    b -= c; b -= a; b ^= (a<<10);  \
    c -= a; c -= b; c ^= (b>>15);  \
}

inline QWORD Hash( const DWORD *data, DWORD count )
{
    DWORD   a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
    while( count-- )
    {
        a ^= *(data++);
        HASH_JENKINS_MIX( a, hi, lo );
    }
    return (((QWORD)hi)<<32)|lo;
}

// This is a modified version of the hash combine function from boost
// https://www.boost.org/doc/libs/1_84_0/boost/intrusive/detail/hash_combine.hpp
// 0xffffffffffffffff / 0x517cc1b727220a95 = M_PI
inline QWORD HashCombine(const QWORD hash1, const QWORD hash2 )
{
    return hash1 ^ (hash2 + 0x517cc1b727220a95 + (hash1 << 6) + (hash1 >> 2));
}

inline QWORD AddEntryToHash(const DWORD data, const QWORD hash)
{
    return HashCombine(hash, Hash(&data, 1));
}

struct HashJenkinsMixReturnAggregate
{
    HashJenkinsMixReturnAggregate(DWORD _a, DWORD _hi, DWORD _lo) :
        a(_a),
        hi(_hi),
        lo(_lo)
    {}

    DWORD a;
    DWORD hi;
    DWORD lo;
};

inline
HashJenkinsMixReturnAggregate HashJenkinsMix(DWORD a, DWORD hi, DWORD lo)
{
    HASH_JENKINS_MIX(a, hi, lo);
    return HashJenkinsMixReturnAggregate(a, hi, lo);
}

__forceinline
void HashNext(DWORD &a, DWORD &hi, DWORD &lo, DWORD data)
{
    a ^= data;
    HashJenkinsMixReturnAggregate result = HashJenkinsMix(a, hi, lo);
    a = result.a;
    hi = result.hi;
    lo = result.lo;
}

__forceinline
void HashFirst(DWORD &a, DWORD &hi, DWORD &lo, DWORD data)
{
    a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
    HashNext(a, hi, lo, data);
}


/*****************************************************************************\
Inline Function:
HashFromBuffer

Description:
    Calculates hash from data buffer.
Input:
    data - pointer to the data buffer
    count - size of the buffer in bytes
\*****************************************************************************/
inline QWORD HashFromBuffer(const char *data, size_t count)
{
    DWORD a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
    const DWORD *dataDw = reinterpret_cast<const DWORD*>(data);
    size_t countDw = (DWORD)(count / sizeof(DWORD));

    while (countDw--)
    {
        a ^= *(dataDw++);
        HASH_JENKINS_MIX(a, hi, lo);
    }
    // If buffer size isn't miltiply of DWORD we have to use last bytes to calculate hash
    if (count % sizeof(DWORD) != 0)
    {
        DWORD lastDw = 0;
        char *lastBytesBuff = reinterpret_cast<char*>(&lastDw);
        const size_t restBytesCount = count % sizeof(DWORD);

        for (unsigned int i = 0; i < restBytesCount; i++)
        {
            lastBytesBuff[i] = data[count - restBytesCount + i];
        }
        a ^= lastDw;
        HASH_JENKINS_MIX(a, hi, lo);
    }
    return (((QWORD)hi) << 32) | lo;
}
#undef HASH_JENKINS_MIX

/*****************************************************************************\

Inline Function:
    Hash32b

Description:
    Calculates 32 bit hash from 32 bit value.

    badc0ded hash - self-reversible, 32->32 mapping, good avalanche
    4 asm instructions in x86, 0 maps to 0

\*****************************************************************************/
inline DWORD Hash32b( const DWORD value )
{
#if defined _WIN32
    return   ( _byteswap_ulong( value * 0xbadc0ded ) ^ 0xfecacafe ) * 0x649c57e5;
#else
    return ( __builtin_bswap32( value * 0xbadc0ded ) ^ 0xfecacafe ) * 0x649c57e5;
#endif
}

/*****************************************************************************\

Inline Function:
    Hash32b

Description:
    Calculates 32 bit hash from sequence of 32 bit values.

    badc0ded hash - self-reversible, 32->32 mapping, good avalanche
    4 asm instructions in x86, 0 maps to 0

\*****************************************************************************/
inline DWORD Hash32b( const DWORD *data, DWORD count )
{
    DWORD hash = 0xdeadf00d;

    while( count-- )
    {
        hash ^= Hash32b( *( data + count ) );
    }

    return hash;
}

/*****************************************************************************\

Inline Function:
    BitCount

Description:
    Returns the number of bits set to 1 in the input 32-bit number.

\*****************************************************************************/
inline DWORD BitCount( DWORD v )
{
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x1010101) >> 24;
}

/*****************************************************************************\

Inline Function:
    BitCount64

Description:
    Returns the number of bits set to 1 in the input 64-bit number.

\*****************************************************************************/
inline DWORD BitCount64( unsigned long long v )
{
    v -= ( v >> 1 ) & 0x5555555555555555ULL;
    v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
    v = ((v + (v >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL;
    return static_cast<DWORD>( v >> 56 );
}

/*****************************************************************************\

Inline Function:
    BitReverse

Description:
    Reverse a 32-bit bitfield in a number.

\*****************************************************************************/
inline DWORD BitReverse( DWORD v )
{
    // swap odd and even bits
    v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
    // swap consecutive pairs
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    // swap nibbles
    v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
    // swap bytes
    v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    // swap words
    v = ( v >> 16             ) | ( v               << 16);
    return v;
}

/*****************************************************************************\
Inline Function:
    PtrAdd

Description:
    Type-safe addition of a pointer and a scalar (in bytes).
\*****************************************************************************/
template<typename Type>
__forceinline Type* PtrAdd( Type* ptr, const size_t numBytes )
{
    return (Type*)( ((BYTE*)ptr) + numBytes );
}

/*****************************************************************************\
Inline Function:
    FixedSIntToInt

Description:
    Converts a fixed signed integer value into a native signed int
\*****************************************************************************/
__forceinline int FixedSIntToInt( DWORD value, DWORD size )
{
    if( value & BIT(size+1) )
    {
        return -1 * (value + 1);
    }

    return value;
}

} // iSTD
