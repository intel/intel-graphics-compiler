;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; The AND mask implies the earlier predicate, so the load is reused, but the
; consumer is conditional on %B alone. %S compares an index %B says nothing about,
; so %B does not imply the mask and the fallback value has to be restored by a
; select.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @distinct_operands(
; CHECK-NEXT:    %index = or i32 %base, %lane
; CHECK-NEXT:    %S = icmp slt i32 %index, 4243200
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %B = icmp ult i32 %base, 4243200
; CHECK-NEXT:    %mask = and i1 %B, %S
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %mask, i32 %a, i32 0
; CHECK-NEXT:    %middle = select i1 %B, i32 %[[V1]], i32 %a
; CHECK-NEXT:    %result = select i1 %A, i32 %other, i32 %middle
; CHECK-NEXT:    ret i32 %result

define i32 @distinct_operands(ptr addrspace(1) %p, i32 noundef %base, i32 noundef %lane, i1 %A, i32 %other) {
  %index = or i32 %base, %lane
  %S = icmp slt i32 %index, 4243200
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %B = icmp ult i32 %base, 4243200
  %mask = and i1 %B, %S
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %mask, i32 0)
  %middle = select i1 %B, i32 %b, i32 %a
  %result = select i1 %A, i32 %other, i32 %middle
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
