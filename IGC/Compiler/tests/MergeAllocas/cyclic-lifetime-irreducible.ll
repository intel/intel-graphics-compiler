;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -S --igc-ocl-merge-allocas %s | FileCheck %s

; The cycle %A -> %B -> %A has two entries, so LoopInfo does not report it as a loop at all. An
; allocation escaping in %A still makes the live range cyclic, which used to hit the
; "No lifetime start block?" assertion.

define void @irreducible_cycle(ptr addrspace(1) %out, i1 %c, i32 %m) nounwind {
; CHECK-LABEL: @irreducible_cycle(
; CHECK: alloca [4 x i32]
entry:
  %buf = alloca [4 x i32]
  br i1 %c, label %B, label %A

A:
  %i.a = phi i32 [ 0, %entry ], [ %inc, %B ]
  %gep = getelementptr inbounds [4 x i32], ptr %buf, i32 0, i32 %i.a
  %p2i = ptrtoint ptr %gep to i32
  store i32 %p2i, ptr addrspace(1) %out
  br label %B

B:
  %i.b = phi i32 [ 0, %entry ], [ %i.a, %A ]
  %inc = add i32 %i.b, 1
  %cmp = icmp slt i32 %inc, %m
  br i1 %cmp, label %A, label %exit

exit:
  ret void
}

; %a dies before the cycle is entered - merging is still allowed.
define void @dead_before_irreducible_cycle(ptr addrspace(1) %out, i1 %c, i32 %m) nounwind {
; CHECK-LABEL: @dead_before_irreducible_cycle(
; CHECK: alloca [4 x i32]
; CHECK-NOT: alloca [4 x i32]
entry:
  %a = alloca [4 x i32]
  %b = alloca [4 x i32]
  %gepa = getelementptr inbounds [4 x i32], ptr %a, i32 0, i32 0
  store i32 7, ptr %gepa
  %loada = load i32, ptr %gepa
  store i32 %loada, ptr addrspace(1) %out
  br i1 %c, label %B, label %A

A:
  %i.a = phi i32 [ 0, %entry ], [ %inc, %B ]
  %gepb = getelementptr inbounds [4 x i32], ptr %b, i32 0, i32 %i.a
  %p2ib = ptrtoint ptr %gepb to i32
  store i32 %p2ib, ptr addrspace(1) %out
  br label %B

B:
  %i.b = phi i32 [ 0, %entry ], [ %i.a, %A ]
  %inc = add i32 %i.b, 1
  %cmp = icmp slt i32 %inc, %m
  br i1 %cmp, label %A, label %exit

exit:
  ret void
}
