;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-accuracy-decorated-calls-bif-resolution -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Function Attrs: nounwind
define spir_func void @test_hf_max_error_decoration(half %f1, half %f2) #0 {
; CHECK: @test_hf_max_error_decoration(half [[F1:%[A-z0-9]*]], half [[F2:%[A-z0-9]*]])
; CHECK: = call spir_func half @_Z16__spirv_ocl_sqrtDh(half [[F1]])
; CHECK: = call spir_func half @_Z16__spirv_ocl_sqrtDh(half [[F1]]) #2
; CHECK: = call spir_func half @_Z16__spirv_ocl_sqrtDh(half [[F1]]) #3
; CHECK: = call spir_func half @_Z16__spirv_ocl_sqrtDh(half [[F1]]) #4
; CHECK: = call spir_func half @_Z19__spirv_ocl_sqrt_crDh(half [[F1]]) #5

  %t80 = call spir_func half @_Z16__spirv_ocl_sqrtDh(half %f1)
  %t81 = call spir_func half @_Z16__spirv_ocl_sqrtDh(half %f1) #2
  %t82 = call spir_func half @_Z16__spirv_ocl_sqrtDh(half %f1) #3
  %t83 = call spir_func half @_Z16__spirv_ocl_sqrtDh(half %f1) #4
  %t84 = call spir_func half @_Z16__spirv_ocl_sqrtDh(half %f1) #5
  ret void
}

; Function Attrs: nounwind readnone
declare spir_func half @_Z16__spirv_ocl_sqrtDh(half) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "fpbuiltin-max-error"="2.500000" }
attributes #3 = { nounwind "fpbuiltin-max-error"="4.000000" }
attributes #4 = { nounwind "fpbuiltin-max-error"="5000.000000" }
attributes #5 = { nounwind "fpbuiltin-max-error"="0.500000" }
