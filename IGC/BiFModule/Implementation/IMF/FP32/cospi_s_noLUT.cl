/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

static float __ocl_svml_cospif_noLUT( float a )
{
    float result;

    float sAbsX = __spirv_ocl_fabs( a );
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
    sP = __spirv_ocl_fma( sP, sR2, as_float( 0xB94FB7FF ) );
    sP = __spirv_ocl_fma( sP, sR2, as_float( 0x3C08876A ) );
    sP = __spirv_ocl_fma( sP, sR2, as_float( 0xBE2AAAA6 ) );

    sP = sP * sR2;
    result = __spirv_ocl_fma( sP, sR, sR );

    float n = __spirv_ocl_nan(0);
    result = __intel_relaxed_isinf( sAbsX ) ? n : result;

    return result;
}
