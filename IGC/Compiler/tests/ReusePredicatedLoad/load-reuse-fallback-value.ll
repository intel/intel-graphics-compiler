;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s
; REQUIRES: llvm-22-plus

; LLVM 14-17 do not prove the mixed signed/unsigned implication below.

; An unsigned comparison implies the matching signed comparison for a positive
; bound, so @reuse replaces the second load. Its result is observed
; unconditionally, so a select restores the fallback value the removed load would
; have produced on inactive lanes. The implication does not hold in the other
; direction - a negative %x satisfies `slt` but not `ult` - so @reverse keeps
; both loads.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @reuse(
; CHECK-NEXT:    %S = icmp slt i32 %x, 4243200
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 11)
; CHECK-NEXT:    %B = icmp ult i32 %x, 4243200
; CHECK-NEXT:    %[[V1:[0-9]+]] = select i1 %B, i32 %a, i32 37
; CHECK-NEXT:    %result = add i32 %a, %[[V1]]
; CHECK-NEXT:    ret i32 %result

define i32 @reuse(ptr addrspace(1) %p, i32 noundef %x) {
  %S = icmp slt i32 %x, 4243200
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 11)
  %B = icmp ult i32 %x, 4243200
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %B, i32 37)
  %result = add i32 %a, %b
  ret i32 %result
}

; CHECK-LABEL: define i32 @reverse(
; CHECK-NEXT:    %B = icmp ult i32 %x, 4243200
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %B, i32 0)
; CHECK-NEXT:    %S = icmp slt i32 %x, 4243200
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
; CHECK-NEXT:    %result = add i32 %a, %b
; CHECK-NEXT:    ret i32 %result

define i32 @reverse(ptr addrspace(1) %p, i32 %x) {
  %B = icmp ult i32 %x, 4243200
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %B, i32 0)
  %S = icmp slt i32 %x, 4243200
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %S, i32 0)
  %result = add i32 %a, %b
  ret i32 %result
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
