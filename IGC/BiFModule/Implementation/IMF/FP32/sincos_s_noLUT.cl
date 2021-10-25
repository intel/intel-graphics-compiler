/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

static float __ocl_svml_sincosf_noLUT(float a, __private float* c)
{
    float result = 0.0f;
    float resultC = 0.0f;

    float   sSinN = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( a, M_1_PI_F, as_float( 0x4B400000 ) );

    uint    usSinN = as_uint( sSinN ) << 31;
    sSinN = sSinN - as_float( 0x4B400000 );

    float   sSinR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinN, as_float( 0xC0490000 ), a );
    sSinR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinN, as_float( 0xBA7DA000 ), sSinR );

    uint    usSinR = as_uint( sSinR ) & FLOAT_SIGN_MASK;

    sSinR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinN, as_float( 0xB4222000 ), sSinR );
    sSinR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinN, as_float( 0xACB4611A ), sSinR );

    float   sSinR2 = sSinR * sSinR;
    sSinR = as_float( as_uint( sSinR ) ^ usSinN );

    float   sSinP = as_float( 0x362EDEF8 );
    sSinP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinP, sSinR2, as_float( 0xB94FB7FF ) );
    sSinP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinP, sSinR2, as_float( 0x3C08876A ) );
    sSinP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinP, sSinR2, as_float( 0xBE2AAAA6 ) );

    sSinP = sSinP * sSinR2;
    result = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sSinP, sSinR, sSinR );

    float   sCosN = sSinN;

    uint    usCosN = ( usSinR ^ usSinN ) ^ FLOAT_SIGN_MASK;
    sCosN = sSinN + as_float( as_uint( 0.5f ) ^ usSinR );

    float   sCosR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosN, as_float( 0xC0490000 ), a );
    sCosR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosN, as_float( 0xBA7DA000 ), sCosR );
    sCosR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosN, as_float( 0xB4222000 ), sCosR );
    sCosR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosN, as_float( 0xACB4611A ), sCosR );

    float   sCosR2 = sCosR * sCosR;
    sCosR = as_float( as_uint( sCosR ) ^ usCosN );

    float sCosP = as_float( 0x362EDEF8 );
    sCosP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosP, sCosR2, as_float( 0xB94FB7FF ) );
    sCosP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosP, sCosR2, as_float( 0x3C08876A ) );
    sCosP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosP, sCosR2, as_float( 0xBE2AAAA6 ) );

    sCosP = sCosP * sCosR2;
    resultC = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sCosP, sCosR, sCosR );

    c[0] = resultC;
    return result;
}
