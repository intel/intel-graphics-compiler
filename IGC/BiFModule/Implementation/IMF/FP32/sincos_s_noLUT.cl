/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

static float __ocl_svml_sincosf_noLUT(float a, __private float* c)
{
    float result = 0.0f;
    float resultC = 0.0f;

    float   sSinN = __spirv_ocl_fma( a, M_1_PI_F, as_float( 0x4B400000 ) );

    uint    usSinN = as_uint( sSinN ) << 31;
    sSinN = sSinN - as_float( 0x4B400000 );

    float   sSinR = __spirv_ocl_fma( sSinN, as_float( 0xC0490000 ), a );
    sSinR = __spirv_ocl_fma( sSinN, as_float( 0xBA7DA000 ), sSinR );

    uint    usSinR = as_uint( sSinR ) & FLOAT_SIGN_MASK;

    sSinR = __spirv_ocl_fma( sSinN, as_float( 0xB4222000 ), sSinR );
    sSinR = __spirv_ocl_fma( sSinN, as_float( 0xACB4611A ), sSinR );

    float   sSinR2 = sSinR * sSinR;
    sSinR = as_float( as_uint( sSinR ) ^ usSinN );

    float   sSinP = as_float( 0x362EDEF8 );
    sSinP = __spirv_ocl_fma( sSinP, sSinR2, as_float( 0xB94FB7FF ) );
    sSinP = __spirv_ocl_fma( sSinP, sSinR2, as_float( 0x3C08876A ) );
    sSinP = __spirv_ocl_fma( sSinP, sSinR2, as_float( 0xBE2AAAA6 ) );

    sSinP = sSinP * sSinR2;
    result = __spirv_ocl_fma( sSinP, sSinR, sSinR );

    float   sCosN = sSinN;

    uint    usCosN = ( usSinR ^ usSinN ) ^ FLOAT_SIGN_MASK;
    sCosN = sSinN + as_float( as_uint( 0.5f ) ^ usSinR );

    float   sCosR = __spirv_ocl_fma( sCosN, as_float( 0xC0490000 ), a );
    sCosR = __spirv_ocl_fma( sCosN, as_float( 0xBA7DA000 ), sCosR );
    sCosR = __spirv_ocl_fma( sCosN, as_float( 0xB4222000 ), sCosR );
    sCosR = __spirv_ocl_fma( sCosN, as_float( 0xACB4611A ), sCosR );

    float   sCosR2 = sCosR * sCosR;
    sCosR = as_float( as_uint( sCosR ) ^ usCosN );

    float sCosP = as_float( 0x362EDEF8 );
    sCosP = __spirv_ocl_fma( sCosP, sCosR2, as_float( 0xB94FB7FF ) );
    sCosP = __spirv_ocl_fma( sCosP, sCosR2, as_float( 0x3C08876A ) );
    sCosP = __spirv_ocl_fma( sCosP, sCosR2, as_float( 0xBE2AAAA6 ) );

    sCosP = sCosP * sCosR2;
    resultC = __spirv_ocl_fma( sCosP, sCosR, sCosR );

    c[0] = resultC;
    return result;
}
