;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The walk for the values two predicates share has a fixed depth limit (8).

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The freeze walk is shorter than what the proof can see. The implication query
; finds %q nine `and` levels below %m9, but the walk stops at %m1, eight levels
; down. When the walk is cut short above a value that may be undef, a shared
; value may be hidden below it (here %q), so the reuse is rejected. Otherwise %b
; would become a select over %a with %q not frozen.
; CHECK-LABEL: define i32 @deep_mask_undef(
; CHECK-NOT:     freeze
; CHECK:         %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
; CHECK-NOT:     freeze
; CHECK:         %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m9, i32 13)
; CHECK-NEXT:    %s = add i32 %a, %b

define i32 @deep_mask_undef(ptr addrspace(1) %p, i32 %u, i1 %t) {
  %q = icmp ult i32 %u, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m1 = and i1 %q, %t
  %m2 = and i1 %m1, %t
  %m3 = and i1 %m2, %t
  %m4 = and i1 %m3, %t
  %m5 = and i1 %m4, %t
  %m6 = and i1 %m5, %t
  %m7 = and i1 %m6, %t
  %m8 = and i1 %m7, %t
  %m9 = and i1 %m8, %t
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m9, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

; With noundef arguments every value in both predicates is well defined. The
; walk still stops at %m1, but %m1 cannot be undef or poison, so nothing hidden
; below it needs a freeze, and %b is reused with a fallback select.
; CHECK-LABEL: define i32 @deep_mask_noundef(
; CHECK-NOT:     freeze
; CHECK:         %m9 = and i1 %m8, %t
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %m9, i32 %a, i32 13
; CHECK-NEXT:    %s = add i32 %a, %[[SEL]]

define i32 @deep_mask_noundef(ptr addrspace(1) %p, i32 noundef %u, i1 noundef %t) {
  %q = icmp ult i32 %u, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %m1 = and i1 %q, %t
  %m2 = and i1 %m1, %t
  %m3 = and i1 %m2, %t
  %m4 = and i1 %m3, %t
  %m5 = and i1 %m4, %t
  %m6 = and i1 %m5, %t
  %m7 = and i1 %m6, %t
  %m8 = and i1 %m7, %t
  %m9 = and i1 %m8, %t
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m9, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
