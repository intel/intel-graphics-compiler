;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; Alignment, metadata and the address space are part of a load's identity, and
; only a call that cannot write memory keeps earlier loads available.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @different_alignment(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 8, i1 %q, i32 0)
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @different_alignment(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 8, i1 %q, i32 0)
  %r = add i32 %a, %b
  ret i32 %r
}

; CHECK-LABEL: define i32 @different_metadata(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !nontemporal ![[NT:[0-9]+]]
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !invariant.load ![[INV:[0-9]+]]
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @different_metadata(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !nontemporal !0
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !invariant.load !1
  %r = add i32 %a, %b
  ret i32 %r
}

; Matching is done on the pointer value, so a cast of the same address is a
; different candidate.
; CHECK-LABEL: define i32 @different_address_space(
; CHECK-NEXT:    %generic = addrspacecast ptr addrspace(1) %p to ptr addrspace(4)
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p4.i32(ptr addrspace(4) %generic, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @different_address_space(ptr addrspace(1) %p, i1 noundef %q) {
  %generic = addrspacecast ptr addrspace(1) %p to ptr addrspace(4)
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p4.i32(ptr addrspace(4) %generic, i64 4, i1 %q, i32 0)
  %r = add i32 %a, %b
  ret i32 %r
}

; CHECK-LABEL: define i32 @readonly_call(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %ignore = call i32 @readonly(ptr addrspace(1) %p)
; CHECK-NEXT:    %r = add i32 %a, %a
; CHECK-NEXT:    ret i32 %r

define i32 @readonly_call(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %ignore = call i32 @readonly(ptr addrspace(1) %p)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %r = add i32 %a, %b
  ret i32 %r
}

; CHECK-LABEL: define i32 @opaque_call(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %ignore = call i32 @opaque(ptr addrspace(1) %p)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %r = add i32 %a, %b
; CHECK-NEXT:    ret i32 %r

define i32 @opaque_call(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %ignore = call i32 @opaque(ptr addrspace(1) %p)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %r = add i32 %a, %b
  ret i32 %r
}

declare i32 @readonly(ptr addrspace(1)) readonly
declare i32 @opaque(ptr addrspace(1))
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p4.i32(ptr addrspace(4), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }

!0 = !{i32 1}
!1 = !{}
