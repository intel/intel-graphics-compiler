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
// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

static float precise_cospif( float a )
{
    float result;

    float sAbsX = __builtin_spirv_OpenCL_fabs_f32( a );
    if( sAbsX > as_float( 0x4A800000 ) )            // 2^22
    {
        float   sShifter =
            ( sAbsX < as_float( 0x4F000000 ) ) ?    // 2^31
            as_float( 0x4FC00000 ) :
            0.0f;

        a = a - (sShifter + a - sShifter);
    }

    float   sN = a + 0.5f + as_float( 0x4B400000 );

    uint    usN = as_uint( sN ) << 31;
    sN = sN - as_float( 0x4B400000 );
    sN = sN - 0.5f;

    float   sR = M_PI_F * (a - sN);

    float   sR2 = sR * sR;
    sR = as_float( as_uint( sR ) ^ usN );

    float   sP = as_float( 0x362EDEF8 );
    sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR2, as_float( 0xB94FB7FF ) );
    sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR2, as_float( 0x3C08876A ) );
    sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR2, as_float( 0xBE2AAAA6 ) );

    sP = sP * sR2;
    result = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, sR );

    float n = __builtin_spirv_OpenCL_nan_i32(0U);
    result = __intel_relaxed_isinf( sAbsX ) ? n : result;

    return result;
}