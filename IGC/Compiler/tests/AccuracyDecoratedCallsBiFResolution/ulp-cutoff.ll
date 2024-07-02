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
define spir_func void @test_fp_max_error_decoration(double %d, float %f) #0 {
; CHECK: @test_fp_max_error_decoration(double [[D:%[A-z0-9]*]], float [[F:%[A-z0-9]*]])
; CHECK: = call spir_func double @__ocl_svml_sin(double [[D]]) #2
; CHECK: = call spir_func float @__ocl_svml_sinf_ep(float [[F]]) #2
  %t0 = call spir_func double @_Z15__spirv_ocl_sind(double %d) #2
  %t1 = call spir_func float @_Z15__spirv_ocl_sinf(float %f) #2
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func float @_Z15__spirv_ocl_sinf(float) #1

; Function Attrs: nounwind readnone
declare spir_func double @_Z15__spirv_ocl_sind(double) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "fpbuiltin-max-error"="4096.500000" }
