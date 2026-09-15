;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --igc-predload-reuse-max-candidates=1 --verify -S %s | FileCheck %s

; Each address keeps only the newest candidates. With a budget of one,
; @newest_kept still reuses the load recorded last, while in @oldest_dropped the
; only matching candidate has already been evicted and both loads survive.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @newest_kept(
; CHECK-NEXT:    %old = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 0)
; CHECK-NEXT:    %new = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 0)
; CHECK-NEXT:    %mask = and i1 %q2, %q1
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %mask, i32 %new, i32 0
; CHECK-NEXT:    %r1 = add i32 %old, %new
; CHECK-NEXT:    %r2 = add i32 %r1, %[[V1]]
; CHECK-NEXT:    ret i32 %r2

define i32 @newest_kept(ptr addrspace(1) %p, i1 noundef %q1, i1 noundef %q2) {
  %old = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 0)
  %new = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 0)
  %mask = and i1 %q2, %q1
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0)
  %r1 = add i32 %old, %new
  %r2 = add i32 %r1, %b
  ret i32 %r2
}

; CHECK-LABEL: define i32 @oldest_dropped(
; CHECK-NEXT:    %old = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 0)
; CHECK-NEXT:    %new = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 0)
; CHECK-NEXT:    %mask = and i1 %q1, %z
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0)
; CHECK-NEXT:    %r1 = add i32 %old, %new
; CHECK-NEXT:    %r2 = add i32 %r1, %b
; CHECK-NEXT:    ret i32 %r2

define i32 @oldest_dropped(ptr addrspace(1) %p, i1 noundef %q1, i1 noundef %q2, i1 noundef %z) {
  %old = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 0)
  %new = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 0)
  %mask = and i1 %q1, %z
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0)
  %r1 = add i32 %old, %new
  %r2 = add i32 %r1, %b
  ret i32 %r2
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
