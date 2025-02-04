;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------

declare <16 x i32> @llvm.smin.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.smax.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.umin.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.umax.v16i32(<16 x i32>, <16 x i32>)
declare <8 x i32> @llvm.smin.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.smax.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.umin.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.umax.v8i32(<8 x i32>, <8 x i32>)
declare <4 x i32> @llvm.smin.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.smax.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.umin.v4i32(<4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.umax.v4i32(<4 x i32>, <4 x i32>)
declare <3 x i32> @llvm.smin.v3i32(<3 x i32>, <3 x i32>)
declare <3 x i32> @llvm.smax.v3i32(<3 x i32>, <3 x i32>)
declare <3 x i32> @llvm.umin.v3i32(<3 x i32>, <3 x i32>)
declare <3 x i32> @llvm.umax.v3i32(<3 x i32>, <3 x i32>)
declare <2 x i32> @llvm.smin.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.smax.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.umin.v2i32(<2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.umax.v2i32(<2 x i32>, <2 x i32>)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.smax.i32(i32, i32)
declare i32 @llvm.umin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)

define spir_kernel void @test_basic_smin_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_basic_smin_i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call i32 @llvm.smin.i32(i32 %a, i32 %b)
  ret void
}

define spir_kernel void @test_basic_smin_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: @test_basic_smin_v2i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <2 x i32> @llvm.smin.v2i32(<2 x i32> %a, <2 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smin_v3i32(<3 x i32> %a, <3 x i32> %b) {
; CHECK-LABEL: @test_basic_smin_v3i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <3 x i32> @llvm.smin.v3i32(<3 x i32> %a, <3 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smin_v4i32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: @test_basic_smin_v4i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <4 x i32> @llvm.smin.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smin_v8i32(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: @test_basic_smin_v8i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <8 x i32> @llvm.smin.v8i32(<8 x i32> %a, <8 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smin_v16i32(<16 x i32> %a, <16 x i32> %b) {
; CHECK-LABEL: @test_basic_smin_v16i32(
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <16 x i32> @llvm.smin.v16i32(<16 x i32> %a, <16 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smax_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_basic_smax_i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret void
}

define spir_kernel void @test_basic_smax_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: @test_basic_smax_v2i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <2 x i32> @llvm.smax.v2i32(<2 x i32> %a, <2 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smax_v3i32(<3 x i32> %a, <3 x i32> %b) {
; CHECK-LABEL: @test_basic_smax_v3i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <3 x i32> @llvm.smax.v3i32(<3 x i32> %a, <3 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smax_v4i32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: @test_basic_smax_v4i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <4 x i32> @llvm.smax.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smax_v8i32(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: @test_basic_smax_v8i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <8 x i32> @llvm.smax.v8i32(<8 x i32> %a, <8 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_smax_v16i32(<16 x i32> %a, <16 x i32> %b) {
; CHECK-LABEL: @test_basic_smax_v16i32(
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.smax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <16 x i32> @llvm.smax.v16i32(<16 x i32> %a, <16 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umin_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_basic_umin_i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call i32 @llvm.umin.i32(i32 %a, i32 %b)
  ret void
}

define spir_kernel void @test_basic_umin_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: @test_basic_umin_v2i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <2 x i32> @llvm.umin.v2i32(<2 x i32> %a, <2 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umin_v3i32(<3 x i32> %a, <3 x i32> %b) {
; CHECK-LABEL: @test_basic_umin_v3i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <3 x i32> @llvm.umin.v3i32(<3 x i32> %a, <3 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umin_v4i32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: @test_basic_umin_v4i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <4 x i32> @llvm.umin.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umin_v8i32(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: @test_basic_umin_v8i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <8 x i32> @llvm.umin.v8i32(<8 x i32> %a, <8 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umin_v16i32(<16 x i32> %a, <16 x i32> %b) {
; CHECK-LABEL: @test_basic_umin_v16i32(
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umin.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <16 x i32> @llvm.umin.v16i32(<16 x i32> %a, <16 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umax_i32(i32 %a, i32 %b) {
; CHECK-LABEL: @test_basic_umax_i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call i32 @llvm.umax.i32(i32 %a, i32 %b)
  ret void
}

define spir_kernel void @test_basic_umax_v2i32(<2 x i32> %a, <2 x i32> %b) {
; CHECK-LABEL: @test_basic_umax_v2i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <2 x i32> @llvm.umax.v2i32(<2 x i32> %a, <2 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umax_v4i32(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: @test_basic_umax_v4i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <4 x i32> @llvm.umax.v4i32(<4 x i32> %a, <4 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umax_v3i32(<3 x i32> %a, <3 x i32> %b) {
; CHECK-LABEL: @test_basic_umax_v3i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <3 x i32> @llvm.umax.v3i32(<3 x i32> %a, <3 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umax_v8i32(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: @test_basic_umax_v8i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <8 x i32> @llvm.umax.v8i32(<8 x i32> %a, <8 x i32> %b)
  ret void
}

define spir_kernel void @test_basic_umax_v16i32(<16 x i32> %a, <16 x i32> %b) {
; CHECK-LABEL: @test_basic_umax_v16i32(
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.umax.i32(i32 %{{.*}}, i32 %{{.*}})
  %res = call <16 x i32> @llvm.umax.v16i32(<16 x i32> %a, <16 x i32> %b)
  ret void
}
