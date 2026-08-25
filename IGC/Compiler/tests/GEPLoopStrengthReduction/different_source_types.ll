;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-gep-loop-strength-reduction -S < %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64"

%struct.S = type { [4 x i64] }   ; 32 bytes, align 8

define spir_kernel void @repro(ptr addrspace(1) %p, i32 %n) {
entry:
; CHECK-LABEL: entry
; CHECK: [[TMP0:%.*]] = getelementptr %struct.S, ptr addrspace(1) %p, i64 1
; CHECK: [[TMP1:%.*]] = getelementptr i8, ptr addrspace(1) %p, i64 64
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  ; CHECK-LABEL: loop
  ; CHECK-NOT: [[TMP2:%.*]] = getelementptr %struct.S, ptr addrspace(1) [[TMP0]], i64 63
  %a = getelementptr %struct.S, ptr addrspace(1) %p, i64 1
  %b = getelementptr i8, ptr addrspace(1) %p, i64 64

  %va = load i64, ptr addrspace(1) %a, align 8
  %vb = load i64, ptr addrspace(1) %b, align 8

  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %loop, label %exit
exit:
  ret void
}
