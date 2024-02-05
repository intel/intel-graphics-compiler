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
; CHECK: %cmp = icmp eq i32 %in, 5
; CHECK: %ze.l = zext i1 %cmp to i8
; CHECK: %sel.l = select i1 %cmp, i8 2, i8 %ze.l
define spir_func void @foo(i32 %in) {
entry:
  %cmp = icmp eq i32 %in, 5
  %ze = zext i1 %cmp to i2
  %sel = select i1 %cmp, i2 -2, i2 %ze
  ret void
}
