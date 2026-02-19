;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc --device-id 0x0BD4 2>&1 | FileCheck %s --check-prefix=CHECK-PVC-VG
; RUN: igc_opt --typed-pointers %s -S -o - -igc-joint-matrix-resolution --platformpvc 2>&1 | FileCheck %s --check-prefix=CHECK-PVC
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

%spirv.JointMatrixINTEL._int_8_16_3_3_2 = type opaque
%spirv.JointMatrixINTEL._char_8_32_0_3_0 = type opaque
%spirv.JointMatrixINTEL._char_32_16_0_3_1 = type opaque

; CHECK-PVC-VG: error: OpJointMatrixMadINTEL/OpCooperativeMatrixMulAddKHR is not supported on this platform!
; CHECK-PVC-NOT: error: OpJointMatrixMadINTEL/OpCooperativeMatrixMulAddKHR is not supported on this platform!

define spir_kernel void @load_store_legacy_error(i8 addrspace(1)* %src, i64 %stride, i32 addrspace(1)* %dst) {
  %1 = call spir_func %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32 0)
  %2 = call spir_func %spirv.JointMatrixINTEL._char_8_32_0_3_0 addrspace(1)* @_Z79__spirv_JointMatrixLoadINTEL_RPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS1cliii(i8 addrspace(1)* %src, i64 %stride, i32 0, i32 3, i32 0)
  %3 = call spir_func %spirv.JointMatrixINTEL._char_32_16_0_3_1 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__char_32_16_0_3_1PU3AS1cliii(i8 addrspace(1)* %src, i64 %stride, i32 0, i32 3, i32 0)
  %4 = call spir_func %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* @_Z27__spirv_JointMatrixMadINTELPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS142__spirv_JointMatrixINTEL__char_32_16_0_3_1PU3AS140__spirv_JointMatrixINTEL__int_8_16_3_3_2i(%spirv.JointMatrixINTEL._char_8_32_0_3_0 addrspace(1)* %2, %spirv.JointMatrixINTEL._char_32_16_0_3_1 addrspace(1)* %3, %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* %1, i32 3)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__int_8_16_3_3_2liii(i32 addrspace(1)* %dst, %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* %4, i64 %stride, i32 0, i32 3, i32 0)
  ret void
}

declare spir_func %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* @_Z26__spirv_CompositeConstructi(i32)
declare spir_func %spirv.JointMatrixINTEL._char_8_32_0_3_0 addrspace(1)* @_Z79__spirv_JointMatrixLoadINTEL_RPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS1cliii(i8 addrspace(1)*, i64, i32, i32, i32)
declare spir_func %spirv.JointMatrixINTEL._char_32_16_0_3_1 addrspace(1)* @_Z80__spirv_JointMatrixLoadINTEL_RPU3AS142__spirv_JointMatrixINTEL__char_32_16_0_3_1PU3AS1cliii(i8 addrspace(1)*, i64, i32, i32, i32)
declare spir_func %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)* @_Z27__spirv_JointMatrixMadINTELPU3AS141__spirv_JointMatrixINTEL__char_8_32_0_3_0PU3AS142__spirv_JointMatrixINTEL__char_32_16_0_3_1PU3AS140__spirv_JointMatrixINTEL__int_8_16_3_3_2i(%spirv.JointMatrixINTEL._char_8_32_0_3_0 addrspace(1)*, %spirv.JointMatrixINTEL._char_32_16_0_3_1 addrspace(1)*, %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)*, i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1iPU3AS140__spirv_JointMatrixINTEL__int_8_16_3_3_2liii(i32 addrspace(1)*, %spirv.JointMatrixINTEL._int_8_16_3_3_2 addrspace(1)*, i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{void (i8 addrspace(1)*, i64, i32 addrspace(1)*)* @load_store_legacy_error, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
