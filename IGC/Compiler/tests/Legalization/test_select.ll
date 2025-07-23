;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -verify -S %s -o - | FileCheck %s

; ------------------------------------------------
; Legalization: handle optimized and/or sequence
; ------------------------------------------------

define i1 @test_select(i1 %src1, i1 %src2, i1 %cc) {
; CHECK-LABEL: define i1 @test_select(
; CHECK-SAME: i1 [[SRC1:%.*]], i1 [[SRC2:%.*]], i1 [[CC:%.*]]) {
; CHECK: [[XOR1:%.*]] = xor i1 [[SRC1]], true
; CHECK-NEXT: [[XOR2:%.*]] = xor i1 [[SRC2]], true
; CHECK-NEXT: [[AND1:%.*]] = and i1 [[XOR1]], [[XOR2]]
; CHECK-NEXT: [[ZEXT1:%.*]] = zext i1 [[AND1]] to i32
; CHECK-NEXT: [[ZEXT2:%.*]] = zext i1 [[CC]] to i32
; CHECK-NEXT: [[SELECT1:%.*]] = select i1 [[AND1]], i32 [[ZEXT1]], i32 [[ZEXT2]]
; CHECK-NEXT: [[TRUNC1:%.*]] = trunc i32 [[SELECT1]] to i1
; CHECK-NEXT: ret i1 [[TRUNC1]]
;
  %1 = xor i1 %src1, true
  %2 = xor i1 %src2, true
  %3 = and i1 %1, %2
  %4 = select i1 %3, i1 %3, i1 %cc
  ret i1 %4
}

!igc.functions = !{!0}

!0 = !{i1 (i1, i1, i1)* @test_select, !1}
!1 = !{}