;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowerJmpTableSwitch -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK: switch i32
; CHECK-NOT: @llvm.internal.jump.table
; CHECK-NOT: indirectbr i8*

define dllexport spir_kernel void @foo(i32* %0) {
  %2 = load i32, i32* %0
  switch i32 %2, label %bb1 [
      i32 1, label %bb2
      i32 2, label %bb3
      i32 3, label %bb3
  ]
bb1:
  br label %bb4
bb2:
  br label %bb4
bb3:
  br label %bb4
bb4:
  %res = phi i32 [0, %bb1], [1, %bb2], [2, %bb3]
  store i32 %res, i32* %0
  ret void
}
