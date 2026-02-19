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

%spirv.JointMatrixINTEL._short_7_66_0_3_0 = type opaque
declare spir_func %spirv.JointMatrixINTEL._short_7_66_0_3_0 addrspace(3)* @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__short_7_66_0_3_0PU3AS1ciiiiili(i8 addrspace(3)*, i32, i32, i32, i32, i32, i64, i32)

define spir_kernel void @matrix_load_checked(i8 addrspace(3)* %src) {
; CHECK-LABEL: define spir_kernel void @matrix_load_checked(
  %1 = call spir_func %spirv.JointMatrixINTEL._short_7_66_0_3_0 addrspace(3)* @_Z93__spirv_CooperativeMatrixLoadCheckedINTEL_RPU3AS142__spirv_JointMatrixINTEL__short_7_66_0_3_0PU3AS1ciiiiili(i8 addrspace(3)* %src, i32 0, i32 0, i32 0, i32 16, i32 16, i64 16, i32 0) #0
  ret void
; CHECK: error: Unsupported address space. Matrix checked load supports generic and global pointers.
; CHECK: error: Unsupported row parameter for matrix checked load: 7. Supported values: 1, 2, 4, 8, 16, 32.
; CHECK: error: Unsupported column parameter for matrix checked load: 66. Supported values: 8, 16, 32, 64.
; CHECK: error: Matrix checked load size limit exceeded.
; CHECK: Limit exceeded with values: 66 * 2B = 132B
}

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(3)*)* @matrix_load_checked, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
