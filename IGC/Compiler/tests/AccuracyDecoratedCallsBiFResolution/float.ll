;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-accuracy-decorated-calls-bif-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Function Attrs: nounwind
define spir_func void @test_fp_max_error_decoration(float %f1, float %f2) #0 {
; CHECK: @test_fp_max_error_decoration(float [[F1:%[A-z0-9]*]], float [[F2:%[A-z0-9]*]])
; CHECK: = call spir_func float @_Z15__spirv_ocl_sinf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_sinf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_sinf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_sinf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z15__spirv_ocl_cosf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_cosf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_cosf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_cosf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z15__spirv_ocl_tanf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_tanf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_tanf(float [[F1]]) #3
; CHECK: = call spir_func float @_Z16__spirv_ocl_sinhf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_sinhf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_sinhf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_sinhf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_coshf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_coshf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_coshf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_coshf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_tanhf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_tanhf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_tanhf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_tanhf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_asinf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_asinf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_asinf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_asinf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_acosf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_acosf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_acosf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_acosf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_atanf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_atanf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_atanf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_atanf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_asinhf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_asinhf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_asinhf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_asinhf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_acoshf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_acoshf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_acoshf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_acoshf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_atanhf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_atanhf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_atanhf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_atanhf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z15__spirv_ocl_expf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_expf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_expf(float [[F1]]) #3
; CHECK: = call spir_func float @_Z16__spirv_ocl_exp2f(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_exp2f_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_exp2f(float [[F1]]) #3
; CHECK: = call spir_func float @_Z17__spirv_ocl_exp10f(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_exp10f_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_exp10f(float [[F1]]) #3
; CHECK: = call spir_func float @_Z17__spirv_ocl_expm1f(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_expm1f_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_expm1f(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_expm1f_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z15__spirv_ocl_logf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_logf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_logf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_logf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_log2f(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_log2f_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_log2f(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_log2f_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_log10f(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_log10f_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_log10f(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_log10f_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_log1pf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_log1pf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_log1pf(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_log1pf_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_sqrtf(float [[F1]])
; CHECK: = call spir_func float @_Z17__spirv_ocl_rsqrtf(float [[F1]])
; CHECK: = call spir_func float @_Z15__spirv_ocl_erff(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_erff_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_erff(float [[F1]]) #3
; CHECK: = call spir_func float @__ocl_svml_erff_ep(float [[F1]]) #4
; CHECK: = call spir_func float @_Z16__spirv_ocl_erfcf(float [[F1]])
; CHECK: = call spir_func float @__ocl_svml_erfcf_ha(float [[F1]]) #2
; CHECK: = call spir_func float @__ocl_svml_erfcf(float [[F1]]) #3
; CHECK: = call spir_func float @_Z17__spirv_ocl_atan2ff(float [[F1]], float [[F2]])
; CHECK: = call spir_func float @__ocl_svml_atan2f_ha(float [[F1]], float [[F2]]) #2
; CHECK: = call spir_func float @__ocl_svml_atan2f(float [[F1]], float [[F2]]) #3
; CHECK: = call spir_func float @__ocl_svml_atan2f_ep(float [[F1]], float [[F2]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_ldexpff(float [[F1]], float [[F2]])
; CHECK: = call spir_func float @_Z15__spirv_ocl_powff(float [[F1]], float [[F2]])
; CHECK: = call spir_func float @__ocl_svml_powf_ha(float [[F1]], float [[F2]]) #2
; CHECK: = call spir_func float @__ocl_svml_powf(float [[F1]], float [[F2]]) #3
; CHECK: = call spir_func float @__ocl_svml_powf_ep(float [[F1]], float [[F2]]) #4
; CHECK: = call spir_func float @_Z17__spirv_ocl_hypotff(float [[F1]], float [[F2]])
; CHECK: = call spir_func float @__ocl_svml_hypotf_ha(float [[F1]], float [[F2]]) #2
; CHECK: = call spir_func float @__ocl_svml_hypotf_la(float [[F1]], float [[F2]]) #3
  %t0 = call spir_func float @_Z15__spirv_ocl_sinf(float %f1)
  %t1 = call spir_func float @_Z15__spirv_ocl_sinf(float %f1) #2
  %t2 = call spir_func float @_Z15__spirv_ocl_sinf(float %f1) #3
  %t3 = call spir_func float @_Z15__spirv_ocl_sinf(float %f1) #4
  %t4 = call spir_func float @_Z15__spirv_ocl_cosf(float %f1)
  %t5 = call spir_func float @_Z15__spirv_ocl_cosf(float %f1) #2
  %t6 = call spir_func float @_Z15__spirv_ocl_cosf(float %f1) #3
  %t7 = call spir_func float @_Z15__spirv_ocl_cosf(float %f1) #4
  %t8 = call spir_func float @_Z15__spirv_ocl_tanf(float %f1)
  %t9 = call spir_func float @_Z15__spirv_ocl_tanf(float %f1) #2
  %t10 = call spir_func float @_Z15__spirv_ocl_tanf(float %f1) #3
  %t11 = call spir_func float @_Z15__spirv_ocl_tanf(float %f1) #4
  %t12 = call spir_func float @_Z16__spirv_ocl_sinhf(float %f1)
  %t13 = call spir_func float @_Z16__spirv_ocl_sinhf(float %f1) #2
  %t14 = call spir_func float @_Z16__spirv_ocl_sinhf(float %f1) #3
  %t15 = call spir_func float @_Z16__spirv_ocl_sinhf(float %f1) #4
  %t16 = call spir_func float @_Z16__spirv_ocl_coshf(float %f1)
  %t17 = call spir_func float @_Z16__spirv_ocl_coshf(float %f1) #2
  %t18 = call spir_func float @_Z16__spirv_ocl_coshf(float %f1) #3
  %t19 = call spir_func float @_Z16__spirv_ocl_coshf(float %f1) #4
  %t20 = call spir_func float @_Z16__spirv_ocl_tanhf(float %f1)
  %t21 = call spir_func float @_Z16__spirv_ocl_tanhf(float %f1) #2
  %t22 = call spir_func float @_Z16__spirv_ocl_tanhf(float %f1) #3
  %t23 = call spir_func float @_Z16__spirv_ocl_tanhf(float %f1) #4
  %t24 = call spir_func float @_Z16__spirv_ocl_asinf(float %f1)
  %t25 = call spir_func float @_Z16__spirv_ocl_asinf(float %f1) #2
  %t26 = call spir_func float @_Z16__spirv_ocl_asinf(float %f1) #3
  %t27 = call spir_func float @_Z16__spirv_ocl_asinf(float %f1) #4
  %t28 = call spir_func float @_Z16__spirv_ocl_acosf(float %f1)
  %t29 = call spir_func float @_Z16__spirv_ocl_acosf(float %f1) #2
  %t30 = call spir_func float @_Z16__spirv_ocl_acosf(float %f1) #3
  %t31 = call spir_func float @_Z16__spirv_ocl_acosf(float %f1) #4
  %t32 = call spir_func float @_Z16__spirv_ocl_atanf(float %f1)
  %t33 = call spir_func float @_Z16__spirv_ocl_atanf(float %f1) #2
  %t34 = call spir_func float @_Z16__spirv_ocl_atanf(float %f1) #3
  %t35 = call spir_func float @_Z16__spirv_ocl_atanf(float %f1) #4
  %t36 = call spir_func float @_Z17__spirv_ocl_asinhf(float %f1)
  %t37 = call spir_func float @_Z17__spirv_ocl_asinhf(float %f1) #2
  %t38 = call spir_func float @_Z17__spirv_ocl_asinhf(float %f1) #3
  %t39 = call spir_func float @_Z17__spirv_ocl_asinhf(float %f1) #4
  %t40 = call spir_func float @_Z17__spirv_ocl_acoshf(float %f1)
  %t41 = call spir_func float @_Z17__spirv_ocl_acoshf(float %f1) #2
  %t42 = call spir_func float @_Z17__spirv_ocl_acoshf(float %f1) #3
  %t43 = call spir_func float @_Z17__spirv_ocl_acoshf(float %f1) #4
  %t44 = call spir_func float @_Z17__spirv_ocl_atanhf(float %f1)
  %t45 = call spir_func float @_Z17__spirv_ocl_atanhf(float %f1) #2
  %t46 = call spir_func float @_Z17__spirv_ocl_atanhf(float %f1) #3
  %t47 = call spir_func float @_Z17__spirv_ocl_atanhf(float %f1) #4
  %t48 = call spir_func float @_Z15__spirv_ocl_expf(float %f1)
  %t49 = call spir_func float @_Z15__spirv_ocl_expf(float %f1) #2
  %t50 = call spir_func float @_Z15__spirv_ocl_expf(float %f1) #3
  %t51 = call spir_func float @_Z15__spirv_ocl_expf(float %f1) #4
  %t52 = call spir_func float @_Z16__spirv_ocl_exp2f(float %f1)
  %t53 = call spir_func float @_Z16__spirv_ocl_exp2f(float %f1) #2
  %t54 = call spir_func float @_Z16__spirv_ocl_exp2f(float %f1) #3
  %t55 = call spir_func float @_Z16__spirv_ocl_exp2f(float %f1) #4
  %t56 = call spir_func float @_Z17__spirv_ocl_exp10f(float %f1)
  %t57 = call spir_func float @_Z17__spirv_ocl_exp10f(float %f1) #2
  %t58 = call spir_func float @_Z17__spirv_ocl_exp10f(float %f1) #3
  %t59 = call spir_func float @_Z17__spirv_ocl_exp10f(float %f1) #4
  %t60 = call spir_func float @_Z17__spirv_ocl_expm1f(float %f1)
  %t61 = call spir_func float @_Z17__spirv_ocl_expm1f(float %f1) #2
  %t62 = call spir_func float @_Z17__spirv_ocl_expm1f(float %f1) #3
  %t63 = call spir_func float @_Z17__spirv_ocl_expm1f(float %f1) #4
  %t64 = call spir_func float @_Z15__spirv_ocl_logf(float %f1)
  %t65 = call spir_func float @_Z15__spirv_ocl_logf(float %f1) #2
  %t66 = call spir_func float @_Z15__spirv_ocl_logf(float %f1) #3
  %t67 = call spir_func float @_Z15__spirv_ocl_logf(float %f1) #4
  %t68 = call spir_func float @_Z16__spirv_ocl_log2f(float %f1)
  %t69 = call spir_func float @_Z16__spirv_ocl_log2f(float %f1) #2
  %t70 = call spir_func float @_Z16__spirv_ocl_log2f(float %f1) #3
  %t71 = call spir_func float @_Z16__spirv_ocl_log2f(float %f1) #4
  %t72 = call spir_func float @_Z17__spirv_ocl_log10f(float %f1)
  %t73 = call spir_func float @_Z17__spirv_ocl_log10f(float %f1) #2
  %t74 = call spir_func float @_Z17__spirv_ocl_log10f(float %f1) #3
  %t75 = call spir_func float @_Z17__spirv_ocl_log10f(float %f1) #4
  %t76 = call spir_func float @_Z17__spirv_ocl_log1pf(float %f1)
  %t77 = call spir_func float @_Z17__spirv_ocl_log1pf(float %f1) #2
  %t78 = call spir_func float @_Z17__spirv_ocl_log1pf(float %f1) #3
  %t79 = call spir_func float @_Z17__spirv_ocl_log1pf(float %f1) #4
  %t80 = call spir_func float @_Z16__spirv_ocl_sqrtf(float %f1)
  %t81 = call spir_func float @_Z16__spirv_ocl_sqrtf(float %f1) #2
  %t82 = call spir_func float @_Z16__spirv_ocl_sqrtf(float %f1) #3
  %t83 = call spir_func float @_Z16__spirv_ocl_sqrtf(float %f1) #4
  %t84 = call spir_func float @_Z17__spirv_ocl_rsqrtf(float %f1)
  %t85 = call spir_func float @_Z17__spirv_ocl_rsqrtf(float %f1) #2
  %t86 = call spir_func float @_Z17__spirv_ocl_rsqrtf(float %f1) #3
  %t87 = call spir_func float @_Z17__spirv_ocl_rsqrtf(float %f1) #4
  %t88 = call spir_func float @_Z15__spirv_ocl_erff(float %f1)
  %t89 = call spir_func float @_Z15__spirv_ocl_erff(float %f1) #2
  %t90 = call spir_func float @_Z15__spirv_ocl_erff(float %f1) #3
  %t91 = call spir_func float @_Z15__spirv_ocl_erff(float %f1) #4
  %t92 = call spir_func float @_Z16__spirv_ocl_erfcf(float %f1)
  %t93 = call spir_func float @_Z16__spirv_ocl_erfcf(float %f1) #2
  %t94 = call spir_func float @_Z16__spirv_ocl_erfcf(float %f1) #3
  %t95 = call spir_func float @_Z16__spirv_ocl_erfcf(float %f1) #4
  %t96 = call spir_func float @_Z17__spirv_ocl_atan2ff(float %f1, float %f2)
  %t97 = call spir_func float @_Z17__spirv_ocl_atan2ff(float %f1, float %f2) #2
  %t98 = call spir_func float @_Z17__spirv_ocl_atan2ff(float %f1, float %f2) #3
  %t99 = call spir_func float @_Z17__spirv_ocl_atan2ff(float %f1, float %f2) #4
  %t100 = call spir_func float @_Z17__spirv_ocl_ldexpff(float %f1, float %f2)
  %t101 = call spir_func float @_Z17__spirv_ocl_ldexpff(float %f1, float %f2) #2
  %t102 = call spir_func float @_Z17__spirv_ocl_ldexpff(float %f1, float %f2) #3
  %t103 = call spir_func float @_Z17__spirv_ocl_ldexpff(float %f1, float %f2) #4
  %t104 = call spir_func float @_Z15__spirv_ocl_powff(float %f1, float %f2)
  %t105 = call spir_func float @_Z15__spirv_ocl_powff(float %f1, float %f2) #2
  %t106 = call spir_func float @_Z15__spirv_ocl_powff(float %f1, float %f2) #3
  %t107 = call spir_func float @_Z15__spirv_ocl_powff(float %f1, float %f2) #4
  %t108 = call spir_func float @_Z17__spirv_ocl_hypotff(float %f1, float %f2)
  %t109 = call spir_func float @_Z17__spirv_ocl_hypotff(float %f1, float %f2) #2
  %t110 = call spir_func float @_Z17__spirv_ocl_hypotff(float %f1, float %f2) #3
  %t111 = call spir_func float @_Z17__spirv_ocl_hypotff(float %f1, float %f2) #4
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_sinf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_cosf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_tanf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_sinhf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_coshf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_tanhf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_asinf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_acosf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_atanf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_asinhf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_acoshf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_atanhf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_expf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_exp2f(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_exp10f(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_expm1f(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_logf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_log2f(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_log10f(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_log1pf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_sqrtf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_rsqrtf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_erff(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z16__spirv_ocl_erfcf(float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_atan2ff(float, float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z17__spirv_ocl_ldexpff(float, float) #1

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_powff(float, float) #1

; Function Attrs: nounwind
declare spir_func float @_Z17__spirv_ocl_hypotff(float, float) #0

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "fpbuiltin-max-error"="2.500000" }
attributes #3 = { nounwind "fpbuiltin-max-error"="4.000000" }
attributes #4 = { nounwind "fpbuiltin-max-error"="5000.000000" }
