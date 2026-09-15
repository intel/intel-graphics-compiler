;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; The load is observed only through the false value of the select, so the
; condition enters the predicate negated.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @shrink(
; CHECK-NEXT:    %predload.usepred.not = xor i1 %A, true
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %predload.usepred.not
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 19)
; CHECK-NEXT:    %result = select i1 %A, i32 %other, i32 %a
; CHECK-NEXT:    ret i32 %result

define i32 @shrink(ptr addrspace(1) %p, i1 %S, i1 noundef %A, i32 %other) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 19)
  %result = select i1 %A, i32 %other, i32 %a
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
