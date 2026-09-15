;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; When the use predicate joins several equivalent but distinct conditions, the
; shrink freezes one of them once, and every select that read one of the
; conditions reads that freeze. The mask is built from it too, so the load and
; all its users agree on the condition.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %c1 and %c2 compare the same %n, which may be undef. %c2 is frozen, %u1 reads
; %c2.fr instead of %c1, and %c1 is deleted.
; CHECK-LABEL: define i32 @equivalent_icmps(
; CHECK-NEXT:    %c2 = icmp ult i32 %n, 8
; CHECK-NEXT:    %c2.fr = freeze i1 %c2
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %c2.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %u1 = select i1 %c2.fr, i32 %a, i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2.fr, i32 %a, i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    ret i32 %r

define i32 @equivalent_icmps(ptr addrspace(1) %p, i1 %q, i32 %n, i32 %x, i32 %y) {
  %c1 = icmp ult i32 %n, 8
  %c2 = icmp ult i32 %n, 8
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %u1 = select i1 %c1, i32 %a, i32 %x
  %u2 = select i1 %c2, i32 %a, i32 %y
  %r = add i32 %u1, %u2
  ret i32 %r
}

; The conditions are computed after the load. %s and %c2 are hoisted above it,
; so %c2.fr dominates both selects.
; CHECK-LABEL: define i32 @equivalent_icmps_after_load(
; CHECK-NEXT:    %s = add i32 %n, 1
; CHECK-NEXT:    %c2 = icmp ult i32 %s, 8
; CHECK-NEXT:    %c2.fr = freeze i1 %c2
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %c2.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %u1 = select i1 %c2.fr, i32 %a, i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2.fr, i32 %a, i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    ret i32 %r

define i32 @equivalent_icmps_after_load(ptr addrspace(1) %p, i1 %q, i32 %n, i32 %x, i32 %y) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %s = add i32 %n, 1
  %c1 = icmp ult i32 %s, 8
  %c2 = icmp ult i32 %s, 8
  %u1 = select i1 %c1, i32 %a, i32 %x
  %u2 = select i1 %c2, i32 %a, i32 %y
  %r = add i32 %u1, %u2
  ret i32 %r
}

; `fcmp nnan` can create poison even though %f is noundef, so freezing %f would
; not make the conditions well defined. Freezing condition does.
; CHECK-LABEL: define i32 @equivalent_poison_generating_fcmps(
; CHECK-NEXT:    %c2 = fcmp nnan olt float %f, 1.000000e+00
; CHECK-NEXT:    %c2.fr = freeze i1 %c2
; CHECK-NEXT:    %predload.shrunk = and i1 %q, %c2.fr
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %u1 = select i1 %c2.fr, i32 %a, i32 %x
; CHECK-NEXT:    %u2 = select i1 %c2.fr, i32 %a, i32 %y
; CHECK-NEXT:    %r = add i32 %u1, %u2
; CHECK-NEXT:    ret i32 %r

define i32 @equivalent_poison_generating_fcmps(ptr addrspace(1) %p, i1 %q, float noundef %f, i32 %x, i32 %y) {
  %c1 = fcmp nnan olt float %f, 1.0
  %c2 = fcmp nnan olt float %f, 1.0
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %u1 = select i1 %c1, i32 %a, i32 %x
  %u2 = select i1 %c2, i32 %a, i32 %y
  %r = add i32 %u1, %u2
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
