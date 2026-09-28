;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s


define spir_kernel void @test(ptr %0) {
entry:
  ; CHECK: [[TMP1:%.*]] = ptrtoint ptr %0 to i64
  ; CHECK-NEXT: [[TMP2:%.*]] = and i64 [[TMP1]], 7
  ; CHECK-NOT: zext i3 [[TMP1]] to i64
  %1 = ptrtoint ptr %0 to i3
  %2 = zext i3 %1 to i64
  ret void
}