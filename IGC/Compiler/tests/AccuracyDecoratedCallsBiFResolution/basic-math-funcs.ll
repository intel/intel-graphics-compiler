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
; CHECK: = fadd float [[F1]], [[F2]]
; unsupported: = call float @__ocl_svml_fadd_ha(float [[F1]], float [[F2]]) [[A1:#[0-9]*]]
; unsupported: = call float @__ocl_svml_fadd(float [[F1]], float [[F2]]) [[A2:#[0-9]*]]
; unsupported: = call float @__ocl_svml_fadd_ep(float [[F1]], float [[F2]]) [[A3:#[0-9]*]]
; CHECK: = fsub float [[F1]], [[F2]]
; unsupported: = call float @__ocl_svml_fsub_ha(float [[F1]], float [[F2]]) #1
; unsupported: = call float @__ocl_svml_fsub(float [[F1]], float [[F2]]) #2
; unsupported: = call float @__ocl_svml_fsub_ep(float [[F1]], float [[F2]]) #3
; CHECK: = fmul float [[F1]], [[F2]]
; unsupported: = call float @__ocl_svml_fmul_ha(float [[F1]], float [[F2]]) #1
; unsupported: = call float @__ocl_svml_fmul(float [[F1]], float [[F2]]) #2
; unsupported: = call float @__ocl_svml_fmul_ep(float [[F1]], float [[F2]]) #3
; CHECK: = fdiv float [[F1]], [[F2]]
; CHECK: = call float @__builtin_spirv_divide_cr_f32_f32(float [[F1]], float [[F2]])
; CHECK: = fdiv float [[F1]], [[F2]], !fpbuiltin-max-error
; CHECK: = fdiv float [[F1]], [[F2]], !fpbuiltin-max-error
; CHECK: = frem float [[F1]], [[F2]]
; unsupported: = call float @__ocl_svml_frem_ha(float [[F1]], float [[F2]]) #1
; unsupported: = call float @__ocl_svml_frem(float [[F1]], float [[F2]]) #2
; unsupported: = call float @__ocl_svml_frem_ep(float [[F1]], float [[F2]]) #3
  %add0 = fadd float %f1, %f2
  %add1 = fadd float %f1, %f2, !fpbuiltin-max-error !1
  %add2 = fadd float %f1, %f2, !fpbuiltin-max-error !2
  %add3 = fadd float %f1, %f2, !fpbuiltin-max-error !3
  %sub0 = fsub float %f1, %f2
  %sub1 = fsub float %f1, %f2, !fpbuiltin-max-error !1
  %sub2 = fsub float %f1, %f2, !fpbuiltin-max-error !2
  %sub3 = fsub float %f1, %f2, !fpbuiltin-max-error !3
  %mul0 = fmul float %f1, %f2
  %mul1 = fmul float %f1, %f2, !fpbuiltin-max-error !1
  %mul2 = fmul float %f1, %f2, !fpbuiltin-max-error !2
  %mul3 = fmul float %f1, %f2, !fpbuiltin-max-error !3
  %div0 = fdiv float %f1, %f2
  %div1 = fdiv float %f1, %f2, !fpbuiltin-max-error !1
  %div2 = fdiv float %f1, %f2, !fpbuiltin-max-error !2
  %div3 = fdiv float %f1, %f2, !fpbuiltin-max-error !3
  %rem0 = frem float %f1, %f2
  %rem1 = frem float %f1, %f2, !fpbuiltin-max-error !1
  %rem2 = frem float %f1, %f2, !fpbuiltin-max-error !2
  %rem3 = frem float %f1, %f2, !fpbuiltin-max-error !3
  ret void
}

attributes #0 = { nounwind }

!1 = !{!"1.500000"}
!2 = !{!"5.000000"}
!3 = !{!"5000.000000"}
