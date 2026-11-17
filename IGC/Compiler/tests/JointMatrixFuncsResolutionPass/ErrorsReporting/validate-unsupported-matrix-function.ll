;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - calls the pass can't resolve are reported as
; errors instead of crashing the compiler.
; ------------------------------------------------

; Misspelled builtins (no KHR suffix) are not recognized. Users of their
; results still get resolved.
define spir_kernel void @unknown_function(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %stride) {
; CHECK-LABEL: @unknown_function(
; CHECK-NOT:     __spirv_CooperativeMatrixLoad
; CHECK:         call void @__builtin_spriv_OpJointMatrixMadINTEL_16x16x16_fp16_fp16_fp32_fp32(
; CHECK-NOT:     __spirv_CooperativeMatrixStore
; CHECK:         ret void
  %a = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0) @_Z29__spirv_CooperativeMatrixLoadPU3AS1Dhli(ptr addrspace(1) %src, i64 %stride, i32 0)
  %c = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHR(target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0) %a, target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 1) zeroinitializer, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) zeroinitializer)
  call spir_func void @_Z30__spirv_CooperativeMatrixStorePU3AS1fli(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %c, i64 %stride, i32 0)
  ret void
}

; Layout and stride swapped: layout 16 does not exist.
define spir_kernel void @out_of_range_layout(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %stride) {
; CHECK-LABEL: @out_of_range_layout(
; CHECK-NEXT:    ret void
  %a = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fli(ptr addrspace(1) %src, i64 16, i32 0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fli(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %a, i32 16, i64 %stride)
  ret void
}

define spir_kernel void @out_of_range_prefetch_layout(ptr addrspace(1) %src) {
; CHECK-LABEL: @out_of_range_prefetch_layout(
; CHECK-NEXT:    ret void
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS1fiiiil(ptr addrspace(1) %src, i32 16, i32 16, i32 0, i32 16, i64 16)
  ret void
}

define spir_kernel void @non_constant_layout(ptr addrspace(1) %src, i32 %layout, i64 %stride) {
; CHECK-LABEL: @non_constant_layout(
; CHECK-NOT:     __spirv_CooperativeMatrixLoadKHR
; CHECK:         ret void
  %a = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fii(ptr addrspace(1) %src, i32 %layout, i64 %stride)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fli(ptr addrspace(1) %src, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %a, i32 0, i64 %stride)
  ret void
}

; Declarations taking a matrix that are not matrix builtins.
define spir_kernel void @unknown_declaration(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %stride) {
; CHECK-LABEL: @unknown_declaration(
; CHECK-NOT:     call {{.*}} @external_fn
; CHECK-NOT:     call {{.*}} @external_void_fn
; CHECK:         ret void
  %a = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fii(ptr addrspace(1) %src, i32 0, i64 %stride)
  %b = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @external_fn(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %a)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fli(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %b, i32 0, i64 %stride)
  call spir_func void @external_void_fn(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) %a)
  ret void
}

; CHECK: error: in kernel 'unknown_function': Unsupported matrix function: '__spirv_CooperativeMatrixLoad(half AS1*, long, int)'.
; CHECK: error: in kernel 'unknown_function': Unsupported matrix function: '__spirv_CooperativeMatrixStore(float AS1*, long, int)'.
; CHECK: error: in kernel 'out_of_range_layout': Unsupported matrix load memory layout: 16.
; CHECK: error: in kernel 'out_of_range_layout': Unsupported matrix store memory layout: 16.
; CHECK: error: in kernel 'out_of_range_prefetch_layout': Unsupported matrix prefetch memory layout: 16.
; CHECK: error: in kernel 'non_constant_layout': Matrix load memory layout must be a compile time constant.
; CHECK: error: in kernel 'unknown_declaration': Unsupported matrix function: 'external_fn'.
; CHECK: error: in kernel 'unknown_declaration': Unsupported matrix function: 'external_void_fn'.

declare spir_func target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0) @_Z29__spirv_CooperativeMatrixLoadPU3AS1Dhli(ptr addrspace(1), i64, i32)
declare spir_func void @_Z30__spirv_CooperativeMatrixStorePU3AS1fli(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2), i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHR(target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 0), target("spirv.CooperativeMatrixKHR", half, 3, 16, 16, 1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2))
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fli(ptr addrspace(1), i64, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @_Z32__spirv_CooperativeMatrixLoadKHRPU3AS1fii(ptr addrspace(1), i32, i64)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1fli(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2), i32, i64)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2) @external_fn(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2))
declare spir_func void @external_void_fn(target("spirv.CooperativeMatrixKHR", float, 3, 16, 16, 2))
declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS1fiiiil(ptr addrspace(1), i32, i32, i32, i32, i64)

!igc.functions = !{!0, !3, !4, !5, !6}
!0 = !{ptr @unknown_function, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @out_of_range_layout, !1}
!4 = !{ptr @non_constant_layout, !1}
!5 = !{ptr @unknown_declaration, !1}
!6 = !{ptr @out_of_range_prefetch_layout, !1}
