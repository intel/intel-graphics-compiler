;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; Mask must come after all PHIs.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @phi_insertion(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label %loop
; CHECK-EMPTY:
; CHECK-NEXT:  loop:
; CHECK-NEXT:    %q = phi i1 [ %q0, %entry ], [ %q0, %loop ]
; CHECK-NEXT:    %cond = phi i1 [ %cond0, %entry ], [ %cond0, %loop ]
; CHECK-NEXT:    %fallback = phi i32 [ 0, %entry ], [ %r, %loop ]
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 %fallback
; CHECK-NEXT:    br i1 %again, label %loop, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    ret i32 %r

define i32 @phi_insertion(ptr addrspace(1) %p, i1 %q0, i1 noundef %cond0, i1 %again) {
entry:
  br label %loop

loop:
  %q = phi i1 [ %q0, %entry ], [ %q0, %loop ]
  %cond = phi i1 [ %cond0, %entry ], [ %cond0, %loop ]
  %fallback = phi i32 [ 0, %entry ], [ %r, %loop ]
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %r = select i1 %cond, i32 %a, i32 %fallback
  br i1 %again, label %loop, label %exit

exit:
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
