;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -platformpvc -igc-joint-matrix-resolution -dce -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------

define spir_kernel void @test_generic(ptr %src, ptr %dst) {
  call void @load_store_generic(ptr %src, ptr %dst)
  call void @load_store_large_generic(ptr %src, ptr %dst)
  ret void
}

define spir_kernel void @test_global(ptr %src, ptr %dst) {
  call void @load_store_global(ptr %src, ptr %dst)
  call void @load_store_large_global(ptr %src, ptr %dst)
  ret void
}

define spir_kernel void @test_local(ptr  %src, ptr %dst) {
  call void @load_store_local(ptr %src, ptr %dst)
  call void @load_store_large_local(ptr %src, ptr %dst)
  ret void
}

; CHECK-LABEL: define void @load_store_generic(
define void @load_store_generic(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_generic(ptr %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_generic(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_generic(
define void @load_store_large_generic(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 16, i32 0)
; CHECK: [[MATRIX:%.*]] = load { <64 x float>, <64 x float> }, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_generic(ptr %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_generic(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_global(
define void @load_store_global(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_global(ptr %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_global(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_global(
define void @load_store_large_global(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 16, i32 0)
; CHECK: [[MATRIX:%.*]] = load { <64 x float>, <64 x float> }, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_global(ptr %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_global(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_local(
define void @load_store_local(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca <4 x i32>
; CHECK: [[PTR:%.*]] = alloca <4 x i32>

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 8, i32 0)
; CHECK: [[MATRIX:%.*]] = load <4 x i32>, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_local(ptr %src, i32 8, i32 0)

; Matrix store sequence:
; CHECK: store <4 x i32> [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_PackedA_RowMajor_SG16_8x8_i32_4_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_local(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) %1, i32 8, i32 0)

; CHECK: ret void

  ret void
}

; CHECK-LABEL: define void @load_store_large_local(
define void @load_store_large_local(ptr %src, ptr %dst) {

; Allocas:
; CHECK: [[TMP4:%.*]] = alloca { <64 x float>, <64 x float> }
; CHECK: [[PTR:%.*]] = alloca { <64 x float>, <64 x float> }

; Matrix load sequence:
; CHECK: call void @__builtin_spriv_OpJointMatrixLoadINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_v8i8_pi32_i32(ptr [[PTR]], ptr %src, i64 16, i32 0)
; CHECK: [[MATRIX:%.*]] = load { <64 x float>, <64 x float> }, ptr [[PTR]]

  %1 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_local(ptr %src, i64 16, i32 0)

; Matrix store sequence:
; CHECK: store { <64 x float>, <64 x float> } [[MATRIX]], ptr [[TMP4]]
; CHECK: call void @__builtin_spriv_OpJointMatrixStoreINTEL_Accumulator_RowMajor_SG16_32x64_i32_128_generic_pi64_v8i8(ptr %dst, ptr [[TMP4]], i64 8, i32 0)

  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_local(ptr %dst, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) %1, i64 8, i32 0)

; CHECK: ret void

  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_generic(ptr, i32, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_global(ptr, i32, i32)
declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0) @__builtin_spirv_OpJointMatrixLoadINTEL_local(ptr, i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_generic(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0), i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_global(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0), i32, i32)
declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL.8x8_local(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 8, 8, 0), i32, i32)

declare target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_generic(ptr, i64, i32)
declare target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_global(ptr, i64, i32)
declare target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2) @__builtin_spirv_OpJointMatrixLoadINTELacc_32x64_f32_p1i8_i64_i32_local(ptr, i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_generic(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_global(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i64, i32)
declare void @__builtin_spirv_OpJointMatrixStoreINTELacc_32x64_f32_p1i8_acc_32x64_f32_i64_i32_local(ptr, target("spirv.CooperativeMatrixKHR", float, 3, 32, 64, 2), i64, i32)

!igc.functions = !{!0, !4, !5}
!0 = !{ptr @test_generic, !1}
!4 = !{ptr @test_global, !1}
!5 = !{ptr @test_local, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
