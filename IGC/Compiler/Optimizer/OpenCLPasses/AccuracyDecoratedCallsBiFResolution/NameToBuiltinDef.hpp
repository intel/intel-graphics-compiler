/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//------------------------------------------------------------------------------
// This file contains the definitions for mapping from mangled names to builtin
// names of functions decorated with fpbuiltin-max-error
//
// DEF_NAME_TO_BUILTIN is defined in AccuracyDecoratedCallsBiFResolution.cpp
//
//------------------------------------------------------------------------------
// if entry is commented out, it means it's not implemented in BiFModule yet.
//------------------------------------------------------------------------------
//                           name         accuracy        builtin
//------------------------------------------------------------------------------

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sinf", ENHANCED_PRECISION, "__ocl_svml_sinf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sinf", LOW_ACCURACY, "__ocl_svml_sinf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sinf", HIGH_ACCURACY, "__ocl_svml_sinf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_sincosf", ENHANCED_PRECISION, "__ocl_svml_sincosf_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sincosf", LOW_ACCURACY, "__ocl_svml_sincosf");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sincosf", HIGH_ACCURACY, "__ocl_svml_sincosf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_sinpif", ENHANCED_PRECISION, "__ocl_svml_sinpif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sinpif", LOW_ACCURACY, "__ocl_svml_sinpif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sinpif", HIGH_ACCURACY, "__ocl_svml_sinpif_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_sincospif", ENHANCED_PRECISION, "__ocl_svml_sincospif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sincospif", LOW_ACCURACY, "__ocl_svml_sincospif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_sincospif", HIGH_ACCURACY, "__ocl_svml_sincospif_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosf", ENHANCED_PRECISION, "__ocl_svml_cosf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosf", LOW_ACCURACY, "__ocl_svml_cosf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosf", HIGH_ACCURACY, "__ocl_svml_cosf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_cospif", ENHANCED_PRECISION, "__ocl_svml_cospif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_cospif", LOW_ACCURACY, "__ocl_svml_cospif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_cospif", HIGH_ACCURACY, "__ocl_svml_cospif_ha");

// DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tanf", ENHANCED_PRECISION, "__ocl_svml_tanf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tanf", LOW_ACCURACY, "__ocl_svml_tanf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tanf", HIGH_ACCURACY, "__ocl_svml_tanf_ha");

// DEF_NAME_TO_BUILTIN("_spirv_ocl_tanpif", ENHANCED_PRECISION, "__ocl_svml_tanpif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_tanpif", LOW_ACCURACY, "__ocl_svml_tanpif");
// DEF_NAME_TO_BUILTIN("_spirv_ocl_tanpif", HIGH_ACCURACY, "__ocl_svml_tanpif_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhf", ENHANCED_PRECISION, "__ocl_svml_sinhf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhf", LOW_ACCURACY, "__ocl_svml_sinhf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhf", HIGH_ACCURACY, "__ocl_svml_sinhf_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshf", ENHANCED_PRECISION, "__ocl_svml_coshf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshf", LOW_ACCURACY, "__ocl_svml_coshf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshf", HIGH_ACCURACY, "__ocl_svml_coshf_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhf", ENHANCED_PRECISION, "__ocl_svml_tanhf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhf", LOW_ACCURACY, "__ocl_svml_tanhf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhf", HIGH_ACCURACY, "__ocl_svml_tanhf_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asinf", ENHANCED_PRECISION, "__ocl_svml_asinf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asinf", LOW_ACCURACY, "__ocl_svml_asinf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asinf", HIGH_ACCURACY, "__ocl_svml_asinf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_asinpif", ENHANCED_PRECISION, "__ocl_svml_asinpif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_asinpif", LOW_ACCURACY, "__ocl_svml_asinpif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_asinpif", HIGH_ACCURACY, "__ocl_svml_asinpif_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosf", ENHANCED_PRECISION, "__ocl_svml_acosf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosf", LOW_ACCURACY, "__ocl_svml_acosf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosf", HIGH_ACCURACY, "__ocl_svml_acosf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_acospif", ENHANCED_PRECISION, "__ocl_svml_acospif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_acospif", LOW_ACCURACY, "__ocl_svml_acospif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_acospif", HIGH_ACCURACY, "__ocl_svml_acospif_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atanf", ENHANCED_PRECISION, "__ocl_svml_atanf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atanf", LOW_ACCURACY, "__ocl_svml_atanf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atanf", HIGH_ACCURACY, "__ocl_svml_atanf_ha");

DEF_NAME_TO_BUILTIN("_spirv_ocl_atanpif", ENHANCED_PRECISION, "__ocl_svml_atanpif_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_atanpif", LOW_ACCURACY, "__ocl_svml_atanpif");
DEF_NAME_TO_BUILTIN("_spirv_ocl_atanpif", HIGH_ACCURACY, "__ocl_svml_atanpif_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atan2f", ENHANCED_PRECISION, "__ocl_svml_atan2f_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atan2f", LOW_ACCURACY, "__ocl_svml_atan2f");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atan2f", HIGH_ACCURACY, "__ocl_svml_atan2f_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhf", ENHANCED_PRECISION, "__ocl_svml_asinhf_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhf", LOW_ACCURACY, "__ocl_svml_asinhf");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhf", HIGH_ACCURACY, "__ocl_svml_asinhf_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshf", ENHANCED_PRECISION, "__ocl_svml_acoshf_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshf", LOW_ACCURACY, "__ocl_svml_acoshf");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshf", HIGH_ACCURACY, "__ocl_svml_acoshf_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhf", ENHANCED_PRECISION, "__ocl_svml_atanhf_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhf", LOW_ACCURACY, "__ocl_svml_atanhf");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhf", HIGH_ACCURACY, "__ocl_svml_atanhf_ha");

// DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expf", ENHANCED_PRECISION, "__ocl_svml_expf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expf", LOW_ACCURACY, "__ocl_svml_expf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expf", HIGH_ACCURACY, "__ocl_svml_expf_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2f", ENHANCED_PRECISION, "__ocl_svml_exp2f_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2f", LOW_ACCURACY, "__ocl_svml_exp2f");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2f", HIGH_ACCURACY, "__ocl_svml_exp2f_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10f", ENHANCED_PRECISION, "__ocl_svml_exp10f_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10f", LOW_ACCURACY, "__ocl_svml_exp10f");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10f", HIGH_ACCURACY, "__ocl_svml_exp10f_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1f", ENHANCED_PRECISION, "__ocl_svml_expm1f_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1f", LOW_ACCURACY, "__ocl_svml_expm1f");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1f", HIGH_ACCURACY, "__ocl_svml_expm1f_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logf", ENHANCED_PRECISION, "__ocl_svml_logf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logf", LOW_ACCURACY, "__ocl_svml_logf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logf", HIGH_ACCURACY, "__ocl_svml_logf_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2f", ENHANCED_PRECISION, "__ocl_svml_log2f_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2f", LOW_ACCURACY, "__ocl_svml_log2f");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2f", HIGH_ACCURACY, "__ocl_svml_log2f_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10f", ENHANCED_PRECISION, "__ocl_svml_log10f_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10f", LOW_ACCURACY, "__ocl_svml_log10f");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10f", HIGH_ACCURACY, "__ocl_svml_log10f_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pf", ENHANCED_PRECISION, "__ocl_svml_log1pf_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pf", LOW_ACCURACY, "__ocl_svml_log1pf");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pf", HIGH_ACCURACY, "__ocl_svml_log1pf_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtf", ENHANCED_PRECISION, "__ocl_svml_sqrtf_ep");
// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtf", LOW_ACCURACY, "__ocl_svml_sqrtf");
// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtf", HIGH_ACCURACY, "__ocl_svml_sqrtf_ha");
//
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtf", ENHANCED_PRECISION, "__ocl_svml_rsqrtf_ep");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtf", LOW_ACCURACY, "__ocl_svml_rsqrtf");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtf", HIGH_ACCURACY, "__ocl_svml_rsqrtf_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erff", ENHANCED_PRECISION, "__ocl_svml_erff_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erff", LOW_ACCURACY, "__ocl_svml_erff");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erff", HIGH_ACCURACY, "__ocl_svml_erff_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcf", ENHANCED_PRECISION, "__ocl_svml_erfcf_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcf", LOW_ACCURACY, "__ocl_svml_erfcf");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcf", HIGH_ACCURACY, "__ocl_svml_erfcf_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2ff", ENHANCED_PRECISION, "__ocl_svml_atan2f_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2ff", LOW_ACCURACY, "__ocl_svml_atan2f");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2ff", HIGH_ACCURACY, "__ocl_svml_atan2f_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpff", ENHANCED_PRECISION, "__ocl_svml_ldexpf_ep");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpff", LOW_ACCURACY, "__ocl_svml_ldexpf");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpff", HIGH_ACCURACY, "__ocl_svml_ldexpf_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powff", ENHANCED_PRECISION, "__ocl_svml_powf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powff", LOW_ACCURACY, "__ocl_svml_powf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powff", HIGH_ACCURACY, "__ocl_svml_powf_ha");

// DEF_NAME_TO_BUILTIN("_spirv_ocl_pownff", ENHANCED_PRECISION, "__ocl_svml_pownf_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_pownff", LOW_ACCURACY, "__ocl_svml_pownf");
// DEF_NAME_TO_BUILTIN("_spirv_ocl_pownff", HIGH_ACCURACY, "__ocl_svml_pownf_ha");

// DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powrff", ENHANCED_PRECISION, "__ocl_svml_powrf_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_powrff", LOW_ACCURACY, "__ocl_svml_powrf");
// DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powrff", HIGH_ACCURACY, "__ocl_svml_powrf_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotff", ENHANCED_PRECISION, "__ocl_svml_hypotf_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotff", LOW_ACCURACY, "__ocl_svml_hypotf_la");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotff", HIGH_ACCURACY, "__ocl_svml_hypotf_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sind", ENHANCED_PRECISION, "__ocl_svml_sin_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sind", LOW_ACCURACY, "__ocl_svml_sin");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_sind", HIGH_ACCURACY, "__ocl_svml_sin_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosd", ENHANCED_PRECISION, "__ocl_svml_cos_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosd", LOW_ACCURACY, "__ocl_svml_cos");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_cosd", HIGH_ACCURACY, "__ocl_svml_cos_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tand", ENHANCED_PRECISION, "__ocl_svml_tan_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tand", LOW_ACCURACY, "__ocl_svml_tan");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_tand", HIGH_ACCURACY, "__ocl_svml_tan_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhd", ENHANCED_PRECISION, "__ocl_svml_sinh_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhd", LOW_ACCURACY, "__ocl_svml_sinh");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sinhd", HIGH_ACCURACY, "__ocl_svml_sinh_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshd", ENHANCED_PRECISION, "__ocl_svml_cosh_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshd", LOW_ACCURACY, "__ocl_svml_cosh");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_coshd", HIGH_ACCURACY, "__ocl_svml_cosh_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhd", ENHANCED_PRECISION, "__ocl_svml_tanh_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhd", LOW_ACCURACY, "__ocl_svml_tanh");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_tanhd", HIGH_ACCURACY, "__ocl_svml_tanh_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asind", ENHANCED_PRECISION, "__ocl_svml_asin_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asind", LOW_ACCURACY, "__ocl_svml_asin");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_asind", HIGH_ACCURACY, "__ocl_svml_asin_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosd", ENHANCED_PRECISION, "__ocl_svml_acos_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosd", LOW_ACCURACY, "__ocl_svml_acos");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_acosd", HIGH_ACCURACY, "__ocl_svml_acos_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atand", ENHANCED_PRECISION, "__ocl_svml_atan_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atand", LOW_ACCURACY, "__ocl_svml_atan");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_atand", HIGH_ACCURACY, "__ocl_svml_atan_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhd", ENHANCED_PRECISION, "__ocl_svml_asinh_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhd", LOW_ACCURACY, "__ocl_svml_asinh");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_asinhd", HIGH_ACCURACY, "__ocl_svml_asinh_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshd", ENHANCED_PRECISION, "__ocl_svml_acosh_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshd", LOW_ACCURACY, "__ocl_svml_acosh");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_acoshd", HIGH_ACCURACY, "__ocl_svml_acosh_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhd", ENHANCED_PRECISION, "__ocl_svml_atanh_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhd", LOW_ACCURACY, "__ocl_svml_atanh");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atanhd", HIGH_ACCURACY, "__ocl_svml_atanh_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expd", ENHANCED_PRECISION, "__ocl_svml_exp_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expd", LOW_ACCURACY, "__ocl_svml_exp");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_expd", HIGH_ACCURACY, "__ocl_svml_exp_ha");

DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2d", ENHANCED_PRECISION, "__ocl_svml_exp2_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2d", LOW_ACCURACY, "__ocl_svml_exp2");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_exp2d", HIGH_ACCURACY, "__ocl_svml_exp2_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10d", ENHANCED_PRECISION, "__ocl_svml_exp10_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10d", LOW_ACCURACY, "__ocl_svml_exp10");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_exp10d", HIGH_ACCURACY, "__ocl_svml_exp10_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1d", ENHANCED_PRECISION, "__ocl_svml_expm1_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1d", LOW_ACCURACY, "__ocl_svml_expm1");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_expm1d", HIGH_ACCURACY, "__ocl_svml_expm1_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logd", ENHANCED_PRECISION, "__ocl_svml_log_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logd", LOW_ACCURACY, "__ocl_svml_log");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_logd", HIGH_ACCURACY, "__ocl_svml_log_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2d", ENHANCED_PRECISION, "__ocl_svml_log2_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2d", LOW_ACCURACY, "__ocl_svml_log2_v2");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_log2d", HIGH_ACCURACY, "__ocl_svml_log2_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10d", ENHANCED_PRECISION, "__ocl_svml_log10_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10d", LOW_ACCURACY, "__ocl_svml_log10_v2");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log10d", HIGH_ACCURACY, "__ocl_svml_log10_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pd", ENHANCED_PRECISION, "__ocl_svml_log1p_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pd", LOW_ACCURACY, "__ocl_svml_log1p");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_log1pd", HIGH_ACCURACY, "__ocl_svml_log1p_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtd", ENHANCED_PRECISION, "__ocl_svml_sqrt_ep");
// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtd", LOW_ACCURACY, "__ocl_svml_sqrt");
// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_sqrtd", HIGH_ACCURACY, "__ocl_svml_sqrt_ha");
//
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtd", ENHANCED_PRECISION, "__ocl_svml_rsqrt_ep");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtd", LOW_ACCURACY, "__ocl_svml_rsqrt");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_rsqrtd", HIGH_ACCURACY, "__ocl_svml_rsqrt_ha");

DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erfd", ENHANCED_PRECISION, "__ocl_svml_erf_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erfd", LOW_ACCURACY, "__ocl_svml_erf");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_erfd", HIGH_ACCURACY, "__ocl_svml_erf_ha");

// DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcd", ENHANCED_PRECISION, "__ocl_svml_erfc_ep");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcd", LOW_ACCURACY, "__ocl_svml_erfc");
DEF_NAME_TO_BUILTIN("_Z16__spirv_ocl_erfcd", HIGH_ACCURACY, "__ocl_svml_erfc_ha");

DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2dd", ENHANCED_PRECISION, "__ocl_svml_atan2_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2dd", LOW_ACCURACY, "__ocl_svml_atan2");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_atan2dd", HIGH_ACCURACY, "__ocl_svml_atan2_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpdd", ENHANCED_PRECISION, "__ocl_svml_ldexp_ep");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpdd", LOW_ACCURACY, "__ocl_svml_ldexp");
// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_ldexpdd", HIGH_ACCURACY, "__ocl_svml_ldexp_ha");

// DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powdd", ENHANCED_PRECISION, "__ocl_svml_pow_ep");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powdd", LOW_ACCURACY, "__ocl_svml_pow");
DEF_NAME_TO_BUILTIN("_Z15__spirv_ocl_powdd", HIGH_ACCURACY, "__ocl_svml_pow_ha");

// DEF_NAME_TO_BUILTIN("_spirv_ocl_powndd", ENHANCED_PRECISION, "__ocl_svml_pown_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_powndd", LOW_ACCURACY, "__ocl_svml_pown");
// DEF_NAME_TO_BUILTIN("_spirv_ocl_powndd", HIGH_ACCURACY, "__ocl_svml_pown_ha");

// DEF_NAME_TO_BUILTIN("_spirv_ocl_powrdd", ENHANCED_PRECISION, "__ocl_svml_pow_ep");
DEF_NAME_TO_BUILTIN("_spirv_ocl_powrdd", LOW_ACCURACY, "__ocl_svml_powr");
// DEF_NAME_TO_BUILTIN("_spirv_ocl_powrdd", HIGH_ACCURACY, "__ocl_svml_powr_ha");

// DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotdd", ENHANCED_PRECISION, "__ocl_svml_hypot_ep");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotdd", LOW_ACCURACY, "__ocl_svml_hypot_la");
DEF_NAME_TO_BUILTIN("_Z17__spirv_ocl_hypotdd", HIGH_ACCURACY, "__ocl_svml_hypot_ha");
