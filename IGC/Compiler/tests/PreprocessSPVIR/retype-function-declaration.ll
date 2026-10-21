;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-preprocess-spvir -S < %s | FileCheck %s

; This test verifies that PreprocessSPVIR retypes TargetExtTys in function
; declarations.

; CHECK-LABEL: define spir_kernel void @image_kernel(ptr addrspace(1) %image)
define spir_kernel void @image_kernel(target("spirv.Image", void, 1, 0, 0, 0, 0, 0, 0) %image) {
  ret void
}

; CHECK-LABEL: define spir_kernel void @buffer_kernel(ptr addrspace(1) %buffer)
define spir_kernel void @buffer_kernel(ptr addrspace(1) %buffer) {
  ret void
}

; CHECK-LABEL: define spir_kernel void @sampler_kernel(ptr addrspace(2) %sampler)
define spir_kernel void @sampler_kernel(target("spirv.Sampler") %sampler) {
  ret void
}

; CHECK-LABEL: define spir_func void @foo(i64 %in)
define spir_func void @foo(i64 %in) {
  %img = call spir_func target("spirv.SampledImage", void, 1, 0, 0, 0, 0, 0, 0) @_Z90__spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0m(i64 %in)
  ret void
}

declare spir_func target("spirv.SampledImage", void, 1, 0, 0, 0, 0, 0, 0) @_Z90__spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0m(i64)

; CHECK: declare spir_func ptr addrspace(1) @_Z90__spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0m(i64)
