;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; A condition derived from a memory read cannot be hoisted before the load: the
; read would be reordered against whatever writes that memory.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @memory_condition(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %condition = load i32, ptr addrspace(1) %q, align 4
; CHECK-NEXT:    %A = icmp eq i32 %condition, 0
; CHECK-NEXT:    %result = select i1 %A, i32 %a, i32 %other
; CHECK-NEXT:    ret i32 %result

define i32 @memory_condition(ptr addrspace(1) %p, ptr addrspace(1) %q, i1 %S, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %condition = load i32, ptr addrspace(1) %q, align 4
  %A = icmp eq i32 %condition, 0
  %result = select i1 %A, i32 %a, i32 %other
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
