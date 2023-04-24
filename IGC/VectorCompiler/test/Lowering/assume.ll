;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-NOT: call void @llvm.assume

declare void @llvm.assume(i1)

define void @assume(i64 %i.ext, i64 %a)  {
  %cmp0 = icmp ne i64 %i.ext, %a
  call void @llvm.assume(i1 %cmp0)
  ret void
}
