/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef __EXP_HYPERBOLIC_CL__
#define __EXP_HYPERBOLIC_CL__

/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

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
