;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -loop-unroll-legacy-wrapped -instsimplify -S < %s | FileCheck %s

; The legacy AssumptionCache, created for ScalarEvolution before the wrapper
; runs, keeps a pointer to the TTI of TargetTransformInfoWrapperPass. The
; wrapper must leave that TTI usable: InstSimplify scans the assumptions
; through it without requesting the TTI again.

; CHECK-LABEL: define void @test(
; CHECK: call void @llvm.assume(i1 %c)
; CHECK-NEXT: store i1 false, ptr addrspace(1) %out

define void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %loop, label %exit

exit:
  %x = load i32, ptr addrspace(1) %in
  %c = icmp ne i32 %x, 0
  call void @llvm.assume(i1 %c)
  %z = icmp eq i32 %x, 0
  store i1 %z, ptr addrspace(1) %out
  ret void
}

declare void @llvm.assume(i1)
