;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-shrink-load-predicate --verify -S %s | FileCheck %s

; Fast-math flags assert facts about the result of a load. Where %cond is false,
; the select returns the fallback value. A shrunk load returns the same fallback
; value there, but nnan or ninf makes it poison, so the select cannot be replaced
; by the load. A load with either flag is left unchanged.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; nnan: the fallback value is NaN.
; CHECK-LABEL: define float @nan_fallback(
; CHECK-NEXT: %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float [[NAN:0x7FF8000000000000|\+qnan]])
; CHECK-NEXT: %r = select i1 %cond, float %a, float [[NAN]]
; CHECK-NEXT: ret float %r

define float @nan_fallback(ptr addrspace(1) %p, i1 noundef %q, i1 noundef %cond) {
  %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0x7FF8000000000000)
  %r = select i1 %cond, float %a, float 0x7FF8000000000000
  ret float %r
}

; ninf: the fallback value is +Inf.
; CHECK-LABEL: define float @inf_fallback(
; CHECK-NEXT: %a = call ninf float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float [[INF:0x7FF0000000000000|\+inf]])
; CHECK-NEXT: %r = select i1 %cond, float %a, float [[INF]]
; CHECK-NEXT: ret float %r

define float @inf_fallback(ptr addrspace(1) %p, i1 noundef %q, i1 noundef %cond) {
  %a = call ninf float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0x7FF0000000000000)
  %r = select i1 %cond, float %a, float 0x7FF0000000000000
  ret float %r
}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1), i64, i1, float) nounwind willreturn readonly
