/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"
#include <math.h> // for powf()

namespace iSTD
{
/*****************************************************************************\
Inline Function:
    FloatToLong

Description:
    converts a float to long using SSE, avoiding FP stack stall
\*****************************************************************************/
__forceinline long FloatToLong( const float value )
{
#if defined(_WIN32) && defined(_MSC_VER)
    return _mm_cvtsi128_si32( _mm_cvttps_epi32( _mm_set_ps1( value ) ) );
#else
    return (long)value;
#endif
}

/*****************************************************************************\
Inline Function:
    Ceiling

Description:
    Rounds a float up to the next integer value
\*****************************************************************************/
__forceinline long Ceiling( const float value )
{
    long roundVal = FloatToLong( value );
    if( ( value - roundVal ) != 0 )
    {
        return roundVal + 1;
    }
    return roundVal;
}

/*****************************************************************************\
Inline Function:
    Scale

Description:
    Scales a [0.0,1.0] float to [0,max] integer
\*****************************************************************************/
__forceinline DWORD Scale( float value, DWORD max )
{
    ASSERT( CheckLimits( value, 0.0f, 1.0f ) );
    return ( (DWORD)FloatToLong( (value) * (float)(max) ) );
}

/*****************************************************************************\
Inline Function:
    Normalize

Description:
    Normalize the floating-point value with the range [min,max] to [0.0f,1.0f]
\*****************************************************************************/
__forceinline float Normalize( float value, float min, float max )
{
    return ( value - min ) / ( max - min );
}

/*****************************************************************************\
Inline Function:
    Lerp

Description:
    Linear-Interpolation
\*****************************************************************************/
__forceinline float Lerp( float x, float y, float z )
{
    return ( ( x * ( 1 - z ) ) + ( y * z ) );
}

/*****************************************************************************\
Inline Function:
    FloatToFixed

Description:
    Converts a floating-point number to the specified fixed-point number
\*****************************************************************************/
template <class Type>
__forceinline Type FloatToFixed(
    float value,
    const int whole,
    const int fractional, 
    const int round = 0 )
{
    ASSERT( fractional + whole <= 32 );

    // Optional floating point rounding precision
    value += ( round != 0 )
        ? 0.5f * ( 1.0f / (float)( 1 << round ) )
        : 0;

    Type fixed = (Type)FloatToLong( value * (float)( 1 << fractional ) );

#ifdef _DEBUG
    DWORD mask = 0xffffffff << ( whole + fractional );
    ASSERT( 
        (( fixed >= 0 ) && (( fixed & mask ) == 0 )) ||
        (( fixed <  0 ) && (( fixed & mask ) == mask )) );
#endif

    return fixed;
}

/*****************************************************************************\
Inline Function:
    FixedToFloat

Description:
    Converts the specified fixed-point number to a floating-point number
\*****************************************************************************/
template <class Type>
__forceinline float FixedToFloat(
      Type fixed,
      const int whole,
      const int fractional )
{
    ASSERT( fractional + whole <= 32 );

    //check sign bit if negative
    if (fixed >> (fractional + whole) != 0)
    {
        // pad the sign from left to 32 bit
        fixed |= (0xffffffff << (fractional + whole));
    }

    float value = (float)fixed / (float)( 1 << fractional );

    return value;
}

/*****************************************************************************\
Inline Function:
    Float32ToSnorm

Description:
    Converts a 32bit float to a bitcount size SNORM value
\*****************************************************************************/
template< DWORD bitcount >
inline DWORD Float32ToSnorm( const float value )
{
    ASSERT( bitcount <= 32 );

    long snormValue = 0;

    FLOAT32 f32;
    f32.value.f = value;

    // NaN -> 0
    if( f32.exponent == BITMASK( 8 ) && 
        f32.fraction != 0 )
    {
        snormValue = 0;
    }
    else
    {
        const bool isPosInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 0;
        const bool isNegInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 1;

        // Clamp > 1.0f || +Inf -> 1.0f
        // Clamp < -1.0f || -Inf -> -1.0f
        if( f32.value.f > 1.0f || isPosInfinity )
        {
            f32.value.f = 1.0f;
        }
        else if ( f32.value.f < -1.0f || isNegInfinity )
        {
            f32.value.f = -1.0f;
        }

        // Convert float scale to integer scale
        f32.value.f *= (float)( ( 0x1 << ( bitcount - 1 ) ) - 1 );

        // Convert to integer by rounding and dropping
        // the fractional part
        f32.value.f = ( f32.value.f >= 0 )
            ? f32.value.f + 0.5f
            : f32.value.f - 0.5f;
        snormValue = FloatToLong( f32.value.f );
    }

    return (DWORD)snormValue;
}

/*****************************************************************************\
Inline Function:
Float32ToSnormSM

Description:
Converts a 32bit float to a bitcount size SNORM value in Sign Magnitude format (SM)
\*****************************************************************************/
template< DWORD bitcount >
inline DWORD Float32ToSnormSM( const float value )
{
    ASSERT( bitcount <= 32 );

    long snormValue = 0;

    FLOAT32 f32;
    f32.value.f = value;

    // NaN -> 0
    if( f32.exponent == BITMASK( 8 ) && 
        f32.fraction != 0 )
    {
        snormValue = 0;
    }
    else
    {
        const bool isPosInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 0;
        const bool isNegInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 1;

        // Clamp > 1.0f || +Inf -> 1.0f
        // Clamp < -1.0f || -Inf -> -1.0f
        if( f32.value.f > 1.0f || isPosInfinity )
        {
            f32.value.f = 1.0f;
        }
        else if ( f32.value.f < -1.0f || isNegInfinity )
        {
            f32.value.f = -1.0f;
        }

        // Convert float scale to integer scale
        f32.value.f *= (float)( ( 0x1 << ( bitcount - 1 ) ) - 1 );

        // Convert to integer by rounding and dropping
        // the fractional part
        f32.value.f = ( f32.value.f >= 0 )
            ? f32.value.f + 0.5f
            : f32.value.f - 0.5f;
        snormValue = FloatToLong( f32.value.f );

        if(snormValue < 0)
        {
            snormValue *= -1;
            snormValue = snormValue | (0x1 << ( bitcount - 1 ) );
        }
    }

    return (DWORD)snormValue;
}

/*****************************************************************************\
Inline Function:
    LinearToSRGB

Description:
    Converts a 32bit float in linear space to SRGB space
\*****************************************************************************/
inline float LinearToSRGB( const float value )
{
    float srgbValue = value;

    ASSERT( value >= 0.0f && value <= 1.0f );

    if (value < 0.0f)
    {
        srgbValue = 0.0f;
    }
    else if( value < 0.0031308f )
    {
        srgbValue = 12.92f * value;
    }
    else if (value < 1.0f)
    {
        srgbValue = ( 1.055f * powf( value, (1.0f/2.4f) ) ) - 0.055f;
    }
    else
    {
        srgbValue = 1.0f;
    }

    ASSERT( srgbValue >= 0.0f && srgbValue <= 1.0f );

    return srgbValue;
}

/*****************************************************************************\
Inline Function:
    SRGBToLinear

Description:
    Converts a 32bit float in SRGB space to linear space
\*****************************************************************************/
inline float SRGBToLinear( const float value )
{
    float linearValue = value;

    ASSERT( value >= 0.0f && value <= 1.0f );

    if( linearValue <= 0.04045f )
    {
        linearValue = value / 12.92f;
    }
    else
    {
        linearValue = powf( ( ( value + 0.055f ) / 1.055f ), 2.4f );
    }

    ASSERT( linearValue >= 0.0f && linearValue <= 1.0f );

    return linearValue;
}

/*****************************************************************************\
Inline Function:
    Float32ToUnorm

Description:
    Converts a 32bit float to a bitcount size UNORM value
\*****************************************************************************/
template< DWORD bitcount >
inline DWORD Float32ToUnorm( const float value )
{
    ASSERT( bitcount <= 32 );

    DWORD unormValue = 0;

    FLOAT32 f32;
    f32.value.f = value;

    // NaN -> 0
    if( f32.exponent == BITMASK( 8 ) && 
        f32.fraction != 0 )
    {
        unormValue = 0;
    }
    else
    {
        const bool isPosInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 0;
        const bool isNegInfinity = 
            f32.exponent == BITMASK( 8 ) &&
            f32.fraction == 0 &&
            f32.sign == 1;

        // Clamp > 1.0f || +Inf -> 1.0f
        // Clamp < 0.0f || -Inf -> 0.0f
        if( f32.value.f > 1.0f || isPosInfinity )
        {
            f32.value.f = 1.0f;
        }
        else if ( f32.value.f < 0.0f || isNegInfinity )
        {
            f32.value.f = 0.0f;
        }

        // Convert float scale to integer scale
        f32.value.f *= BITMASK( bitcount );

        // Convert to integer by rounding and dropping
        // fractional bits
        f32.value.f += 0.5f;
        unormValue = (DWORD)FloatToLong( f32.value.f );
        unormValue = (DWORD)f32.value.f;
        unormValue = Min( (DWORD)unormValue, (DWORD)BITMASK( bitcount ) );
    }

    return unormValue;
}

/*****************************************************************************\
Inline Function:
    Float32ToFloat16

Description:
    Converts a 32bit float to a 16bit float
\*****************************************************************************/
inline unsigned short Float32ToFloat16( const float value )
{
    FLOAT16 f16;
    f16.value.u = 0;

    FLOAT32 f32;
    f32.value.f = value;

    // +/-0 32bit -> +/- 0 16bit
    if( f32.exponent == 0 &&
        f32.fraction == 0 )
    {
        f16.exponent = 0;
        f16.fraction = 0;
        f16.sign = f32.sign;
    }
    // NaN 32bit -> NaN 16bit
    else if( f32.exponent == BITMASK( 8 ) &&
        f32.fraction != 0 )
    {
        f16.exponent = BITMASK( 5 );
        f16.fraction = 0x1 << 9;
        f16.sign = 1;
    }
    // +/-Inf 32bit -> +/-Inf 16bit
    else if (
        f32.exponent == BITMASK( 8 ) &&
        f32.fraction == 0 )
    {
        f16.exponent = BITMASK( 5 );
        f16.fraction = 0;
        f16.sign = f32.sign;
    }
    else
    {
        const long ExpBias16    = 31 / 2;
        const long ExpBais32    = 255 / 2;
        const long expUnbiased  = f32.exponent - ExpBais32;

        // 32bit normalized value out of minimum range of 16bit capacity
        // resulting in minimum non-denorm 16bit value
        if ( expUnbiased <= -25 )
        {
            f16.exponent = 0;
            f16.fraction = 0;
        }
        // 32bit normalized value within the 16bit denormalized unbiased
        // exponent range
        else if( expUnbiased > -25 && expUnbiased < -14 )
        {
            long adjustedUnbiasedExp = expUnbiased;
            unsigned long adjustedFranction = f32.fraction;

            // Shift the implicit 1 into the fraction, making implicit 0
            // as denormalized format dictates
            adjustedFranction >>= 1;
            adjustedFranction |= ( 0x1 << 22 );

            // Round off the fraction until the unbiased exponent is
            // within a denormalized representable range
            unsigned long denormShiftAmount = -1 * ( adjustedUnbiasedExp + 15 );
            ASSERT( denormShiftAmount < 10 );
            adjustedFranction >>= denormShiftAmount;
            adjustedUnbiasedExp += denormShiftAmount;

            f16.exponent = adjustedUnbiasedExp + ExpBias16;
            f16.fraction = ( adjustedFranction ) >> ( 23 - 10 );

            // Assert that the 16bit is actually denormalized. The result
            // should never be 0 because of the addition of the implicit 1
            ASSERT( f16.exponent == 0 && f16.fraction != 0 );
        }
        // 32bit value out of maximum dynamic range of 16bit capacity
        // resulting in maximum 16bit value
        else if( expUnbiased > 15 )
        {
            f16.exponent = 15 + ExpBias16;
            f16.fraction = BITMASK( 10 );
        }
        // Otherwise, normalized down conversion falls within normalized 
        // 16bit range
        else
        {
            f16.exponent = expUnbiased + ExpBias16;
            f16.fraction = ( f32.fraction ) >> ( 23 - 10 );
        }

        // Sign is preserved under any circumstance
        f16.sign = f32.sign;
    }

    return f16.value.u;
}

} // iSTD
