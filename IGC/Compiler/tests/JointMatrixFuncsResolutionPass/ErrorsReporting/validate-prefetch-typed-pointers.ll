;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-joint-matrix-resolution --platformpvc -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

declare spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flliil(float addrspace(1)*, i64, i64, i32, i32, i64)

define spir_kernel void @matrix_prefetch(float addrspace(1)* %src) {
; CHECK-LABEL: define spir_kernel void @matrix_prefetch(
  entry:
  call spir_func void @_Z38__spirv_CooperativeMatrixPrefetchINTELPU3AS4flliil(float addrspace(1)* %src, i64 52, i64 128, i32 1, i32 0, i64 64)
  ret void
; CHECK: error: in kernel 'matrix_prefetch': Unsupported row parameter for matrix prefetch: 52
; CHECK: error: in kernel 'matrix_prefetch': Unsupported column parameter for matrix prefetch: 128. Supported values: 8, 16, 32, 64.
; CHECK: error: in kernel 'matrix_prefetch': Matrix prefetch size limit exceeded
; CHECK: Limit exceeded with values: 128 * 4B = 512B
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}
!0 = !{void (float addrspace(1)*)* @matrix_prefetch, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = distinct !{!"FuncMDMap[0]", void (float addrspace(1)*)* @matrix_prefetch}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"funcArgs"}
!9 = !{!"functionType", !"KernelFunction"}
