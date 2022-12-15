;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -igc-unreachable-handling -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; UnreachableHandling
; ------------------------------------------------

; Test checks that unreachable is replaced by ret

; Last instruction in BB case
define void @test_unreachable_end(i1 %src) {
; CHECK-LABEL: @test_unreachable_end(
; CHECK-NEXT:    br i1 [[SRC:%.*]], label [[A:%.*]], label [[B:%.*]]
; CHECK:       a:
; CHECK-NEXT:    ret void
; CHECK:       b:
; CHECK-NEXT:    ret void
;
  br i1 %src, label %a, label %b
a:
  unreachable
b:
  ret void
}

; Some instructions after unreachable are present

define void @test_unreachable_inst_after(i1 %src) {
; CHECK-LABEL: @test_unreachable_inst_after(
; CHECK-NEXT:    ret void
; CHECK:       1:
; CHECK-NEXT:    [[TMP2:%.*]] = add i1 [[SRC:%.*]], [[SRC]]
; CHECK-NEXT:    ret void
;
  unreachable
  %2 = add i1 %src, %src
  ret void
}
