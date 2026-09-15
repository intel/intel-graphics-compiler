;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; The condition needs four instructions moved before the load, which exceeds the
; default hoist budget of two, so the load keeps its predicate.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @hoist_cap(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %x1 = add i32 %x, 1
; CHECK-NEXT:    %x2 = xor i32 %x1, 7
; CHECK-NEXT:    %x3 = add i32 %x2, 3
; CHECK-NEXT:    %cond = icmp ult i32 %x3, 100
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %r

define i32 @hoist_cap(ptr addrspace(1) %p, i1 %q, i32 %x, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %x1 = add i32 %x, 1
  %x2 = xor i32 %x1, 7
  %x3 = add i32 %x2, 3
  %cond = icmp ult i32 %x3, 100
  %r = select i1 %cond, i32 %a, i32 %other
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
