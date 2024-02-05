;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTypeLegalization -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

; CHECK-LABEL: foo
; CHECK: %ze.l = zext i32 %in to i64
; CHECK: %op.l = mul i64 %ze.l, %ze.l
; CHECK: %sh.l = lshr i64 %ze.l, 1
; CHECK: %tr = trunc i64 %sh.l to i32
; CHECK: ret i32 %tr
define spir_func i32 @foo(i32 %in) {
entry:
  %ze = zext i32 %in to i33
  %op = mul i33 %ze, %ze
  %sh = lshr i33 %ze, 1
  %tr = trunc i33 %sh to i32
  ret i32 %tr
}
