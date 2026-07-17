;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -igc-unreachable-handling -S < %s 2>&1 | FileCheck %s
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

  br i1 %src, label %a, label %b
a:
  unreachable
b:
  ret void
}

; Instructions after unreachable live in a block with no entry-reachable
; predecessor, so it is dropped and the function collapses to a single ret.

define void @test_unreachable_inst_after(i1 %src) {
; CHECK-LABEL: @test_unreachable_inst_after(
; CHECK-NEXT:    ret void
; CHECK-NOT:     add

  unreachable
  %2 = add i1 %src, %src
  ret void
}
