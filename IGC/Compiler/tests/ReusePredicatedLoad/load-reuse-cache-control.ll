;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; Cache decorations are part of a load's identity: only @matching is reused,
; and the surviving load keeps its decoration.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @different(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl ![[C0:[0-9]+]]
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl ![[C1:[0-9]+]]
; CHECK-NEXT:    %result = add i32 %a, %b
; CHECK-NEXT:    ret i32 %result

define i32 @different(ptr addrspace(1) %p, i1 noundef %mask) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl !0
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl !1
  %result = add i32 %a, %b
  ret i32 %result
}

; CHECK-LABEL: define i32 @matching(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl ![[POLICY:[0-9]+]]
; CHECK-NEXT:    %result = add i32 %a, %a
; CHECK-NEXT:    ret i32 %result
; CHECK: ![[POLICY]] = !{i32 4}

define i32 @matching(ptr addrspace(1) %p, i1 noundef %mask) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl !0
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0), !lsc.cache.ctrl !0
  %result = add i32 %a, %b
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }

!0 = !{i32 4}
!1 = !{i32 2}
