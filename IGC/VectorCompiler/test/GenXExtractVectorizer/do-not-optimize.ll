;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXExtractVectorizer -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32)

; CHECK-COUNT-5: add i32

define dllexport void @kernel(<8 x i32> %in, i32 %cond) {
entry:
  switch i32 %cond, label %bb0 [
  i32 1, label %bb1
  i32 2, label %bb2
  i32 3, label %bb3
  i32 4, label %bb4
  i32 5, label %bb5
  ]

bb0:
  %rd0 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %in, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %add0 = add i32 %rd0, 10
  br label %exit

bb1:
  %rd1 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %in, i32 0, i32 1, i32 1, i16 4, i32 undef)
  %add1 = add i32 %rd1, 11
  br label %exit

bb2:
  %rd2 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %in, i32 0, i32 1, i32 1, i16 8, i32 undef)
  %add2 = add i32 %rd2, 12
  br label %exit

bb3:
  %rd3 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %in, i32 0, i32 1, i32 1, i16 12, i32 undef)
  %add3 = add i32 %rd3, 13
  br label %exit

bb4:
  %rd4 = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %in, i32 0, i32 1, i32 1, i16 16, i32 undef)
  %add4 = add i32 %rd4, 14
  br label %exit

bb5:
  br label %exit

exit:
  %add.res = phi i32 [%add0, %bb0], [%add1, %bb1], [%add2, %bb2], [%add3, %bb3], [%add4, %bb4], [15, %bb5]
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (<8 x i32>, i32)* @kernel}
