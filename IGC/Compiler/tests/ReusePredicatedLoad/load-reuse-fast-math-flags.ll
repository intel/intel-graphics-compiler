;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; Fast-math flags assert facts about the result of a load, so a candidate is
; reused only when its flags equal the later load's.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; %a is `nnan` and %b has no flags. With a NaN in memory, %safe is some defined
; value and %b is NaN, so %s is NaN. Reusing %a for %b would make %s poison.
; CHECK-LABEL: define float @different_fast_math_flags(
; CHECK-NEXT:    %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.000000e+00)
; CHECK-NEXT:    %safe = freeze float %a
; CHECK-NEXT:    %b = call float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.000000e+00)
; CHECK-NEXT:    %s = fadd float %safe, %b
; CHECK-NEXT:    ret float %s

define float @different_fast_math_flags(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.0)
  %safe = freeze float %a
  %b = call float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.0)
  %s = fadd float %safe, %b
  ret float %s
}

; Equal flags: %b is replaced by %a.
; CHECK-LABEL: define float @equal_fast_math_flags(
; CHECK-NEXT:    %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.000000e+00)
; CHECK-NEXT:    %safe = freeze float %a
; CHECK-NEXT:    %s = fadd float %safe, %a
; CHECK-NEXT:    ret float %s

define float @equal_fast_math_flags(ptr addrspace(1) %p, i1 noundef %q) {
  %a = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.0)
  %safe = freeze float %a
  %b = call nnan float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) %p, i64 4, i1 %q, float 0.0)
  %s = fadd float %safe, %b
  ret float %s
}

declare float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1), i64, i1, float) #0
attributes #0 = { nounwind willreturn readonly }
