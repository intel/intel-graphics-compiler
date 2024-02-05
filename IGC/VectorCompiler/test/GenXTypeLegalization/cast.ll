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
; CHECK: [[TRUNC_FOO:[^ ]+]] = trunc i64 %in to i16
; CHECK: %tr.l = and i16 [[TRUNC_FOO]], 511
; CHECK: %ze.l = zext i16 %tr.l to i32
define spir_func void @foo(i64 %in) {
entry:
  %tr = trunc i64 %in to i9
  %ze = zext i9 %tr to i31
  ret void
}

; CHECK-LABEL: bar
; CHECK-NOT: zext
define spir_func void @bar(i64 %in) {
entry:
  %tr = trunc i64 %in to i9
  %ze = zext i9 %tr to i11
  ret void
}
