;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-15-plus
; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; LLVM 14 does not look through an `or` on the right-hand side of an implication,
; which proving %q => (or %q, %cx) needs.

; The walk for the values two predicates share skips operands that carry no
; data value.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Both read_register calls read the same metadata !0, which is not a shared
; value. The only shared value is the noundef %q, and %q2 => %q => %q1 needs
; nothing else, so %b is reused without a freeze.
; CHECK-LABEL: define i32 @shared_metadata(
; CHECK-NOT:     freeze
; CHECK:         %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
; CHECK-NEXT:    %[[SEL:[0-9]+]] = select i1 %q2, i32 %a, i32 13
; CHECK-NEXT:    %sum = add i32 %a, %[[SEL]]
; CHECK-NEXT:    ret i32 %sum
define i32 @shared_metadata(ptr addrspace(1) %p, i1 noundef %q) {
  %x = call i32 @llvm.read_register.i32(metadata !0)
  %y = call i32 @llvm.read_register.i32(metadata !0)
  %cx = icmp eq i32 %x, 0
  %cy = icmp eq i32 %y, 0
  %q1 = or i1 %q, %cx
  %q2 = and i1 %q, %cy
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q1, i32 7)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q2, i32 13)
  %sum = add i32 %a, %b
  ret i32 %sum
}

declare i32 @llvm.read_register.i32(metadata)
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
!0 = !{!"eax"}
