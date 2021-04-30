/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __EXP_HYPERBOLIC_CL__
#define __EXP_HYPERBOLIC_CL__

// This version of exp is only used for hyperbolic functions, sinh, cosh and tanh
// TODO: Need to investigate why replacing this with a regular old exp() is
//       insufficient to pass conformance.

float exp_for_hyper(float x)
{
    float C1 =   0.693359375f;
    float C2 =  -2.12194440e-4f;
    float z;
    int n;
    float xx = x;
    x = fabs(x);

    if( x > as_float(0x42B17217))
    {
        return (xx > 0.0f)? as_float(INFINITY) : 0.0f;
    }

    if( xx < as_float(0xC2CC0D65) )
    {
    return(0.0f);
    }

    /* Express e**x = e**g 2**n
    *   = e**g e**( n loge(2) )
    *   = e**( g + n loge(2) )
    */
    z = floor( M_LOG2E_F * x + 0.5f ); /* floor() truncates toward -infinity. */
    x -= z * C1;
    x -= z * C2;
    n = z;
    z = x * x;
    /* Theoretical peak relative error in [-0.5, +0.5] is 4.2e-9. */
    z =
        ((((( 1.9875691500E-4f  * x
        + 1.3981999507E-3f) * x
        + 8.3334519073E-3f) * x
        + 4.1665795894E-2f) * x
        + 1.6666665459E-1f) * x
        + 5.0000001201E-1f) * z
        + x
        + 1.0f;
    /* multiply by power of 2 */
    x = ldexp( z, n );
    return (xx > 0.0f)? ( x ) : 1/x;
}

#endif  //__EXP_HYPERBOLIC_CL__
