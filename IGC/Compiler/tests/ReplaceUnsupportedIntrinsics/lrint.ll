;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

; CHECK-LABEL: define i32 @test_lrint(float %arg)
define i32 @test_lrint(float %arg) {
; CHECK:  [[FPTOSI_LRINT:%[a-zA-Z0-9.]+]] = fptosi float %arg to i32
; CHECK:  ret i32 [[FPTOSI_LRINT]]
  %1 = call i32 @llvm.lrint.i32.f32(float %arg)
  ret i32 %1
}

; CHECK-LABEL: define i64 @test_llrint(double %arg)
define i64 @test_llrint(double %arg) {
; CHECK:  [[FPTOSI_LLRINT:%[a-zA-Z0-9.]+]] = fptosi double %arg to i64
; CHECK:  ret i64 [[FPTOSI_LLRINT]]
  %1 = call i64 @llvm.llrint.i64.f64(double %arg)
  ret i64 %1
}

declare i32 @llvm.lrint.i32.f32(float)
declare i64 @llvm.llrint.i64.f64(double)
