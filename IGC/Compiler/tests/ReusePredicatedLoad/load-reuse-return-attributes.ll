;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; Call-site attributes are part of a load's identity. Reusing the range-limited
; result for a call without that fact would make the value poison whenever the
; memory holds something outside [1, 10), so both loads survive.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @different_return_attributes(
; CHECK-NEXT:    %a = call range(i32 1, 10) i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %s = select i1 %use_first, i32 %a, i32 0
; CHECK-NEXT:    %r = add i32 %s, %b
; CHECK-NEXT:    ret i32 %r

define i32 @different_return_attributes(ptr addrspace(1) %p, i1 %q, i1 %use_first) {
  %a = call range(i32 1, 10) i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %s = select i1 %use_first, i32 %a, i32 0
  %r = add i32 %s, %b
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
