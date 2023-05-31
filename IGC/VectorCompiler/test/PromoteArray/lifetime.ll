;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"

%stype = type { <16 x i32>, <16 x i32> }

declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture)
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture)

define dllexport spir_kernel void @test(i32 %x, i64 %impl.arg.private.base) {
entry:
  ; CHECK: [[ALLOCA:%[^ ]+]] = alloca <32 x i32>
  %stype.i = alloca %stype, align 64
  %stype1.split = getelementptr %stype, %stype* %stype.i, i64 0, i32 0
  %stype2.split = getelementptr %stype, %stype* %stype.i, i64 0, i32 1
  %lifetime = bitcast %stype* %stype.i to i8*
  %cond = icmp ne i32 %x, 0
  br i1 %cond, label %loop, label %exit

loop:
  %x.curr = phi i32 [ %x, %entry ], [ %x.next, %loop ]

  ; CHECK: [[SCAST:%[^ ]+]] = bitcast <32 x i32>* [[ALLOCA]] to i8*
  ; CHECK: call void @llvm.lifetime.start.p0i8(i64 128, i8* [[SCAST]])
  ; CHECK: store <32 x i32> undef, <32 x i32>* [[ALLOCA]]
  call void @llvm.lifetime.start.p0i8(i64 128, i8* nonnull %lifetime)
  store <16 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <16 x i32>* %stype1.split, align 64
  store <16 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <16 x i32>* %stype2.split, align 64
  call void @llvm.lifetime.end.p0i8(i64 128, i8* nonnull %lifetime)
  ; CHECK: [[ECAST:%[^ ]+]] = bitcast <32 x i32>* [[ALLOCA]] to i8*
  ; CHECK: call void @llvm.lifetime.end.p0i8(i64 128, i8* [[ECAST]])

  %x.next = sub i32 %x.curr, 1
  %cond.loop = icmp ne i32 %x, 0
  br i1 %cond.loop, label %loop, label %exit

exit:
  ret void
}
