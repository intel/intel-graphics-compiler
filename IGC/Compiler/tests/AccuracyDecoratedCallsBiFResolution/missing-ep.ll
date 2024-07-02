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
; CHECK: = call spir_func double @__ocl_svml_sinh(double [[D1]]) #2
  %t0 = call spir_func double @_Z16__spirv_ocl_sinhd(double %d1) #2
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func double @_Z16__spirv_ocl_sinhd(double) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "fpbuiltin-max-error"="70000000.000000" }
