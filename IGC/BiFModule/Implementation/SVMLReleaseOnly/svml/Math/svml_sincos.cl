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

static float precise_sincosf(float a, __private float* c)
{
    float result = 0.0f;
    float resultC = 0.0f;

    float   sSinN = __builtin_spirv_OpenCL_fma_f32_f32_f32( a, M_1_PI_F, as_float( 0x4B400000 ) );

    uint    usSinN = as_uint( sSinN ) << 31;
    sSinN = sSinN - as_float( 0x4B400000 );

    float   sSinR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinN, as_float( 0xC0490000 ), a );
    sSinR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinN, as_float( 0xBA7DA000 ), sSinR );

    uint    usSinR = as_uint( sSinR ) & FLOAT_SIGN_MASK;

    sSinR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinN, as_float( 0xB4222000 ), sSinR );
    sSinR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinN, as_float( 0xACB4611A ), sSinR );

    float   sSinR2 = sSinR * sSinR;
    sSinR = as_float( as_uint( sSinR ) ^ usSinN );

    float   sSinP = as_float( 0x362EDEF8 );
    sSinP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinP, sSinR2, as_float( 0xB94FB7FF ) );
    sSinP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinP, sSinR2, as_float( 0x3C08876A ) );
    sSinP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinP, sSinR2, as_float( 0xBE2AAAA6 ) );

    sSinP = sSinP * sSinR2;
    result = __builtin_spirv_OpenCL_fma_f32_f32_f32( sSinP, sSinR, sSinR );

    float   sCosN = sSinN;

    uint    usCosN = ( usSinR ^ usSinN ) ^ FLOAT_SIGN_MASK;
    sCosN = sSinN + as_float( as_uint( 0.5f ) ^ usSinR );

    float   sCosR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosN, as_float( 0xC0490000 ), a );
    sCosR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosN, as_float( 0xBA7DA000 ), sCosR );
    sCosR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosN, as_float( 0xB4222000 ), sCosR );
    sCosR = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosN, as_float( 0xACB4611A ), sCosR );

    float   sCosR2 = sCosR * sCosR;
    sCosR = as_float( as_uint( sCosR ) ^ usCosN );

    float sCosP = as_float( 0x362EDEF8 );
    sCosP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosP, sCosR2, as_float( 0xB94FB7FF ) );
    sCosP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosP, sCosR2, as_float( 0x3C08876A ) );
    sCosP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosP, sCosR2, as_float( 0xBE2AAAA6 ) );

    sCosP = sCosP * sCosR2;
    resultC = __builtin_spirv_OpenCL_fma_f32_f32_f32( sCosP, sCosR, sCosR );

    c[0] = resultC;
    return result;
}
