;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

%struct.a = type { [5 x [16 x double]] }
define dllexport spir_kernel void @test_lowergep([5 x %struct.a]* %a, i64 %b, i64 %c) {
  %1 = getelementptr [5 x %struct.a], [5 x %struct.a]* %a, i64 0, i64 %b, i32 0, i64 %c, i64 0
; CHECK: [[V1:%.*]] = ptrtoint [5 x %struct.a]* %a to i64
; CHECK-NEXT: [[V2:%.*]] = mul i64 %b, 640
; CHECK-NEXT: [[V3:%.*]] = add i64 [[V1]], %2
; CHECK-NEXT: [[V4:%.*]] = shl i64 %c, 7
; CHECK-NEXT: [[V5:%.*]] = add i64 [[V3]], %4
; CHECK-NEXT: [[V6:%.*]] = inttoptr i64 [[V5]] to double*
  ret void
}
