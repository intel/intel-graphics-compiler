;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=0 --verify -S %s | FileCheck %s --check-prefixes=CHECK,ZERO
; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --igc-shrink-predload-max-masks=1 --verify -S %s | FileCheck %s --check-prefixes=CHECK,ONE

; The mask budget counts the `and` instructions built, and an unconditional
; predicate needs none: it shrinks to the existing condition %c. So %a is
; free to shrink.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @existing_condition_is_free(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %c, i32 0)
; ZERO-NEXT:     %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %otherp, i64 4, i1 %q, i32 0)
; ONE-NEXT:      %predload.shrunk = and i1 %q, %d
; ONE-NEXT:      %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %otherp, i64 4, i1 %predload.shrunk, i32 0)
; CHECK-NEXT:    %sa = select i1 %c, i32 %a, i32 1
; CHECK-NEXT:    %sb = select i1 %d, i32 %b, i32 1
; CHECK-NEXT:    %r = add i32 %sa, %sb
; CHECK-NEXT:    ret i32 %r

define i32 @existing_condition_is_free(ptr addrspace(1) %p, ptr addrspace(1) %otherp, i1 noundef %q, i1 noundef %c, i1 noundef %d) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 true, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %otherp, i64 4, i1 %q, i32 0)
  %sa = select i1 %c, i32 %a, i32 1
  %sb = select i1 %d, i32 %b, i32 1
  %r = add i32 %sa, %sb
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
