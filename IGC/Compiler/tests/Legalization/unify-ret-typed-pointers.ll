;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: ret
; ------------------------------------------------

; Checks unificataion of multiple ret
; to one unified bb

define void @test_ret_void(i1 %cc, i32* %p1, i32* %p2) {
; CHECK-LABEL: define void @test_ret_void(
; CHECK-SAME: i1 [[CC:%.*]], i32* [[P1:%.*]], i32* [[P2:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    br i1 [[CC]], label %[[B1:.*]], label %[[B2:.*]]
; CHECK:       [[B1]]:
; CHECK-NEXT:    store i32 13, i32* [[P1]]
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK:.*]]
; CHECK:       [[B2]]:
; CHECK-NEXT:    store i32 144, i32* [[P2]]
; CHECK-NEXT:    br label %[[UNIFIEDRETURNBLOCK]]
; CHECK:       [[UNIFIEDRETURNBLOCK]]:
; CHECK-NEXT:    ret void
;
entry:
  br i1 %cc, label %b1, label %b2
b1:
  store i32 13, i32* %p1
  ret void
b2:
  store i32 144, i32* %p2
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
b1:
  ret i32 %s1
b2:
  ret i32 144
}

!igc.functions = !{!0, !1}

!0 = !{void (i1, i32*,i32*)* @test_ret_void, !1}
!1 = !{i32 (i1, i32)* @test_ret_i32, !1}
!2 = !{}
