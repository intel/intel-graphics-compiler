;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-scalarize -S < %s | FileCheck %s
; ------------------------------------------------
; ScalarizeFunction
; ------------------------------------------------

declare <4 x i64> @llvm.fshl.v4i64(<4 x i64>, <4 x i64>, <4 x i64>)
declare <2 x i32> @llvm.fshl.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)

define spir_kernel void @test_basic_v4i64(<4 x i64> %a, <4 x i64> %b, <4 x i64> %c) {
; CHECK-LABEL: @test_basic_v4i64(
; CHECK: call i64 @llvm.fshl.i64(i64 %{{.*}}, i64 %{{.*}}, i64 %{{.*}})
; CHECK: call i64 @llvm.fshl.i64(i64 %{{.*}}, i64 %{{.*}}, i64 %{{.*}})
; CHECK: call i64 @llvm.fshl.i64(i64 %{{.*}}, i64 %{{.*}}, i64 %{{.*}})
; CHECK: call i64 @llvm.fshl.i64(i64 %{{.*}}, i64 %{{.*}}, i64 %{{.*}})
  %res = call <4 x i64> @llvm.fshl.v4i64(<4 x i64> %a, <4 x i64> %b, <4 x i64> %c)
  ret void
}

define spir_kernel void @test_basic_v2i32(<2 x i32> %a, <2 x i32> %b, <2 x i32> %c) {
; CHECK-LABEL: @test_basic_v2i32(
; CHECK: call i32 @llvm.fshl.i32(i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}})
; CHECK: call i32 @llvm.fshl.i32(i32 %{{.*}}, i32 %{{.*}}, i32 %{{.*}})
  %res = call <2 x i32> @llvm.fshl.v2i32(<2 x i32> %a, <2 x i32> %b, <2 x i32> %c)
  ret void
}