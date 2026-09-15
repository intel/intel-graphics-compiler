;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; A freeze is inserted only when the load is reused. Whether every shared value
; can be frozen is checked before any of them is, so a reuse rejected for one
; shared value leaves the function unchanged.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %m implies %c, and the two share %q and a literal undef. The literal undef
; cannot be frozen, so the reuse is rejected before %q is frozen.
; CHECK-LABEL: define i32 @shared_literal_undef(
; CHECK-NEXT:    %c = or i1 %q, undef
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c, i32 7)
; CHECK-NEXT:    %m = and i1 %q, undef
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
; CHECK-NEXT:    %s = add i32 %a, %b
; CHECK-NEXT:    ret i32 %s

define i32 @shared_literal_undef(ptr addrspace(1) %p, i1 %q) {
  %c = or i1 %q, undef
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c, i32 7)
  %m = and i1 %q, undef
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %m, i32 13)
  %s = add i32 %a, %b
  ret i32 %s
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
