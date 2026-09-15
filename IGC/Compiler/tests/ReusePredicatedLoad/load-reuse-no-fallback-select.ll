;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s
; REQUIRES: llvm-22-plus

; LLVM 14-17 do not look through a negated left-hand implication condition.

; Reuse needs no fallback select when every consumer already ignores the later
; result outside its predicate. In @true_incoming_value the consumer is conditional
; on the AND mask itself, which implies the earlier predicate; in @false_incoming_value
; it is conditional on the negation of the later predicate, which implies the earlier
; unsigned bound.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define float @true_incoming_value(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %mask = and i1 %S, %A
; CHECK-NEXT:    %af = bitcast i32 %a to float
; CHECK-NEXT:    %bf = bitcast i32 %a to float
; CHECK-NEXT:    %result = select i1 %mask, float %bf, float %af
; CHECK-NEXT:    ret float %result

define float @true_incoming_value(ptr addrspace(1) %p, i1 noundef %S, i1 noundef %A) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %mask = and i1 %S, %A
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 37)
  %af = bitcast i32 %a to float
  %bf = bitcast i32 %b to float
  %result = select i1 %mask, float %bf, float %af
  ret float %result
}

; CHECK-LABEL: define float @false_incoming_value(
; CHECK-NEXT:    %S = icmp ult i32 %x, 100
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %C = icmp ugt i32 %x, 50
; CHECK-NEXT:    %af = bitcast i32 %a to float
; CHECK-NEXT:    %bf = bitcast i32 %a to float
; CHECK-NEXT:    %result = select i1 %C, float %af, float %bf
; CHECK-NEXT:    ret float %result

define float @false_incoming_value(ptr addrspace(1) %p, i32 noundef %x) {
  %S = icmp ult i32 %x, 100
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %C = icmp ugt i32 %x, 50
  %mask = xor i1 %C, true
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 37)
  %af = bitcast i32 %a to float
  %bf = bitcast i32 %b to float
  %result = select i1 %C, float %af, float %bf
  ret float %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
