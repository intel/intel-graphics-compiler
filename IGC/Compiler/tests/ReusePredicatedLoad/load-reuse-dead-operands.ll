;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; Reuse deletes the operands it leaves dead - here %fallback, which was the only
; use of the load %i. That cleanup must not remove %i from the candidates of
; the loads that follow: %c and %d reuse it, so it is not dead after all.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @dead_operands(
; CHECK-NEXT:    %i = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %r1 = select i1 %q, i32 %a, i32 0
; CHECK-NEXT:    %r2 = add i32 %i, %i
; CHECK-NEXT:    %r = add i32 %r1, %r2
; CHECK-NEXT:    %r3 = add i32 %r, %a
; CHECK-NEXT:    ret i32 %r3

define i32 @dead_operands(ptr addrspace(1) %p, ptr addrspace(1) %p2, i1 noundef %q) {
  %i = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
  %fallback = add i32 %i, 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 %fallback)
  %r1 = select i1 %q, i32 %b, i32 0
  %c = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
  %d = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
  %r2 = add i32 %c, %d
  %r = add i32 %r1, %r2
  %r3 = add i32 %r, %a
  ret i32 %r3
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
