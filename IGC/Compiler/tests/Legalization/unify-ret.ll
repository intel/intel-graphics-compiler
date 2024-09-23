;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -opaque-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: ret
; ------------------------------------------------

; Checks unificataion of multiple ret
; to one unified bb


define void @test_ret_void(i1 %cc, ptr %p1, ptr %p2) {
; CHECK-LABEL: define void @test_ret_void(
; CHECK-SAME: i1 [[CC:%.*]], ptr [[P1:%.*]], ptr [[P2:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    br i1 [[CC]], label %[[B1:.*]], label %[[B2:.*]]
; CHECK:       [[B1]]:
; CHECK-NEXT:    store i32 13, ptr [[P1]], align 4
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK:.*]]
; CHECK:       [[B2]]:
; CHECK-NEXT:    store i32 144, ptr [[P2]], align 4
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK]]
; CHECK:       [[UNIFIEDRETURNBLOCK]]:
; CHECK-NEXT:    ret void
;
entry:
  br i1 %cc, label %b1, label %b2

b1:                                               ; preds = %entry
  store i32 13, ptr %p1, align 4
  ret void

b2:                                               ; preds = %entry
  store i32 144, ptr %p2, align 4
  ret void
}

define i32 @test_ret_i32(i1 %cc, i32 %s1) {
; CHECK-LABEL: define i32 @test_ret_i32(
; CHECK-SAME: i1 [[CC:%.*]], i32 [[S1:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    br i1 [[CC]], label %[[B1:.*]], label %[[B2:.*]]
; CHECK:       [[B1]]:
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK:.*]]
; CHECK:       [[B2]]:
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK]]
; CHECK:       [[UNIFIEDRETURNBLOCK]]:
; CHECK-NEXT:    [[UNIFIEDRETVAL:%.*]] = phi i32 [ [[S1]], %[[B1]] ], [ 144, %[[B2]] ]
; CHECK-NEXT:    ret i32 [[UNIFIEDRETVAL]]
;
entry:
  br i1 %cc, label %b1, label %b2

b1:                                               ; preds = %entry
  ret i32 %s1

b2:                                               ; preds = %entry
  ret i32 144
}

!igc.functions = !{!0, !1}

!0 = !{ptr @test_ret_void, !1}
!1 = distinct !{ptr @test_ret_i32, !1}

