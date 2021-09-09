/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../../include/BiF_Definitions.cl"
#include "../../../Headers/spirv.h"
#include "hadd.cl"

 //FOIL-based long mul_hi
 //
 // Summary: Treat mul_hi(long x, long y) as:
 // (a+b) * (c+d) where a and c are the high-order parts of x and y respectively
 // and b and d are the low-order parts of x and y.
 // Thinking back to algebra, we use FOIL to do the work.

INLINE OVERLOADABLE long libclc_mul_hi(long x, long y) {
    long f, o, i;
    ulong l;

    //Move the high/low halves of x/y into the lower 32-bits of variables so
    //that we can multiply them without worrying about overflow.
    long x_hi = x >> 32;
    long x_lo = x & UINT_MAX;
    long y_hi = y >> 32;
    long y_lo = y & UINT_MAX;

    //Multiply all of the components according to FOIL method
    f = x_hi * y_hi;
    o = x_hi * y_lo;
    i = x_lo * y_hi;
    l = x_lo * y_lo;

    //Now add the components back together in the following steps:
    //F: doesn't need to be modified
    //O/I: Need to be added together.
    //L: Shift right by 32-bits, then add into the sum of O and I
    //Once O/I/L are summed up, then shift the sum by 32-bits and add to F.
    //
    //We use hadd to give us a bit of extra precision for the intermediate sums
    //but as a result, we shift by 31 bits instead of 32
    return (long)(f + (libclc_hadd(o, (i + (long)((ulong)l >> 32))) >> 31));
}
