;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=1 -S %s | FileCheck %s

; Loads that agree on predicate and condition share one mask instruction.
;
; The second RUN line is the same check at a mask budget of one: reusing a mask
; builds no instruction and adds no live i1, so it is not charged and a budget
; that allows a single mask is enough for both loads.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @plain_condition(
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %sa = select i1 %cond, i32 %a, i32 %other
; CHECK-NEXT:    %sb = select i1 %cond, i32 %b, i32 %other
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @plain_condition(ptr addrspace(1) %p, i1 %q, i1 noundef %cond, i32 %other) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %sa = select i1 %cond, i32 %a, i32 %other
  %sb = select i1 %cond, i32 %b, i32 %other
  %r = add i32 %sa, %sb
  ret i32 %r
}

; CHECK-LABEL: define i32 @negated_condition(
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %predload.usepred.not = xor i1 %cond, true
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %predload.usepred.not
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %sa = select i1 %cond, i32 %other, i32 %a
; CHECK-NEXT:    %sb = select i1 %cond, i32 %other, i32 %b
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @negated_condition(ptr addrspace(1) %p, i1 %q, i1 noundef %cond, i32 %other) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %sa = select i1 %cond, i32 %other, i32 %a
  %sb = select i1 %cond, i32 %other, i32 %b
  %r = add i32 %sa, %sb
  ret i32 %r
}

; CHECK-LABEL: define i32 @hoisted_condition(
; CHECK-NEXT:    %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
; CHECK-NEXT:    %cond = icmp ult i32 %n, 8
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %cond
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %sa = select i1 %cond, i32 %a, i32 %other
; CHECK-NEXT:    %sb = select i1 %cond, i32 %b, i32 %other
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @hoisted_condition(ptr addrspace(1) %p, i1 %q, i32 noundef %n, i32 %other) {
  %p1 = getelementptr i32, ptr addrspace(1) %p, i64 1
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p1, i64 4, i1 %q, i32 0)
  %cond = icmp ult i32 %n, 8
  %sa = select i1 %cond, i32 %a, i32 %other
  %sb = select i1 %cond, i32 %b, i32 %other
  %r = add i32 %sa, %sb
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
