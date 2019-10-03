/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#if defined(cl_khr_fp64)

INLINE
double __builtin_spirv_OpenCL_trunc_f64(double x )
{
    //Algorithm performs rounding towards zero by truncating bits in the fractional part
    // of the number.This is done by finding out the position of the fractional bits of
    //the mantissa and masking them out with zeros.

    signed high32Bit = (int)(as_long( x ) >> 32);
    //int physicalexp = (int)__builtin_IB_ubfe( 11,20,high32bit );
    uint physicalExp = ( high32Bit & 0x7fffffff ) >> 20;             // Extract the physical exponent.
    int  logicalExp = (int)physicalExp - DOUBLE_BIAS;                // Extract the logical exponent.

    // Compute shift amount.
    signed shiftAmountHigh32bit = -logicalExp + 52;
    signed shiftAmountLow32bit  = -logicalExp + 20;

    shiftAmountHigh32bit = ( shiftAmountHigh32bit > 32 ) ? 32 : shiftAmountHigh32bit;
    shiftAmountLow32bit  = ( shiftAmountLow32bit  > 20 ) ? 20 : shiftAmountLow32bit;
    shiftAmountHigh32bit = ( shiftAmountHigh32bit > 0 )  ? shiftAmountHigh32bit : 0;
    shiftAmountLow32bit  = ( shiftAmountLow32bit  > 0 )  ? shiftAmountLow32bit  : 0;

    // Create final mask.
    // All-ones initial mask.
    signed shiftedValHigh32bit = 0xFFFFFFFF << shiftAmountHigh32bit;
    signed shiftedValLow32bit  = 0xFFFFFFFF << shiftAmountLow32bit;
    signed temp1 = 0;
    signed temp2 = 0;
    signed maskValLow32bit = 0;
    signed maskValHigh32bit = 0x80000000;   // Initialize sign only mask. If exponent < 0 , override the mask with sign-only mask.
    temp1 = ((shiftAmountHigh32bit != 32) ? shiftedValHigh32bit : 0x00000000); // If exponent = 32, mask with 0.
    if(shiftAmountLow32bit != 32)
    {
        temp2 = shiftedValLow32bit;
        if(!(logicalExp < 0))
        {
            maskValLow32bit  = temp1;
            maskValHigh32bit = temp2;
        }
    }

    uint andDst1 = (uint)(as_ulong( x )) & maskValLow32bit;
    uint andDst2 = high32Bit & maskValHigh32bit;
    //combining low and high 32-bits forming 64-bit val.
    ulong temp3 = (ulong)andDst2 << 32;
    ulong temp4 = ((ulong)andDst1) ;
    ulong roundedToZeroVal = temp3 | as_ulong( temp4 );    // Rounding towards zero is completed at this point
    return as_double( roundedToZeroVal );
}

#endif // defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, half, half, f16 )

#endif // defined(cl_khr_fp16)
