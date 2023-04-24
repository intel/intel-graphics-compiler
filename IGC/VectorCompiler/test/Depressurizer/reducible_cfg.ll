;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -GenXDepressurizerWrapper -march=genx64 -mcpu=Gen9 -S -mtriple=spir64-unknown-unknown < %s | FileCheck %s

; COM: Sanity checker: nothing should be changed, compilation should not fail.

define dllexport void @test(i64 %val) "CMGenxMain" {
  br label %outer_loop_header
outer_loop_header:
  %cond = icmp ult i64 %val, 0
  br i1 %cond, label %outer_loop_exit, label %inner_loop_header.crit_edge
inner_loop_header.crit_edge:
  br label %inner_loop_header
inner_loop_header:
  %cond2 = icmp ult i64 %val, 1234
  br i1 %cond2, label %inner_loop_exit.crit_edge, label %inner_loop_body
inner_loop_body:
  br label %inner_loop_header
inner_loop_exit.crit_edge:
  br label %inner_loop_exit
inner_loop_exit:
  br label %outer_loop_header
outer_loop_exit:
  ret void

; CHECK-LABEL: test
; CHECK-NEXT: br label %outer_loop_header
; CHECK-LABEL: outer_loop_header:
; CHECK-NEXT: %cond = icmp ult i64 %val, 0
; CHECK-NEXT: br i1 %cond, label %outer_loop_exit, label %inner_loop_header.crit_edge
; CHECK-LABEL: inner_loop_header.crit_edge:
; CHECK-NEXT: br label %inner_loop_header
; CHECK-LABEL: inner_loop_header:
; CHECK-NEXT: %cond2 = icmp ult i64 %val, 1234
; CHECK-NEXT: br i1 %cond2, label %inner_loop_exit.crit_edge, label %inner_loop_body
; CHECK-LABEL: inner_loop_body:
; CHECK-NEXT: br label %inner_loop_header
; CHECK-LABEL: inner_loop_exit.crit_edge:
; CHECK-NEXT: br label %inner_loop_exit
; CHECK-LABEL: inner_loop_exit:
; CHECK-NEXT: br label %outer_loop_header
; CHECK-LABEL: outer_loop_exit:
; CHECK-NEXT: ret void
}
