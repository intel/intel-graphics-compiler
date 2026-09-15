;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-load-predicate -S %s | FileCheck %s

; Shrinking is skipped when the predicate already implies the condition, even when
; the condition is not a comparison: @already_implied is left alone, and
; @needs_shrinking gets exactly one extra mask. Running the pass twice must
; produce the same output as running it once.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @already_implied(
; CHECK-NEXT:    %SA = and i1 %S, %A
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %SA, i32 0)
; CHECK-NEXT:    %result = select i1 %A, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %result

define i32 @already_implied(ptr addrspace(1) %p, i1 %S, i1 noundef %A, i32 %other) {
  %SA = and i1 %S, %A
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %SA, i32 0)
  %result = select i1 %A, i32 %a, i32 %other
  ret i32 %result
}

; CHECK-LABEL: define i32 @needs_shrinking(
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %A
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %result = select i1 %A, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %result

define i32 @needs_shrinking(ptr addrspace(1) %p, i1 %S, i1 noundef %A, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %result = select i1 %A, i32 %a, i32 %other
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
