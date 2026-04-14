;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: bmg-supported, opaque-pointers, llvm-14-plus

; RUN: llvm-as < %s -o %t.bc %OPAQUE_PTR_FLAG%
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg | FileCheck %s

; Verifies that when recompiling kernel 'entry', we don't try to erase 'erased' function, leading to a crash.

; CHECK: Start recompilation of the kernel
; CHECK-NEXT: in kernel: 'entry'
; CHECK-NOT: Use still stuck around after Def is destroyed: call spir_func void @erased()

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @erased() {
entry:
  ret void
}

define spir_kernel void @entry(ptr %seed) {
entry:
  call spir_func void @erased()
  call spir_func void @do_hash(ptr %seed)
  ret void
}

define internal spir_func void @do_hash(ptr %in) {
entry:
  call spir_func void @hash(ptr %in, i32 136)
  ret void
}

define internal spir_func void @hash(ptr %in, i32 %rate) {
entry:
  %a = alloca [200 x i8], align 1
  br label %while.cond

while.cond:                                       ; preds = %while.cond, %entry
  call spir_func void @xor(ptr %a, ptr %in, i32 %rate)
  call spir_func void @foo(ptr %a)
  br label %while.cond
}

define internal spir_func void @xor(ptr %dst, ptr %src, i32 %len) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %add, %for.body ]
  %cmp = icmp ult i32 %i.0, %len
  br i1 %cmp, label %for.body, label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond
  ret void

for.body:                                         ; preds = %for.cond
  %idxprom = zext i32 %i.0 to i64
  %arrayidx = getelementptr i8, ptr %src, i64 %idxprom
  %0 = load i8, ptr %arrayidx, align 1
  %arrayidx2 = getelementptr i8, ptr %dst, i64 %idxprom
  store i8 %0, ptr %arrayidx2, align 1
  %add = add i32 %i.0, 1
  br label %for.cond
}

declare spir_func void @foo(ptr)
