;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: select i1
; ------------------------------------------------

; Checks legalization of select with i1 type:
;
; %sel = select i1 %cc, i1 %a, i1 %b
;
; into
;
; %a.ext = zext i1 %a to i32
; %b.ext = zext i1 %b to i32
; %sel.ext = select i1 %cc, i32 %a.ext, i32 %b.ext
; %sel =  trunc i32 %sel.ext to i1
;

define i1 @test_select(i1 %cc, i1 %src1, i1 %src2) {
; CHECK-LABEL: define i1 @test_select(
; CHECK-SAME: i1 [[CC:%.*]], i1 [[SRC1:%.*]], i1 [[SRC2:%.*]]) {
; CHECK:    [[TMP1:%.*]] = zext i1 [[SRC1]] to i32
; CHECK:    [[TMP2:%.*]] = zext i1 [[SRC2]] to i32
; CHECK:    [[TMP3:%.*]] = select i1 [[CC]], i32 [[TMP1]], i32 [[TMP2]]
; CHECK:    [[TMP4:%.*]] = trunc i32 [[TMP3]] to i1
; CHECK:    ret i1 [[TMP4]]
;
  %1 = select i1 %cc, i1 %src1, i1 %src2
  ret i1 %1
}

!igc.functions = !{!0}

!0 = !{i1 (i1, i1, i1)* @test_select, !1}
!1 = !{}
