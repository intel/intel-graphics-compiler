;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers --regkey CodeSinkingMinSize=0 -igc-code-sinking -S < %s | FileCheck %s

define spir_kernel void @repro(i32 %x, i32 %sel, ptr %out) {
entry:
  %v = add i32 %x, 1
  %c = icmp eq i32 %sel, 0
  br i1 %c, label %left, label %right

left:
  br label %merge

right:
  br label %merge

; %dead has no predecessors -> unreachable -> not present in the DominatorTree,
; yet it is a predecessor of %merge and appears in the PHI below.
dead:
  br label %merge

merge:
  %p = phi i32 [ %v, %left ], [ %v, %right ], [ %v, %dead ]
  store i32 %p, ptr %out
  ret void
}

; CHECK-LABEL: @repro(
; CHECK: phi i32 {{.*}}[ %{{.*}}, %dead ]
; CHECK: ret void
