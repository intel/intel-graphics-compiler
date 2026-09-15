;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; Every predicate implies an unconditional mask, so the predicated load reuses the
; unconditional one and keeps its own fallback value.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @unconditional_candidate(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 0)
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %q, i32 %a, i32 7
; CHECK-NEXT:    %r = add i32 %a, %[[V1]]
; CHECK-NEXT:    ret i32 %r

define i32 @unconditional_candidate(ptr addrspace(1) %p, i1 %q) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 7)
  %r = add i32 %a, %b
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
