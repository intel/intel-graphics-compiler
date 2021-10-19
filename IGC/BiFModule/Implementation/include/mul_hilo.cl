/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __MUL_HILO_CL__
#define __MUL_HILO_CL__


/*****************************************************************************\

Function:
    __intc_umul_hilo

Description:
    This function multiplies 2 unsigned longs (64 bits) operands and returns
    the 128 bit result in two parts (high - returned by function / low -
    return through pointer to lo parameter).

Input:
    ulong    sourceA    - operand
    ulong    sourceB    - operand
    ulong*   destLow    - return the low bits of the multiplication
Output:
    return the high bits of the multiplication.

\*****************************************************************************/
ulong ___intc_umul_hilo(ulong sourceA,
                        ulong sourceB,
                        __private ulong *destLow)
{
    ulong lowA, lowB;
    ulong highA, highB;

    // Split up the values
    lowA = sourceA & 0xffffffff;
    highA = sourceA >> 32;
    lowB = sourceB & 0xffffffff;
    highB = sourceB >> 32;

    // Note that, with this split, our multiplication becomes:
    //     ( a * b )
    // = ( ( aHI << 32 + aLO ) * ( bHI << 32 + bLO ) ) >> 64
    // = ( ( aHI << 32 * bHI << 32 ) + ( aHI << 32 * bLO ) + ( aLO * bHI << 32 ) + ( aLO * bLO ) ) >> 64
    // = ( ( aHI * bHI << 64 ) + ( aHI * bLO << 32 ) + ( aLO * bHI << 32 ) + ( aLO * bLO ) ) >> 64
    // = ( aHI * bHI ) + ( aHI * bLO >> 32 ) + ( aLO * bHI >> 32 ) + ( aLO * bLO >> 64 )

    // Now, since each value is 32 bits, the max size of any multiplication is:
    // ( 2 ^ 32 - 1 ) * ( 2 ^ 32 - 1 ) = 2^64 - 4^32 + 1 = 2^64 - 2^33 + 1,
    // which fits within 64 bits. Which means we can do each component within a
    // 64-bit integer as necessary (each component above marked as AB1 - AB4)
    ulong aHibHi = highA * highB;
    ulong aHibLo = highA * lowB;
    ulong aLobHi = lowA * highB;
    ulong aLobLo = lowA * lowB;

    // Assemble terms.
    //  We note that in certain cases, sums of products cannot overflow:
    //
    //      The maximum product of two N-bit unsigned numbers is
    //
    //          (2**N-1)^2 = 2**2N - 2**(N+1) + 1
    //
    //      We note that we can add the maximum N-bit number to the 2N-bit product
    //      twice without overflow:
    //
    //          (2**N-1)^2 + 2*(2**N-1) = 2**2N - 2**(N+1) + 1 + 2**(N+1) - 2 = 2**2N - 1
    //
    //  If we breakdown the product of two numbers a,b into high and low halves of
    //  partial products as follows:
    //
    //                                              a.hi                a.lo
    // x                                            b.hi                b.lo
    //===============================================================================
    //  (b.hi*a.hi).hi      (b.hi*a.hi).lo
    //                      (b.lo*a.hi).hi      (b.lo*a.hi).lo
    //                      (b.hi*a.lo).hi      (b.hi*a.lo).lo
    // +                                        (b.lo*a.lo).hi      (b.lo*a.lo).lo
    //===============================================================================
    //
    // The (b.lo*a.lo).lo term cannot cause a carry, so we can ignore them
    // for now.  We also know from above, that we can add (b.lo*a.lo).hi
    // and (b.hi*a.lo).lo to the 2N bit term [(b.lo*a.hi).hi + (b.lo*a.hi).lo]
    // without overflow.  That takes care of all of the terms
    // on the right half that might carry.  Do that now.
    //
    ulong aLobLoHi = aLobLo >> 32;
    ulong aLobHiLo = aLobHi & 0xFFFFFFFFUL;
    aHibLo += aLobLoHi + aLobHiLo;

    // That leaves us with these terms:
    //
    //                                              a.hi                a.lo
    // x                                            b.hi                b.lo
    //===============================================================================
    //  (b.hi*a.hi).hi      (b.hi*a.hi).lo
    //                      (b.hi*a.lo).hi
    //                    [ (b.lo*a.hi).hi + (b.lo*a.hi).lo + other ]
    // +                                                                (b.lo*a.lo).lo
    //===============================================================================

    // All of the overflow potential from the right half has now been accumulated
    // into the [ (b.lo*a.hi).hi + (b.lo*a.hi).lo ] 2N bit term.
    // We can safely separate into high and low parts. Per our rule above, we know we
    // can accumulate the high part of that and (b.hi*a.lo).hi
    // into the 2N bit term (b.lo*a.hi) without carry.  The low part can be pieced
    // together with (b.lo*a.lo).lo, to give the final low result

    *destLow = (aHibLo << 32) | ( aLobLo & 0xFFFFFFFFUL );
    return ( aHibHi + (aHibLo >> 32 ) + (aLobHi >> 32) );       // Cant overflow
}


/*****************************************************************************\

Function:
    __intc_mul_hilo

Description:
    This function multiplies 2 longs (64 bits) operands and returns
    the 128 bit result in two parts (high - returned by function / low -
    return through pointer to lo parameter).

Input:
    long    sourceA    - operand
    long    sourceB    - operand
    ulong*  destLow   - return the low bits of the multiplication
Output:
    return the high bits of the multiplication.

\*****************************************************************************/
long ___intc_mul_hilo(long sourceA,
                      long sourceB,
                      __private ulong *destLow)
{
    // Find sign of result
    long aSign = sourceA >> 63;
    long bSign = sourceB >> 63;
    long resultSign = aSign ^ bSign;

    // take absolute values of the argument
    sourceA = (sourceA ^ aSign) - aSign;
    sourceB = (sourceB ^ bSign) - bSign;

    ulong hi;
    hi = ___intc_umul_hilo( (ulong) sourceA,
                            (ulong) sourceB,
                            destLow );

    // Fix the sign
    if( resultSign )
    {
        *destLow ^= resultSign;
        hi  ^= resultSign;
        *destLow -= resultSign;

        //carry if necessary
        if( 0 == *destLow )
            hi -= resultSign;
    }

    return (long) hi;
}


#endif // __MUL_HILO_CL__
