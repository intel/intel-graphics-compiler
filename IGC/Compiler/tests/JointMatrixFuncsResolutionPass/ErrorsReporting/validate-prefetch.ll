;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flliil(ptr addrspace(1), i64, i64, i32, i32, i64)

define spir_kernel void @matrix_prefetch(ptr addrspace(1) %src) {
; CHECK-LABEL: define spir_kernel void @matrix_prefetch(
  entry:
  ; COM: on opaque pointers the pass infers the element type from the GEP
  %ptr = getelementptr inbounds float, ptr addrspace(1) %src, i64 0
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flliil(ptr addrspace(1) %ptr, i64 52, i64 128, i32 1, i32 0, i64 64)
  ret void
; CHECK: error: in kernel 'matrix_prefetch': Unsupported row parameter for matrix prefetch: 52
; CHECK: error: in kernel 'matrix_prefetch': Unsupported column parameter for matrix prefetch: 128. Supported values: 8, 16, 32, 64.
; CHECK: error: in kernel 'matrix_prefetch': Matrix prefetch size limit exceeded
; CHECK: Limit exceeded with values: 128 * 32 bits = 512B
}

!igc.functions = !{!0}
!0 = !{ptr @matrix_prefetch, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
