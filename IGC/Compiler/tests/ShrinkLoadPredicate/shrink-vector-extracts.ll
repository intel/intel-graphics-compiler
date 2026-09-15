;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate -S %s | FileCheck %s

; An i32 load is bitcast to two halves. Both extracts feed the false value of
; selects on the same condition, so one IfFalse use predicate reaches the load.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define <2 x half> @vector_uses(
; CHECK-NEXT:    %predload.usepred.not = xor i1 %A, true
; CHECK-NEXT:    %predload.shrunk = and i1 %S, %predload.usepred.not
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %halves = bitcast i32 %a to <2 x half>
; CHECK-NEXT:    %lo = extractelement <2 x half> %halves, i32 0
; CHECK-NEXT:    %hi = extractelement <2 x half> %halves, i32 1
; CHECK-NEXT:    %rlo = select i1 %A, half %x, half %lo
; CHECK-NEXT:    %rhi = select i1 %A, half %y, half %hi
; CHECK-NEXT:    %v0 = insertelement <2 x half> poison, half %rlo, i32 0
; CHECK-NEXT:    %result = insertelement <2 x half> %v0, half %rhi, i32 1
; CHECK-NEXT:    ret <2 x half> %result

define <2 x half> @vector_uses(ptr addrspace(1) %p, i1 %S, i1 noundef %A, half %x, half %y) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %halves = bitcast i32 %a to <2 x half>
  %lo = extractelement <2 x half> %halves, i32 0
  %hi = extractelement <2 x half> %halves, i32 1
  %rlo = select i1 %A, half %x, half %lo
  %rhi = select i1 %A, half %y, half %hi
  %v0 = insertelement <2 x half> poison, half %rlo, i32 0
  %result = insertelement <2 x half> %v0, half %rhi, i32 1
  ret <2 x half> %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
