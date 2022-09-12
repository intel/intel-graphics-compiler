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
; GenSpecificPattern: cmp pattern
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_cmp(i64 %src1) {
; CHECK-LABEL: @test_cmp(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = bitcast i64 [[SRC1:%.*]] to <2 x i32>
; CHECK:    [[TMP1:%.*]] = extractelement <2 x i32> [[TMP0]], i32 1
; CHECK:    [[TMP2:%.*]] = and i32 [[TMP1]], -134744073
; CHECK:    [[TMP3:%.*]] = icmp slt i32 [[TMP2]], -1431651397
; CHECK:    call void @use.i1(i1 [[TMP3]])
; CHECK:    ret void
;
entry:
  %0 = and i64 %src1, -578721386864836608
  %1 = icmp slt i64 %0, -6148895929387712512
  call void @use.i1(i1 %1)
  ret void
}

declare void @use.i1(i1)
