;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc --device-id 0x0BD4 2>&1 | FileCheck %s --check-prefix=CHECK-PVC-VG
; ------------------------------------------------
; JointMatrixFuncsResolutionPass - verify whether matrix functionality without usage of
;                                  JointMatrixMadINTEL can be compiled on PVC-VG without
;                                  reporting an error. It uses TargetExtensionType
; ------------------------------------------------

; CHECK-PVC-VG-NOT: error

define spir_kernel void @load_store_without_mad(i8 addrspace(1)* %src, i64 %stride, i8 addrspace(1)* %dst) {
  %1 = call spir_func target("spirv.JointMatrixINTEL", i8, 8, 32, 0, 3, 0) @_Z79__spirv_JointMatrixLoadINTEL_RPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS1cliii(i8 addrspace(1)* %src, i64 %stride, i32 0, i32 3, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL_char_8_32_0_3_0liii(i8 addrspace(1)* %dst, target("spirv.JointMatrixINTEL", i8, 8, 32, 0, 3, 0) %1, i64 %stride, i32 0, i32 3, i32 0)
  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", i8, 8, 32, 0, 3, 0) @_Z79__spirv_JointMatrixLoadINTEL_RPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS1cliii(i8 addrspace(1)*, i64, i32, i32, i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL_char_8_32_0_3_0liii(i8 addrspace(1)*, target("spirv.JointMatrixINTEL", i8, 8, 32, 0, 3, 0), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(1)*, i64, i8 addrspace(1)*)* @load_store_without_mad, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
