;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXExtractVectorizer -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)

; CHECK: entry:
; CHECK: %[[add:.*]] = add <4 x i32> %in, <i32 10, i32 11, i32 12, i32 13>
; CHECK-NEXT: %[[rdr1:.*]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %[[add]], i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: %[[rdr2:.*]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %[[add]], i32 0, i32 1, i32 1, i16 4, i32 undef)
; CHECK-NEXT: %[[rdr3:.*]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %[[add]], i32 0, i32 1, i32 1, i16 8, i32 undef)
; CHECK-NEXT: %[[rdr4:.*]] = call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %[[add]], i32 0, i32 1, i32 1, i16 12, i32 undef)

; CHECK: exit:
; CHECK: phi i32 [ %[[rdr1]], %if.then.then ], [ %[[rdr2]], %if.then.else ], [ %[[rdr3]], %if.else.then ], [ %[[rdr4]], %if.else.else ]

define dllexport void @kernel(<4 x i32> %in, i1 %cond1, i1 %cond2) {
entry:
  br i1 %cond1, label %if.then, label %if.else

if.then:
  br i1 %cond2, label %if.then.then, label %if.then.else

if.then.then:
  %rd1 = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %in, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %add1 = add i32 %rd1, 10
  br label %exit

if.then.else:
  %rd2 = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %in, i32 0, i32 1, i32 1, i16 4, i32 undef)
  %add2 = add i32 %rd2, 11
  br label %exit

if.else:
  br i1 %cond2, label %if.else.then, label %if.else.else

if.else.then:
  %rd3 = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %in, i32 0, i32 1, i32 1, i16 8, i32 undef)
  %add3 = add i32 %rd3, 12
  br label %exit

if.else.else:
  %rd4 = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %in, i32 0, i32 1, i32 1, i16 12, i32 undef)
  %add4 = add i32 %rd4, 13
  br label %exit

exit:
  %add.res = phi i32 [%add1, %if.then.then], [%add2, %if.then.else], [%add3, %if.else.then], [%add4, %if.else.else]
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (<4 x i32>, i1, i1)* @kernel}
