;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus

; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S %s 2>&1 | FileCheck %s

define spir_kernel void @test(ptr addrspace(1) %dst) {
  ; CHECK-LABEL: define spir_kernel void @test(ptr addrspace(1) %dst) {
  ; CHECK-NEXT: %1 = alloca <8 x i16>, align 16
  ; CHECK-NEXT: store <8 x i16> <i16 22448, i16 22448, i16 22448, i16 22448, i16 22448, i16 22448, i16 22448, i16 22448>, ptr %1, align 16
  ; CHECK-NEXT: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x16_i16_8_global_pi64_v8i8(ptr addrspace(1) %dst, ptr %1, i64 16, i32 0)
  ; CHECK-NEXT: ret void

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) @__spirv_ConstantComposite_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(half 0xH57B0)
  call spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1DhPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0ii(ptr addrspace(1) %dst, target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) %1, i32 0, i32 16) #0
  ret void
}

declare target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0) @__spirv_ConstantComposite_RPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0(half)
declare spir_func void @_Z33__spirv_CooperativeMatrixStoreKHRPU3AS1DhPU3AS143__spirv_CooperativeMatrixKHR__half_3_8_16_0ii(ptr addrspace(1), target("spirv.CooperativeMatrixKHR", half, 3, 8, 16, 0), i32, i32) #0

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
