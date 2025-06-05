;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

; CHECK-LABEL: define spir_kernel void @test(
; CHECK-SAME: ptr [[PTR1:%.*]], ptr [[PTR2:%.*]]) {
define spir_kernel void @test(ptr %ptr1, ptr %ptr2) {
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <8 x float>
; CHECK-NEXT:    [[TMP2:%.*]] = alloca <8 x float>

; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_16x16_i32_8_generic_v8i8_pi32_i32(ptr [[TMP2]], ptr [[PTR1]], i64 32, i32 0)
; CHECK-NEXT:    [[TMP4:%.*]] = load <8 x float>, ptr [[TMP2]]
  %C1 = call spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(ptr %ptr1, i64 32, i32 0, i32 3, i32 0)

; CHECK-NEXT:    store <8 x float> [[TMP4]], ptr [[TMP1]]
; CHECK-NEXT:    call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_16x16_i32_8_generic_pi64_v8i8(ptr [[PTR2]], ptr [[TMP1]], i64 32, i32 0)
  call spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr %ptr2, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) %C1, i64 32, i32 0, i32 3, i32 0)

; CHECK-NEXT:    ret void
  ret void
}

declare spir_func target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2) @_Z81__spirv_JointMatrixLoadINTEL_RPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2PU3AS1fliii(ptr, i64, i32, i32, i32)
declare spir_func void @_Z29__spirv_JointMatrixStoreINTELPU3AS1fPU3AS143__spirv_JointMatrixINTEL__float_16_16_3_3_2liii(ptr, target("spirv.JointMatrixINTEL", float, 16, 16, 3, 3, 2), i64, i32, i32, i32)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 32}
