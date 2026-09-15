;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=2 -S %s | FileCheck %s --check-prefixes=CHECK,CAP2
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=3 -S %s | FileCheck %s --check-prefixes=CHECK,CAP3

; Each mask built adds a live i1, so their number is capped (default 8). The three
; loads here have three different conditions, so none can reuse another's mask and
; each costs one. With a cap of two the third load keeps its original predicate.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @mask_cap(
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %p2 = getelementptr i32, ptr addrspace(1) %p, i64 2
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %c0
; CHECK-NEXT:    %l0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %predload.shrunk1 = and i1 %q, %c1
; CHECK-NEXT:    %l1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk1, i32 0)
; CAP2-NEXT:    %l2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
; CAP3-NEXT:    %predload.shrunk2 = and i1 %q, %c2
; CAP3-NEXT:    %l2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %predload.shrunk2, i32 0)
; CHECK-NEXT:    %s0 = select i1 %c0, i32 %l0, i32 %other
; CHECK-NEXT:    %s1 = select i1 %c1, i32 %l1, i32 %other
; CHECK-NEXT:    %s2 = select i1 %c2, i32 %l2, i32 %other
; CHECK-NEXT:    %r0 = add i32 %s0, %s1
; CHECK-NEXT:    %r = add i32 %r0, %s2
; CHECK-NEXT:    ret i32 %r

define i32 @mask_cap(ptr addrspace(1) %p, i1 %q, i1 noundef %c0, i1 noundef %c1, i1 noundef %c2, i32 %other) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %p2 = getelementptr i32, ptr addrspace(1) %p, i64 2
  %l0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %l1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %l2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p2, i64 4, i1 %q, i32 0)
  %s0 = select i1 %c0, i32 %l0, i32 %other
  %s1 = select i1 %c1, i32 %l1, i32 %other
  %s2 = select i1 %c2, i32 %l2, i32 %other
  %r0 = add i32 %s0, %s1
  %r = add i32 %r0, %s2
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
