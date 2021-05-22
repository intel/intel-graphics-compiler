/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <limits.h>
#include <cmath>

namespace iSTD
{
/*****************************************************************************\
Constants:
    FPU_FLOAT32_*

Description:
    Binary representation of 32-bit floating point specials.
    FPU_FLOAT32_COMPUTE special value can be used in result tables to mark
    cases, where final value should be computed normally.
\*****************************************************************************/
const DWORD FPU_FLOAT32_NAN         = 0x7FFFFFFF;
const DWORD FPU_FLOAT32_NEG_INF     = 0xFF800000;
const DWORD FPU_FLOAT32_POS_INF     = 0x7F800000;
const DWORD FPU_FLOAT32_NEG_ZERO    = 0x80000000;
const DWORD FPU_FLOAT32_POS_ZERO    = 0x00000000;
const DWORD FPU_FLOAT32_COMPUTE     = 0xFFFFFFFF;
const DWORD FPU_FLOAT32_ONE         = (DWORD) 0x3F800000;
const DWORD FPU_FLOAT32_MINUS_ONE   = (DWORD) 0xBF800000;


/*****************************************************************************\
Enumeration:
    FPU_FLOAT_CLASS

Description:
    Classes of floating point numbers.
    (+0, -0, +finite, -finite, +Inf, -Inf, NaN, -denorm, +denorm)
\*****************************************************************************/
enum FPU_FLOAT_CLASS {
    FPU_FLOAT_CLASS_NEG_INF      = 0,
    FPU_FLOAT_CLASS_NEG_FINITE   = 1,
    FPU_FLOAT_CLASS_NEG_DENORM   = 2,
    FPU_FLOAT_CLASS_NEG_ZERO     = 3,
    FPU_FLOAT_CLASS_POS_ZERO     = 4,
    FPU_FLOAT_CLASS_POS_DENORM   = 5,
    FPU_FLOAT_CLASS_POS_FINITE   = 6,
    FPU_FLOAT_CLASS_POS_INF      = 7,
    FPU_FLOAT_CLASS_NAN          = 8,
    NUM_FPU_FLOAT_CLASSES        = 9
};

/*****************************************************************************\
Inline Function:
    Float32GetClass

Description:
    Returns class (+0, -0, +finite, -finite, +Inf, -Inf, NaN) of 32-bit float.
\*****************************************************************************/
inline FPU_FLOAT_CLASS Float32GetClass( const float f )
{
    FLOAT32 f32;
    f32.value.f = f;

    switch( f32.value.u )
    {
    case FPU_FLOAT32_POS_ZERO:  return FPU_FLOAT_CLASS_POS_ZERO;
    case FPU_FLOAT32_NEG_ZERO:  return FPU_FLOAT_CLASS_NEG_ZERO;
    case FPU_FLOAT32_POS_INF:   return FPU_FLOAT_CLASS_POS_INF;
    case FPU_FLOAT32_NEG_INF:   return FPU_FLOAT_CLASS_NEG_INF;
    default:                    break;
    }

    if( f32.exponent == 0xFF )
    {
        return FPU_FLOAT_CLASS_NAN;
    }
    else if( f32.exponent == 0x00 )
    {
        if( f32.sign == 0 )
        {
            return FPU_FLOAT_CLASS_POS_DENORM;
        }
        else
        {
            return FPU_FLOAT_CLASS_NEG_DENORM;
        }
    }

    if( f32.sign )
    {
        return FPU_FLOAT_CLASS_NEG_FINITE;
    }

    return FPU_FLOAT_CLASS_POS_FINITE;
}

/*****************************************************************************\
Inline Function:
    Float32IsInfinity

Description:
    Returns true if class is +Inf or -Inf of 32-bit float.
\*****************************************************************************/
inline bool Float32IsInfinity( const float f )
{
    FPU_FLOAT_CLASS fClass = Float32GetClass( f );

    return ( fClass == FPU_FLOAT_CLASS_POS_INF ) ||
           ( fClass == FPU_FLOAT_CLASS_NEG_INF );
}

/*****************************************************************************\
Inline Function:
    Float32IsDenorm

Description:
    Returns true if class is +Denorm or -Denorm.
\*****************************************************************************/
inline bool Float32IsDenorm( const float f )
{
    FPU_FLOAT_CLASS fClass = Float32GetClass( f );

    return ( fClass == FPU_FLOAT_CLASS_NEG_DENORM ) ||
           ( fClass == FPU_FLOAT_CLASS_POS_DENORM );
}

/*****************************************************************************\

Inline Function:
    Float32IsFinite

Description:
    Returns true if f is finite: not +/-INF, and not NaN.
\*****************************************************************************/
inline bool Float32IsFinite( const float f )
{
    FPU_FLOAT_CLASS fClass = Float32GetClass( f );

    return ( fClass != FPU_FLOAT_CLASS_NAN )     &&
           ( fClass != FPU_FLOAT_CLASS_NEG_INF ) &&
           ( fClass != FPU_FLOAT_CLASS_POS_INF );
}

/*****************************************************************************\
Inline Function:
    IsFPZero

Description:
    Returns true if the argument x seen as a 32-bit IEEE754 floating point
    number is either positive or negative zero  +0.0, -0.0.

Input:
    dword value that will be interpreted as a binary32 representation
    of single-precision floating point value.

Output:
    True if the value represents either positive or negative float zero.

\*****************************************************************************/    
inline bool IsFPZero( const DWORD x )
{
    return ( x == iSTD::FPU_FLOAT32_POS_ZERO ) || 
           ( x == iSTD::FPU_FLOAT32_NEG_ZERO );
}

/*****************************************************************************\
Inline Function:
    Float32SafeAdd

Description:
    Performs addition taking care of floating point specials in software.
\*****************************************************************************/
inline float Float32SafeAdd( const float arg1, const float arg2, const bool denormRetain  )
{
    // Table for handling IEEE 754 specials in addition
    //
    //  a + b       -Inf    -X      -0      +0      +X      +Inf    NaN
    //
    //  -Inf        -Inf    -Inf    -Inf    -Inf    -Inf    NaN     NaN
    //  -X          -Inf    <add>   <add>   <add>   <add>   +Inf    NaN
    //  -0          -Inf    <add>   -0      +0      <add>   +Inf    NaN
    //  +0          -Inf    <add>   +0      +0      <add>   +Inf    NaN
    //  +X          -Inf    <add>   <add>   <add>   <add>   +Inf    NaN
    //  +Inf        NaN     +Inf    +Inf    +Inf    +Inf    +Inf    NaN
    //  NaN         NaN     NaN     NaN     NaN     NaN     NaN     NaN
    //

    static const DWORD RESULT[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf                  -X                    -denorm               -0                    +0                    +denorm               +X                    +Inf                  NaN
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF  , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // -Inf
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE  , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // -X
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_ZERO , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // -denorm
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_ZERO , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // -0
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_ZERO , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +0
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_ZERO , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +denorm
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE  , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +X
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF  , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +Inf
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // NaN
    };

    const FPU_FLOAT_CLASS t1 = Float32GetClass( arg1 );
    const FPU_FLOAT_CLASS t2 = Float32GetClass( arg2 );

    FLOAT32 f32;
    f32.value.u = RESULT[ t1 ][ t2 ];

    bool computeDenorms = ( denormRetain && ( Float32IsDenorm( arg1 ) || Float32IsDenorm( arg2 ) ) );

    if( ( f32.value.u == FPU_FLOAT32_COMPUTE ) || ( computeDenorms ) )
    {
        return arg1 + arg2;
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    Float32SafeSubtract

Description:
    Performs subtraction taking care of floating point specials in software.
\*****************************************************************************/
inline float Float32SafeSubtract( const float arg1, const float arg2, const bool denormRetain )
{
    FLOAT32 f32;
    f32.value.f = arg2;

    // flip sign bit
    f32.sign ^= 1;

    return Float32SafeAdd( arg1, f32.value.f, denormRetain );
}

/*****************************************************************************\
Inline Function:
    Float32SafeMultiply

Description:
    Performs multiplication taking care of floating point specials in software.
\*****************************************************************************/
inline float Float32SafeMultiply( const float arg1, const float arg2, const bool denormRetain )
{
    // Table for handling IEEE 754 specials in multiplication
    //
    //  a * b       -Inf    -X      -0      +0      +X      +Inf    NaN
    //
    //  -Inf        +Inf    +Inf    NaN     NaN     -Inf    -Inf    NaN
    //  -X          +Inf    <mul>   +0      -0      <mul>   -Inf    NaN
    //  -0          NaN     +0      +0      -0      -0      NaN     NaN
    //  +0          NaN     -0      -0      +0      +0      NaN     NaN
    //  +X          -Inf    <mul>   -0      +0      <mul>   +Inf    NaN
    //  +Inf        -Inf    -Inf    NaN     NaN     +Inf    +Inf    NaN
    //  NaN         NaN     NaN     NaN     NaN     NaN     NaN     NaN
    //

    static const DWORD RESULT[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf                  -X                    -denorm               -0                    +0                    +denorm               +X                    +Inf                  NaN
        { FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NAN      },  // -Inf
        { FPU_FLOAT32_POS_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NAN      },  // -X
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // -denorm
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // -0
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // +0
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // +denorm
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +X
        { FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN      },  // +Inf
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // NaN
    };

    FPU_FLOAT_CLASS t1 = Float32GetClass( arg1 );
    FPU_FLOAT_CLASS t2 = Float32GetClass( arg2 );

    FLOAT32 f32;
    f32.value.u = RESULT[ t1 ][ t2 ];

    bool computeDenorms = ( denormRetain && ( Float32IsDenorm( arg1 ) || Float32IsDenorm( arg2 ) ) );

    if( ( f32.value.u == FPU_FLOAT32_COMPUTE ) || ( computeDenorms ) )
    {
        return arg1 * arg2;
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    Float32SafeFMA

Description:
    Performs fused mutliply and add taking care of floating point specials in 
    software.

    This is machine generated code provided by SSG.

\*****************************************************************************/
inline float Float32SafeFMA( const float a, const float b, const float c )
{
    const DWORD _own_large_value_32[] = { 0x71800000, 0xf1800000 };
    const DWORD _own_small_value_32[] = { 0x0d800000, 0x8d800000 };
    const DWORD _ones[]               = { 0x3f800000, 0xbf800000 };

    DWORD ux = 0;
    DWORD uy = 0; 
    DWORD uz = 0;
    DWORD ur = 0;
    DWORD xbits = 0;
    DWORD ybits = 0; 
    DWORD zbits = 0;
    DWORD uhi = 0;
    DWORD ulo = 0;
    DWORD vhi = 0;
    DWORD vlo = 0;
    DWORD remain = 0;
    DWORD temp = 0;
    DWORD L_mask = 0;
    DWORD R_mask = 0;

    INT zsign = 0;
    INT rsign = 0;
    INT xexp = 0; 
    INT yexp = 0; 
    INT zexp = 0; 
    INT rexp = 0;
    INT carry = 0;
    INT borrow = 0;
    INT rm = 0;
    INT shift = 0;
    INT L_shift = 0;
    INT R_shift = 0;

    UINT64 ubits = 0;
    float resultf = 0;
    float tv = 0;
    float x = a;
    float y = b;
    float z = c;

    // Set to round to nearest even.
    rm = 0;        

    ux = FLOAT32( x >= 0.0f ? x : -x ).value.u;
    uy = FLOAT32( y >= 0.0f ? y : -y ).value.u;;
    uz = FLOAT32( z >= 0.0f ? z : -z ).value.u;;

    int cond1 = ( ux == 0 ) | 
        ( ux >= 0x7f800000 ) | 
        ( ux == 0x3f800000 ) |
        ( uy == 0 ) | 
        ( uy >= 0x7f800000 ) | 
        ( uy == 0x3f800000 ) |
        ( uz == 0 ) | 
        ( uz >= 0x7f800000 );

    if( cond1 != 0 )
    {
        if(  Float32IsInfinity( z ) && 
            !Float32IsInfinity( x ) && 
            !Float32IsInfinity( y ) )
        {
            resultf = ( z + x ) + y;
        }
        else
        {
            resultf = x * y + z;
        }

        return resultf;
    }

    xexp = (int)( ux >> 23 );
    yexp = (int)( uy >> 23 );
    zexp = (int)( uz >> 23 );

    xbits = 0x00800000 | ( ux & 0x007fffff );
    ybits = 0x00800000 | ( uy & 0x007fffff );
    zbits = 0x00800000 | ( uz & 0x007fffff );

  
    rsign = ( FLOAT32(x).value.s ^ FLOAT32(y).value.s ) & 0x80000000;
    rexp  = ( xexp + yexp ) - 0x7F;
    ubits = (UINT64)xbits * ybits;

    if( (DWORD) ( ubits >> 32 ) & 0x00008000 )
    {
        uhi = (DWORD)( ubits >> 24 );
        ulo = ( (DWORD)ubits << 8 );
        rexp++;
    }
    else
    {
        uhi = (DWORD)( ubits >> 23 );
        ulo = ( (DWORD)ubits << 9 );
    }

    int cond2 = ( rexp > zexp ) | 
                ( ( rexp == zexp ) & ( uhi >= zbits ) );

    if( cond2 != 0 )
    {
        shift = ( rexp - zexp );
        vhi = zbits;
        vlo = 0;
        zsign = FLOAT32(z).value.s & 0x80000000;
    }
    else
    {
        shift = ( zexp - rexp );
        rexp = zexp;
        vhi = uhi;
        vlo = ulo;
        uhi = zbits;
        ulo = 0;
        zsign = rsign;
        rsign = FLOAT32(z).value.s & 0x80000000;
    }

    remain = 0;
    if( shift != 0 )
    {
        if( shift < 32 )
        {
            L_shift = 32 - shift;
            R_shift = shift - 0;
            L_mask = ~( 0xffffffffu >> R_shift );
            remain = ( vlo << L_shift );
            vlo = ( ( vhi << L_shift ) & L_mask) | ( vlo >> R_shift );
            vhi = ( vhi >> R_shift );
        }
        else if( shift < 64 )
        {
            L_shift = 64 - shift;
            R_shift = shift - 32;
            L_mask = ~( 0xffffffffu >> R_shift );
            remain = ( ( vhi << L_shift ) & L_mask ) | ( vlo != 0 );
            vlo = ( vhi >> R_shift );
            vhi = 0;
        }
        else
        {
            remain = ( vhi | vlo ) != 0;
            vhi = vlo = 0;
        }
    }

    if( rsign == zsign )
    {
        temp = ulo;
        ulo += vlo;
        carry = ( ulo < temp );
        uhi += ( vhi + carry );

        if ( uhi & 0x01000000 )
        {
            remain = ( uhi << 31 ) | ( ( ulo | remain ) != 0 );
            ur = ( uhi >> 1 ) & 0x007fffff;
            rexp += 1;
        }
        else
        {
            remain = ulo | ( remain != 0 );
            ur = (uhi & 0x007fffff);
        }
    }
    else
    {
        remain = ( 0 - remain );
        borrow = ( remain != 0 );
        temp = ulo;
        ulo -= borrow;
        borrow = ( ulo > temp );
        uhi -= borrow;
        temp = ulo;
        ulo -= vlo;
        borrow = ( ulo > temp );
        uhi -= borrow;
        uhi -= vhi;

        if( uhi != 0 )
        {
            temp = ( uhi << 8 );
            shift = 0;
        }
        else if( ulo != 0 )
        {
            temp = ulo;
            shift = 24;
        }
        else if( remain != 0 )
        {
            temp = remain;
            shift = 24 + 32;
        }
        else
        {
            return FLOAT32( (DWORD)0x00000000 ).value.f;
        }

        shift += clz( temp );

        if( shift < 32 )
        {
            L_shift = shift - 0;
            R_shift = 32 - shift;
            R_mask = ( (DWORD) 1 << L_shift ) - 1;
            ur = ( ( uhi << L_shift ) | (( ulo >> R_shift ) & R_mask ) ) & 0x007fffff;
            remain = ( ulo << L_shift ) | ( remain != 0 );
        }
        else if( shift < 64 )
        {
            L_shift = shift - 32;
            R_shift = 64 - shift;
            R_mask = ( (DWORD) 1 << L_shift ) - 1;
            ur = ( ( ulo << L_shift ) | ( ( remain >> R_shift ) & R_mask ) ) & 0x007fffff;
            remain = ( remain << L_shift );
        }
        else
        {
            L_shift = shift - 64;
            ur = ( remain << L_shift ) & 0x007fffff;
            remain = 0;
        }
        rexp -= shift;
    }

    if( (DWORD) rexp - 1 >= 0xFF - 1 )
    {
        if( rexp >= 0xFF )
        {
            rsign = ( (DWORD)rsign >> 31 );
            if( rsign )
            {
                resultf = tv = FLOAT32(_own_large_value_32[(1)]).value.f * FLOAT32(_own_large_value_32[0]).value.f;
            }
            else
            {
                resultf = tv = FLOAT32(_own_large_value_32[(0)]).value.f * FLOAT32(_own_large_value_32[0]).value.f;
            }

            return resultf;
        }
        else
        {
            //enters here only for rexp = 0
            L_shift = 31;
            R_shift = 1;
            L_mask = ~(0xffffffffu >>  R_shift );
            ur |= 0x00800000;
            remain = ( ( ur << L_shift ) & L_mask ) | ( remain != 0 );
            ur = ( ur >> R_shift );

        }
    }
    else
    {
        ur |= ( rexp << 23 );
    }

    if( remain != 0 )
    {
        tv = ( ( (float *)_ones)[0] + ( (float *)_own_small_value_32)[0] );
        
        int cond3, cond4, cond5, cond6;

        switch( rm )
        {
        case ( 0 << 10 ):
            cond3 = ( ( remain & 0x80000000 ) != 0 ) & ( ( ( ur & 1 ) != 0 ) | 
                    ( ( remain & ~0x80000000 ) != 0 ) );
            if( cond3 != 0 )
            {
                ur++;
                if( ur >= 0x7f800000 )
                {
                    rsign = ( (unsigned)rsign >> 31 );
                    if( rsign )
                    {
                        resultf = tv =
                            ( ( (float *) _own_large_value_32)[1] *
                            ( (float *) _own_large_value_32)[0] );
                    }
                    else
                    {
                        resultf = tv =
                            (((float *) _own_large_value_32)[(0)] *
                            ((float *) _own_large_value_32)[0]);
                    }
                    
                    return resultf;
                }
            }

        case ( 3 << 10 ):
            cond4 = ( ur < 0x00800000 ) | 
                    ( (ur == 0x00800000 ) & ( remain == 0x80000000 ) );

            if( cond4 != 0 )
            {
                tv = ( ( ( float *)_own_small_value_32)[0] * 
                     ( ( float *)_own_small_value_32)[0] );
            }
            break;

        case ( 2 << 10 ):
            cond5 = ( rsign & ( ur < 0x00800000 ) ) | 
                    ( (!rsign) & ( (ur < 0x007fffff ) | ( ( ur == 0x007fffff ) & ( remain < 0x80000000 ) ) ) );

            if( cond5 != 0 )
            {
                tv = ( ( (float *)_own_small_value_32)[0] * 
                       ( (float *)_own_small_value_32)[0] );
            }

            if( !rsign )
            {
                ur++;
                if( ur >= 0x7f800000 )
                {
                    //rsign = ((unsigned) rsign >> 31);
                    resultf = tv = ( ( (float *)_own_large_value_32)[0] * 
                                     ( (float *)_own_large_value_32)[0] );
                    return resultf;
                }
            }
            break;

        case ( 1 << 10 ):
            cond6 = ( !rsign & ( ur < 0x00800000 ) ) | 
                    ( rsign & ( (ur < 0x007fffff ) | ( ( ur == 0x007fffff ) & ( remain < 0x80000000 ) ) ) );

            if( cond6 != 0 )
            {
                tv = ( ( (float *)_own_small_value_32)[0] * 
                       ( (float *)_own_small_value_32)[0] );
            }

            if( rsign )
            {
                ur++;
                if (ur >= 0x7f800000 )
                {
                    //rsign = ((unsigned) rsign >> 31);
                    resultf = tv =
                        ( ( (float *)_own_large_value_32)[1] *
                          ( (float *)_own_large_value_32)[0] );

                    return resultf;
                }
            }
            break;
        }
    }

    resultf = FLOAT32( (DWORD) (rsign | ur ) ).value.f;

    return resultf;
}

/*****************************************************************************\
Inline Function:
    Float32SafeRSQRT

Description:
    Performs correctly rounded single precision reciprocal square root 
    operation taking care of floating point specials in software.
\*****************************************************************************/
inline float Float32SafeRSQRT( const float arg, bool denormRetain )
{
    static const DWORD RESULT[NUM_FPU_FLOAT_CLASSES] =
    {
        FPU_FLOAT32_NAN,       // rsqrt( -inf )    = NaN
        FPU_FLOAT32_NAN,       // rsqrt( -X )      = NaN  //but to be really OK,we should try to maintain the NaN payload
        FPU_FLOAT32_NAN,       // rsqrt( -denorm ) = NaN  //but to be really OK,we should try to maintain the NaN payload
        FPU_FLOAT32_NEG_INF,   // rsqrt( -0 )      = -inf
        FPU_FLOAT32_POS_INF,   // rsqrt( +0 )      = +inf
        FPU_FLOAT32_COMPUTE,   // rsqrt( +denorm)  = computed value
        FPU_FLOAT32_COMPUTE,   // rsqrt( +X )      == computed value
        FPU_FLOAT32_POS_ZERO,  // rsqrt( +inf )    == +0.0
        FPU_FLOAT32_NAN        // rsqrt( NaN )     == NaN
    };

    FPU_FLOAT_CLASS t1 = Float32GetClass( arg );

    FLOAT32 f32;
    f32.value.u = RESULT[ t1 ];

    bool computeDenorms = denormRetain &&  Float32IsDenorm( arg );

    if ( !computeDenorms && t1 == FPU_FLOAT_CLASS_NEG_DENORM ) 
    {
        f32.value.u = FPU_FLOAT32_NEG_INF;
    }
    if ( !computeDenorms && t1 == FPU_FLOAT_CLASS_POS_DENORM )
    {
        f32.value.u = FPU_FLOAT32_POS_INF;
    }

    if( ( f32.value.u == FPU_FLOAT32_COMPUTE ) || ( computeDenorms ) )
    {
        double darg = arg;
        double s = sqrt(darg);      //double-precision square root
        double result = 1.0 / s;    //double-precision division
        return static_cast<float>(result);     //back to floats
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    Float32SafeDivide

Description:
    Performs division taking care of floating point specials in software.
\*****************************************************************************/
inline float Float32SafeDivide( const float arg1, const float arg2, const bool denormRetain )
{
    // Table for handling IEEE 754 specials in division
    //
    //  a / b       -Inf    -X      -0      +0      +X      +Inf    NaN
    //
    //  -Inf        NaN     +Inf    +Inf    -Inf    -Inf    NaN     NaN
    //  -X          +0      <div>   +Inf    -Inf    <div>   -0      NaN
    //  -0          +0      +0      NaN     NaN     -0      -0      NaN
    //  +0          -0      -0      NaN     NaN     +0      +0      NaN
    //  +X          -0      <div>   -Inf    +Inf    <div>   +0      NaN
    //  +Inf        NaN     -Inf    -Inf    +Inf    +Inf    NaN     NaN
    //  NaN         NaN     NaN     NaN     NaN     NaN     NaN     NaN
    //

    static const DWORD RESULT[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf                  -X                    -denorm               -0                    +0                    +denorm               +X                    +Inf                  NaN
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // -Inf
        { FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN      },  // -X
        { FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN      },  // -denorm
        { FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN      },  // -0
        { FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN      },  // +0
        { FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN      },  // +denorm
        { FPU_FLOAT32_NEG_ZERO, FPU_FLOAT32_COMPUTE , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_COMPUTE , FPU_FLOAT32_POS_ZERO, FPU_FLOAT32_NAN      },  // +X
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_NEG_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_POS_INF , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // +Inf
        { FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN     , FPU_FLOAT32_NAN      },  // NaN
    };

    FPU_FLOAT_CLASS t1 = Float32GetClass( arg1 );
    FPU_FLOAT_CLASS t2 = Float32GetClass( arg2 );

    FLOAT32 f32;
    f32.value.u = RESULT[ t1 ][ t2 ];

    bool computeDenorms = ( denormRetain && ( Float32IsDenorm( arg1 ) || Float32IsDenorm( arg2 ) ) );

    if( ( f32.value.u == FPU_FLOAT32_COMPUTE ) || ( computeDenorms ) )
    {
        return arg1 / arg2;
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    Signed32SafeDivideQuotient

Description:
    Computes src0 divided by src1
    Table for handling signed divide quotient and remainder:
        IDIV            SRC0    
            SRC1        +INT            -INT             0
            +INT        +INT            -INT             0
            -INT        -INT            +INT             0
              0     Q:0x7FFFFFFF    Q: 0x80000000   Q:0x7FFFFFFF
                    R:0x7FFFFFFF    R: 0x80000000   R:0x7FFFFFFF
\*****************************************************************************/
inline signed long Signed32SafeDivideQuotient( 
    const signed long src0,
    const signed long src1 )
{
    if( !src1 )
    {
        if( src0 < 0 )
        {
            return LONG_MIN;
        }
        return LONG_MAX;
    }

    return src0 / src1;
}

/*****************************************************************************\
Inline Function:
    Signed32SafeDivideRemainder

Description:
    Computes remainder of src0 divided by src1
\*****************************************************************************/
inline signed long Signed32SafeDivideRemainder( 
    const signed long src0,
    const signed long src1 )
{
    if( !src1 )
    {
        if( src0 < 0 )
        {
            return LONG_MIN;
        }
        return LONG_MAX;
    }

    return src0 % src1;
}

/*****************************************************************************\
Inline Function:
    Unsigned32SafeDivideQuotient

Description:
    Computes src0 divided by src1
       Table for handling unsigned divide quotient and remainder 
          UDIV          SRC0    
              SRC1      <>0             0
              <>0       UINT            0
                0   Q:0xFFFFFFFF    Q:0xFFFFFFFF
                    R:0xFFFFFFFF    R:0xFFFFFFFF
\*****************************************************************************/
inline DWORD Unsigned32SafeDivideQuotient( 
    const DWORD src0,
    const DWORD src1 )
{
    if( !src1 )
    {
        return UINT_MAX;
    }

    return src0 / src1;
}

/*****************************************************************************\
Inline Function:
    Unsigned32SafeDivideRemainder

Description:
    Computes remainder of src0 divided by src1
\*****************************************************************************/
inline DWORD Unsigned32SafeDivideRemainder( 
    const DWORD src0,
    const DWORD src1 )
{
    if( !src1 )
    {
        return UINT_MAX;
    }

    return src0 % src1;
}

/*****************************************************************************\
Inline Function:
    F32ToF16_d

Description:
    Float32 to float16 conversion based on "Fast Half Float Conversions" 
    by Jeroen van der Zijp

Input: 
    32-bit DWORD represantation of float value
Output:
    16-bit DWORD represantation of float value

\*****************************************************************************/
inline WORD F32ToF16_d( DWORD arg )
{
    static const WORD btbl[512] = {
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080,0x0100,
        0x0200,0x0400,0x0800,0x0c00,0x1000,0x1400,0x1800,0x1c00,0x2000,0x2400,0x2800,0x2c00,0x3000,0x3400,0x3800,0x3c00,
        0x4000,0x4400,0x4800,0x4c00,0x5000,0x5400,0x5800,0x5c00,0x6000,0x6400,0x6800,0x6c00,0x7000,0x7400,0x7800,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,0x7c00,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,
        0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8000,0x8001,0x8002,0x8004,0x8008,0x8010,0x8020,0x8040,0x8080,0x8100,
        0x8200,0x8400,0x8800,0x8c00,0x9000,0x9400,0x9800,0x9c00,0xa000,0xa400,0xa800,0xac00,0xb000,0xb400,0xb800,0xbc00,
        0xc000,0xc400,0xc800,0xcc00,0xd000,0xd400,0xd800,0xdc00,0xe000,0xe400,0xe800,0xec00,0xf000,0xf400,0xf800,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,
        0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00,0xfc00
    };
    static const unsigned char stbl[512] = {
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10,0x0f,
        0x0e,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,
        0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x0d,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10,0x0f,
        0x0e,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,
        0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x0d,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x0d
    };
    DWORD sexp = (arg>>23)&0x1ff;
    return (WORD)(btbl[ sexp ]+( (arg&0x007fffff)>>stbl[ sexp ] ));
}

/*****************************************************************************\

Inline Function:
    F32ToF16_f

Description:
    Float32 to float16 conversion based on "Fast Half Float Conversions" 
    by Jeroen van der Zijp

Input: 
    32-bit float value
Output:
    16-bit WORD represantation of float value

\*****************************************************************************/
inline WORD F32ToF16_f( float arg )
{
    return F32ToF16_d( *(DWORD *)&arg );
}

/*****************************************************************************\

Inline Function:
    F16ToF32

Description:
    Float16 to float32 conversion

Input: 
    16-bit WORD representation of float16 value
Output:
    32-bit DWORD represantation of float32 value

\*****************************************************************************/
static inline DWORD F16ToF32( WORD v )
{
    unsigned long index;
    return 
        // is exponent!=0 ?
        v & 0x7C00
            // is exponent==max ?
            ? ( v & 0x7C00 ) == 0x7C00
                // is mantissa!=0 ?
                ? v & 0x03FF
                    // convert NaN
                    ? ( ( v << 13 ) + 0x70000000 ) | 0x7f800000
                    // convert infinities
                    : ( v << 16 ) | 0x7f800000
                // convert normalized values
                : ( ( ( v << 13 ) + 0x70000000 ) & ~0x70000000 ) + 0x38000000
            // is mantissa non-zero ?
            : v & 0x03FF
                // convert denormalized values
                ? index=bsr( v & 0x03FF ), ( ( ( ( v << 16 ) & 0x80000000 ) | ( ( v << 13 ) & 0xF800000 ) ) + 0x33800000 + ( index << 23 ) ) | ( ( ( v & 0x03FF ) << ( 23-index ) ) & ~0x800000 )
                // convert zeros
                : v << 16;
}

/*****************************************************************************\
Inline Function:
    Float32SafeMax

Description:
    MinMax of Floating Point Numbers.

Input:
    arg1
    arg2
    isGen7

Output:
    max( arg1, arg2 )

\*****************************************************************************/
inline float Float32SafeMax( const float arg1, const float arg2, bool isGen7 )
{
    // Values of following arrays corresponds to results of sel.l instructions.

    static const bool RESULT_preGen7[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf   -X      -denorm   -0      +0   +denorm   +X      +Inf    NaN
        { true  , false , false , false , false , false , false , false , true      },  // -Inf
        { true  , false , false , false , false , false , false , false , true      },  // -X
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // -denorm
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // -0
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +0
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +denorm
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +X
        { true  , true  , true  , true  , true  , true  , true  , true  , true      },  // +Inf
        { false , false , false , false , false , false , false , false , false     },  // NaN
    };

    static const bool RESULT_Gen7[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf   -X      -denorm   -0      +0   +denorm   +X      +Inf    NaN
        { true  , false , false , false , false , false , false , false , true      },  // -Inf
        { true  , false , false , false , false , false , false , false , true      },  // -X
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // -denorm
        { true  , true  , true  , true  , false , true  , false , false , true      },  // -0
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +0
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +denorm
        { true  , true  , true  , true  , true  , true  , false , false , true      },  // +X
        { true  , true  , true  , true  , true  , true  , true  , true  , true      },  // +Inf
        { false , false , false , false , false , false , false , false , false     },  // NaN
    };

    const FPU_FLOAT_CLASS t1 = Float32GetClass( arg1 );
    const FPU_FLOAT_CLASS t2 = Float32GetClass( arg2 );

    if( ( t1 == FPU_FLOAT_CLASS_NEG_FINITE || t1 == FPU_FLOAT_CLASS_POS_FINITE ) &&
        ( t2 == FPU_FLOAT_CLASS_NEG_FINITE || t2 == FPU_FLOAT_CLASS_POS_FINITE ) )
    {
        return ( arg1 >= arg2 ) ? arg1 : arg2;
    }

    FLOAT32 f32;

    if( isGen7 )
    {
        f32.value.f = ( RESULT_Gen7[t1][t2] ) ? arg1 : arg2;
    }
    else 
    {
        f32.value.f = ( RESULT_preGen7[t1][t2] ) ? arg1 : arg2;
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    Float32SafeMin

Description:
    MinMax of Floating Point Numbers.

Input:
    arg1
    arg2
    isGen7

Output:
    max( arg1, arg2 )

\*****************************************************************************/
inline float Float32SafeMin( const float arg1, const float arg2, bool isGen7 )
{
    // Values of following arrays corresponds to results of sel.ge instruction.

    static const bool RESULT_preGen7[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf   -X      -denorm   -0      +0   +denorm   +X      +Inf    NaN
        { false , true  , true  , true  , true  , true  , true  , true  , true      },  // -Inf
        { false , false , true  , true  , true  , true  , true  , true  , true      },  // -X
        { false , false , false , false , false , false , true  , true  , true      },  // -denorm
        { false , false , false , false , false , false , true  , true  , true      },  // -0
        { false , false , false , false , false , false , true  , true  , true      },  // +0
        { false , false , false , false , false , false , true  , true  , true      },  // +denorm
        { false , false , false , false , false , false , false , true  , true      },  // +X
        { false , false , false , false , false , false , false , false , true      },  // +Inf
        { false , false , false , false , false , false , false , false , false     },  // NaN
    };

    static const bool RESULT_Gen7[NUM_FPU_FLOAT_CLASSES][NUM_FPU_FLOAT_CLASSES] = {
    //    -Inf   -X      -denorm   -0      +0   +denorm   +X      +Inf    NaN
        { false , true  , true  , true  , true  , true  , true  , true  , true      },  // -Inf
        { false , false , true  , true  , true  , true  , true  , true  , true      },  // -X
        { false , false , false , false , false , false , true  , true  , true      },  // -denorm
        { false , false , false , false , true  , false , true  , true  , true      },  // -0
        { false , false , false , false , false , false , true  , true  , true      },  // +0
        { false , false , false , false , false , false , true  , true  , true      },  // +denorm
        { false , false , false , false , false , false , false , true  , true      },  // +X
        { false , false , false , false , false , false , false , false , true      },  // +Inf
        { false , false , false , false , false , false , false , false , false     },  // NaN
    };

    const FPU_FLOAT_CLASS t1 = Float32GetClass( arg1 );
    const FPU_FLOAT_CLASS t2 = Float32GetClass( arg2 );

    if( ( t1 == FPU_FLOAT_CLASS_NEG_FINITE || t1 == FPU_FLOAT_CLASS_POS_FINITE ) &&
        ( t2 == FPU_FLOAT_CLASS_NEG_FINITE || t2 == FPU_FLOAT_CLASS_POS_FINITE ) )
    {
        return ( arg1 < arg2 ) ? arg1 : arg2;
    }

    FLOAT32 f32;

    if( isGen7 )
    {
        f32.value.f = ( RESULT_Gen7[t1][t2] ) ? arg1 : arg2;
    }
    else 
    {
        f32.value.f = ( RESULT_preGen7[t1][t2] ) ? arg1 : arg2;
    }

    return f32.value.f;
}

/*****************************************************************************\
Inline Function:
    FloatSaturate

Description:

    For a floating-point destination type, the saturation target range is [0.0,
    1.0]. For a floating-point NaN, there is no "closest value"; any NaN
    saturates to 0.0. (...) Any floating-point number greater than 1.0,
    including +INF, saturates to 1.0. Any negative floating-point number,
    including -INF, saturates to 0.0. Any floating-point number in the range 0.0
    to 1.0 is not changed by saturation.

    -0.0 is changed to +0.0.

Input:
    const float f

Output:
    float

\*****************************************************************************/
inline float FloatSaturate( const float f )
{
    switch( Float32GetClass( f ) )
    {
    case FPU_FLOAT_CLASS_NEG_INF:
    case FPU_FLOAT_CLASS_NEG_FINITE:
    case FPU_FLOAT_CLASS_NEG_DENORM:
    case FPU_FLOAT_CLASS_NEG_ZERO:
    case FPU_FLOAT_CLASS_POS_ZERO:
    case FPU_FLOAT_CLASS_NAN:
        return 0.f;
    case FPU_FLOAT_CLASS_POS_DENORM:
        return f;
    case FPU_FLOAT_CLASS_POS_FINITE:
        return ( f <= 1.f ) ? f : 1.f;
    case FPU_FLOAT_CLASS_POS_INF:
        return 1.f;
    default:
        ASSERT( 0 );
        return 0.f;
    }
}

} // namespace iSTD
