/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

static float __ocl_svml_sinf_noLUT( float a )
{
    float result = 0.0f;
    float   sN = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( a, M_1_PI_F, as_float( 0x4B400000 ) );

    uint    usN = as_uint( sN ) << 31;
    sN = sN - as_float( 0x4B400000 );

    float   sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sN, as_float( 0xC0490000 ), a );
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sN, as_float( 0xBA7DA000 ), sR );
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sN, as_float( 0xB4222000 ), sR );
    sR = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sN, as_float( 0xACB4611A ), sR );

    float   sR2 = sR * sR;
    sR = as_float( as_uint( sR ) ^ usN );

    float   sP = as_float( 0x362EDEF8 );
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sP, sR2, as_float( 0xB94FB7FF ) );
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sP, sR2, as_float( 0x3C08876A ) );
    sP = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sP, sR2, as_float( 0xBE2AAAA6 ) );

    sP = sP * sR2;
    result = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )( sP, sR, sR );

    return result;
}
