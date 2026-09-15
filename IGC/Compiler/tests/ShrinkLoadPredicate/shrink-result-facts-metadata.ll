;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; Shrinking makes the fallback value observable where memory used to be read, so a
; load whose result carries facts is left alone.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The load's fallback value 0 is outside the asserted range [1, 10).
; CHECK-LABEL: define i32 @range_metadata(
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !range !{{[0-9]+}}
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 9
; CHECK-NEXT:    ret i32 %r

define i32 @range_metadata(ptr addrspace(1) %p, i1 %q, i1 %cond) {
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0), !range !0
  %r = select i1 %cond, i32 %a, i32 9
  ret i32 %r
}

; The load's fallback value is undef, but the result is asserted noundef.
; CHECK-LABEL: define i32 @noundef_attribute(
; CHECK-NEXT:    %a = call noundef i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 undef)
; CHECK-NEXT:    %r = select i1 %cond, i32 %a, i32 9
; CHECK-NEXT:    ret i32 %r

define i32 @noundef_attribute(ptr addrspace(1) %p, i1 %q, i1 %cond) {
  %a = call noundef i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 undef)
  %r = select i1 %cond, i32 %a, i32 9
  ret i32 %r
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }

!0 = !{i32 1, i32 10}
