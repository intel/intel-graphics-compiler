;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; COM: Sanity checker: nothing should be changed, compilation should not fail

define dllexport void @test(i64 %val) "CMGenxMain" {
  br label %first
first:
  %cond = icmp ult i64 %val, 0
  br i1 %cond, label %left, label %right
right:
  br label %left
left:
  %cond2 = icmp ult i64 %val, 1234
  br i1 %cond2, label %right, label %exit
exit:
  ret void

; CHECK-LABEL: test
; CHECK-NEXT: br label %first
; CHECK-LABEL: first:
; CHECK-NEXT: %cond = icmp ult i64 %val, 0
; CHECK-NEXT: br i1 %cond, label %left, label %right
; CHECK-LABEL: right:
; CHECK-NEXT: br label %left
; CHECK-LABEL: left:
; CHECK-NEXT: %cond2 = icmp ult i64 %val, 1234
; CHECK-NEXT: br i1 %cond2, label %right, label %exit
; CHECK-LABEL: exit:
; CHECK-NEXT: ret void
}
