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
define spir_func void @test_fp_max_error_decoration(double %d1, double %d2) #0 {
; CHECK: @test_fp_max_error_decoration(double [[D1:%[A-z0-9]*]], double [[D2:%[A-z0-9]*]])
; CHECK: = call spir_func double @_Z15__spirv_ocl_sind(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_sin_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_sin(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_sin_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z15__spirv_ocl_cosd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_cos_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_cos(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_cos_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z15__spirv_ocl_tand(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_tan_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_tan(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_tan_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z16__spirv_ocl_sinhd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_sinh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_sinh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_coshd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_cosh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_cosh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_tanhd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_tanh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_tanh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_asind(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_asin_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_asin(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_acosd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_acos_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_acos(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_atand(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_atan_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_atan(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_asinhd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_asinh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_asinh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_acoshd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_acosh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_acosh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_atanhd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_atanh_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_atanh(double [[D1]]) #3
; CHECK: = call spir_func double @_Z15__spirv_ocl_expd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_exp_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_exp(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_exp_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z16__spirv_ocl_exp2d(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_exp2_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_exp2(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_exp2_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z17__spirv_ocl_exp10d(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_exp10_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_exp10(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_exp10_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z17__spirv_ocl_expm1d(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_expm1_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_expm1(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_expm1_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z15__spirv_ocl_logd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_log_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_log(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_log_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z16__spirv_ocl_log2d(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_log2_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_log2_v2(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_log10d(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_log10_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_log10_v2(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_log1pd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_log1p_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_log1p(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_sqrtd(double [[D1]])
; CHECK: = call spir_func double @_Z16__spirv_ocl_sqrtd(double [[D1]]) #2
; CHECK: = call spir_func double @_Z16__spirv_ocl_sqrtd(double [[D1]]) #3
; CHECK: = call spir_func double @_Z16__spirv_ocl_sqrtd(double [[D1]]) #4
; CHECK: = call spir_func double @_Z16__spirv_ocl_sqrtd(double [[D1]]) #5
; CHECK: = call spir_func double @_Z17__spirv_ocl_rsqrtd(double [[D1]])
; CHECK: = call spir_func double @_Z15__spirv_ocl_erfd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_erf_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_erf(double [[D1]]) #3
; CHECK: = call spir_func double @__ocl_svml_erf_ep(double [[D1]]) #4
; CHECK: = call spir_func double @_Z16__spirv_ocl_erfcd(double [[D1]])
; CHECK: = call spir_func double @__ocl_svml_erfc_ha(double [[D1]]) #2
; CHECK: = call spir_func double @__ocl_svml_erfc(double [[D1]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_atan2dd(double [[D1]], double [[D2]])
; CHECK: = call spir_func double @__ocl_svml_atan2_ha(double [[D1]], double [[D2]]) #2
; CHECK: = call spir_func double @__ocl_svml_atan2(double [[D1]], double [[D2]]) #3
; CHECK: = call spir_func double @__ocl_svml_atan2_ep(double [[D1]], double [[D2]]) #4
; CHECK: = call spir_func double @_Z17__spirv_ocl_ldexpdd(double [[D1]], double [[D2]])
; CHECK: = call spir_func double @_Z15__spirv_ocl_powdd(double [[D1]], double [[D2]])
; CHECK: = call spir_func double @__ocl_svml_pow_ha(double [[D1]], double [[D2]]) #2
; CHECK: = call spir_func double @__ocl_svml_pow(double [[D1]], double [[D2]]) #3
; CHECK: = call spir_func double @_Z17__spirv_ocl_hypotdd(double [[D1]], double [[D2]])
; CHECK: = call spir_func double @__ocl_svml_hypot_ha(double [[D1]], double [[D2]]) #2
; CHECK: = call spir_func double @__ocl_svml_hypot_la(double [[D1]], double [[D2]]) #3
  %t0 = call spir_func double @_Z15__spirv_ocl_sind(double %d1)
  %t1 = call spir_func double @_Z15__spirv_ocl_sind(double %d1) #2
  %t2 = call spir_func double @_Z15__spirv_ocl_sind(double %d1) #3
  %t3 = call spir_func double @_Z15__spirv_ocl_sind(double %d1) #4
  %t4 = call spir_func double @_Z15__spirv_ocl_cosd(double %d1)
  %t5 = call spir_func double @_Z15__spirv_ocl_cosd(double %d1) #2
  %t6 = call spir_func double @_Z15__spirv_ocl_cosd(double %d1) #3
  %t7 = call spir_func double @_Z15__spirv_ocl_cosd(double %d1) #4
  %t8 = call spir_func double @_Z15__spirv_ocl_tand(double %d1)
  %t9 = call spir_func double @_Z15__spirv_ocl_tand(double %d1) #2
  %t10 = call spir_func double @_Z15__spirv_ocl_tand(double %d1) #3
  %t11 = call spir_func double @_Z15__spirv_ocl_tand(double %d1) #4
  %t12 = call spir_func double @_Z16__spirv_ocl_sinhd(double %d1)
  %t13 = call spir_func double @_Z16__spirv_ocl_sinhd(double %d1) #2
  %t14 = call spir_func double @_Z16__spirv_ocl_sinhd(double %d1) #3
  %t15 = call spir_func double @_Z16__spirv_ocl_sinhd(double %d1) #4
  %t16 = call spir_func double @_Z16__spirv_ocl_coshd(double %d1)
  %t17 = call spir_func double @_Z16__spirv_ocl_coshd(double %d1) #2
  %t18 = call spir_func double @_Z16__spirv_ocl_coshd(double %d1) #3
  %t19 = call spir_func double @_Z16__spirv_ocl_coshd(double %d1) #4
  %t20 = call spir_func double @_Z16__spirv_ocl_tanhd(double %d1)
  %t21 = call spir_func double @_Z16__spirv_ocl_tanhd(double %d1) #2
  %t22 = call spir_func double @_Z16__spirv_ocl_tanhd(double %d1) #3
  %t23 = call spir_func double @_Z16__spirv_ocl_tanhd(double %d1) #4
  %t24 = call spir_func double @_Z16__spirv_ocl_asind(double %d1)
  %t25 = call spir_func double @_Z16__spirv_ocl_asind(double %d1) #2
  %t26 = call spir_func double @_Z16__spirv_ocl_asind(double %d1) #3
  %t27 = call spir_func double @_Z16__spirv_ocl_asind(double %d1) #4
  %t28 = call spir_func double @_Z16__spirv_ocl_acosd(double %d1)
  %t29 = call spir_func double @_Z16__spirv_ocl_acosd(double %d1) #2
  %t30 = call spir_func double @_Z16__spirv_ocl_acosd(double %d1) #3
  %t31 = call spir_func double @_Z16__spirv_ocl_acosd(double %d1) #4
  %t32 = call spir_func double @_Z16__spirv_ocl_atand(double %d1)
  %t33 = call spir_func double @_Z16__spirv_ocl_atand(double %d1) #2
  %t34 = call spir_func double @_Z16__spirv_ocl_atand(double %d1) #3
  %t35 = call spir_func double @_Z16__spirv_ocl_atand(double %d1) #4
  %t36 = call spir_func double @_Z17__spirv_ocl_asinhd(double %d1)
  %t37 = call spir_func double @_Z17__spirv_ocl_asinhd(double %d1) #2
  %t38 = call spir_func double @_Z17__spirv_ocl_asinhd(double %d1) #3
  %t39 = call spir_func double @_Z17__spirv_ocl_asinhd(double %d1) #4
  %t40 = call spir_func double @_Z17__spirv_ocl_acoshd(double %d1)
  %t41 = call spir_func double @_Z17__spirv_ocl_acoshd(double %d1) #2
  %t42 = call spir_func double @_Z17__spirv_ocl_acoshd(double %d1) #3
  %t43 = call spir_func double @_Z17__spirv_ocl_acoshd(double %d1) #4
  %t44 = call spir_func double @_Z17__spirv_ocl_atanhd(double %d1)
  %t45 = call spir_func double @_Z17__spirv_ocl_atanhd(double %d1) #2
  %t46 = call spir_func double @_Z17__spirv_ocl_atanhd(double %d1) #3
  %t47 = call spir_func double @_Z17__spirv_ocl_atanhd(double %d1) #4
  %t48 = call spir_func double @_Z15__spirv_ocl_expd(double %d1)
  %t49 = call spir_func double @_Z15__spirv_ocl_expd(double %d1) #2
  %t50 = call spir_func double @_Z15__spirv_ocl_expd(double %d1) #3
  %t51 = call spir_func double @_Z15__spirv_ocl_expd(double %d1) #4
  %t52 = call spir_func double @_Z16__spirv_ocl_exp2d(double %d1)
  %t53 = call spir_func double @_Z16__spirv_ocl_exp2d(double %d1) #2
  %t54 = call spir_func double @_Z16__spirv_ocl_exp2d(double %d1) #3
  %t55 = call spir_func double @_Z16__spirv_ocl_exp2d(double %d1) #4
  %t56 = call spir_func double @_Z17__spirv_ocl_exp10d(double %d1)
  %t57 = call spir_func double @_Z17__spirv_ocl_exp10d(double %d1) #2
  %t58 = call spir_func double @_Z17__spirv_ocl_exp10d(double %d1) #3
  %t59 = call spir_func double @_Z17__spirv_ocl_exp10d(double %d1) #4
  %t60 = call spir_func double @_Z17__spirv_ocl_expm1d(double %d1)
  %t61 = call spir_func double @_Z17__spirv_ocl_expm1d(double %d1) #2
  %t62 = call spir_func double @_Z17__spirv_ocl_expm1d(double %d1) #3
  %t63 = call spir_func double @_Z17__spirv_ocl_expm1d(double %d1) #4
  %t64 = call spir_func double @_Z15__spirv_ocl_logd(double %d1)
  %t65 = call spir_func double @_Z15__spirv_ocl_logd(double %d1) #2
  %t66 = call spir_func double @_Z15__spirv_ocl_logd(double %d1) #3
  %t67 = call spir_func double @_Z15__spirv_ocl_logd(double %d1) #4
  %t68 = call spir_func double @_Z16__spirv_ocl_log2d(double %d1)
  %t69 = call spir_func double @_Z16__spirv_ocl_log2d(double %d1) #2
  %t70 = call spir_func double @_Z16__spirv_ocl_log2d(double %d1) #3
  %t71 = call spir_func double @_Z16__spirv_ocl_log2d(double %d1) #4
  %t72 = call spir_func double @_Z17__spirv_ocl_log10d(double %d1)
  %t73 = call spir_func double @_Z17__spirv_ocl_log10d(double %d1) #2
  %t74 = call spir_func double @_Z17__spirv_ocl_log10d(double %d1) #3
  %t75 = call spir_func double @_Z17__spirv_ocl_log10d(double %d1) #4
  %t76 = call spir_func double @_Z17__spirv_ocl_log1pd(double %d1)
  %t77 = call spir_func double @_Z17__spirv_ocl_log1pd(double %d1) #2
  %t78 = call spir_func double @_Z17__spirv_ocl_log1pd(double %d1) #3
  %t79 = call spir_func double @_Z17__spirv_ocl_log1pd(double %d1) #4
  %t80 = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d1)
  %t81 = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d1) #2
  %t82 = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d1) #3
  %t83 = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d1) #4
  %t84 = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d1) #5
  %t85 = call spir_func double @_Z17__spirv_ocl_rsqrtd(double %d1)
  %t86 = call spir_func double @_Z17__spirv_ocl_rsqrtd(double %d1) #2
  %t87 = call spir_func double @_Z17__spirv_ocl_rsqrtd(double %d1) #3
  %t88 = call spir_func double @_Z17__spirv_ocl_rsqrtd(double %d1) #4
  %t89 = call spir_func double @_Z15__spirv_ocl_erfd(double %d1)
  %t90 = call spir_func double @_Z15__spirv_ocl_erfd(double %d1) #2
  %t91 = call spir_func double @_Z15__spirv_ocl_erfd(double %d1) #3
  %t92 = call spir_func double @_Z15__spirv_ocl_erfd(double %d1) #4
  %t93 = call spir_func double @_Z16__spirv_ocl_erfcd(double %d1)
  %t94 = call spir_func double @_Z16__spirv_ocl_erfcd(double %d1) #2
  %t95 = call spir_func double @_Z16__spirv_ocl_erfcd(double %d1) #3
  %t96 = call spir_func double @_Z16__spirv_ocl_erfcd(double %d1) #4
  %t97 = call spir_func double @_Z17__spirv_ocl_atan2dd(double %d1, double %d2)
  %t98 = call spir_func double @_Z17__spirv_ocl_atan2dd(double %d1, double %d2) #2
  %t99 = call spir_func double @_Z17__spirv_ocl_atan2dd(double %d1, double %d2) #3
  %t100 = call spir_func double @_Z17__spirv_ocl_atan2dd(double %d1, double %d2) #4
  %t101 = call spir_func double @_Z17__spirv_ocl_ldexpdd(double %d1, double %d2)
  %t102 = call spir_func double @_Z17__spirv_ocl_ldexpdd(double %d1, double %d2) #2
  %t103 = call spir_func double @_Z17__spirv_ocl_ldexpdd(double %d1, double %d2) #3
  %t104 = call spir_func double @_Z17__spirv_ocl_ldexpdd(double %d1, double %d2) #4
  %t105 = call spir_func double @_Z15__spirv_ocl_powdd(double %d1, double %d2)
  %t106 = call spir_func double @_Z15__spirv_ocl_powdd(double %d1, double %d2) #2
  %t107 = call spir_func double @_Z15__spirv_ocl_powdd(double %d1, double %d2) #3
  %t108 = call spir_func double @_Z15__spirv_ocl_powdd(double %d1, double %d2) #4
  %t109 = call spir_func double @_Z17__spirv_ocl_hypotdd(double %d1, double %d2)
  %t110 = call spir_func double @_Z17__spirv_ocl_hypotdd(double %d1, double %d2) #2
  %t111 = call spir_func double @_Z17__spirv_ocl_hypotdd(double %d1, double %d2) #3
  %t112 = call spir_func double @_Z17__spirv_ocl_hypotdd(double %d1, double %d2) #4
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_sind(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_cosd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_tand(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_sinhd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_coshd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_tanhd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_asind(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_acosd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_atand(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_asinhd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_acoshd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_atanhd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_expd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_exp2d(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_exp10d(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_expm1d(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_logd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_log2d(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_log10d(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_log1pd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_sqrtd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_rsqrtd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_erfd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_erfcd(double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_atan2dd(double, double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z17__spirv_ocl_ldexpdd(double, double) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_powdd(double, double) #1

; Function Attrs: nounwind
declare spir_func double @_Z17__spirv_ocl_hypotdd(double, double) #0

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "fpbuiltin-max-error"="2.500000" }
attributes #3 = { nounwind "fpbuiltin-max-error"="40000.000000" }
attributes #4 = { nounwind "fpbuiltin-max-error"="70000000.000000" }
attributes #5 = { nounwind "fpbuiltin-max-error"="0.5" }
