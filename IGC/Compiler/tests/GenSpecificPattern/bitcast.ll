;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-gen-specific-pattern -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: bitcast pattern
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_bitcast(i32 %src1, i32 %src2) {
; CHECK-LABEL: @test_bitcast(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = insertelement <2 x i32> undef, i32 [[SRC2:%.*]], i32 0
; CHECK:    [[TMP1:%.*]] = insertelement <2 x i32> [[TMP0]], i32 [[SRC1:%.*]], i32 1
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i32> [[TMP1]] to double
; CHECK:    call void @use.f64(double [[TMP2]])
; CHECK:    ret void
;
entry:
  %0 = insertelement <2 x i32> <i32 0, i32 undef>, i32 %src1, i32 1
  %1 = bitcast <2 x i32> %0 to i64
  %2 = zext i32 %src2 to i64
  %3 = or i64 %1, %2
  %4 = bitcast i64 %3 to double
  call void @use.f64(double %4)
  ret void
}

declare void @use.f64(double)
