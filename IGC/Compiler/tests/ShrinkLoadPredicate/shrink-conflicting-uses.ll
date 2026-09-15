;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; A load needed under both a condition and its negation has no single use
; condition, so it keeps its predicate.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @conflicting(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %true.use = select i1 %A, i32 %a, i32 %x
; CHECK-NEXT:    %false.use = select i1 %A, i32 %y, i32 %a
; CHECK-NEXT:    %result = add i32 %true.use, %false.use
; CHECK-NEXT:    ret i32 %result

define i32 @conflicting(ptr addrspace(1) %p, i1 %S, i1 %A, i32 %x, i32 %y) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %true.use = select i1 %A, i32 %a, i32 %x
  %false.use = select i1 %A, i32 %y, i32 %a
  %result = add i32 %true.use, %false.use
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
