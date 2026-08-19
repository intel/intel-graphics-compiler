;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-private-mem-resolution --platformbmg -S %s | FileCheck %s

; A dynamically indexed typed GEP followed by a constant byte-offset ("ptradd")
; GEP - the shape InstCombine produces for `arr[i + 1]`. The byte offset is an
; exact multiple of the 8-byte base type, so the array must keep its SoA
; (SIMD-interleaved) scratch layout: the per-lane base advances by one element
; (laneId * 8), not by the whole array (laneId * 64), and the offset becomes
; element index 1.

; CHECK-LABEL: define spir_kernel void @aligned_byte_gep_chain(
; CHECK: mul i32 %{{.*}}, 8
; CHECK-NOT: mul i32 %{{.*}}, 64
; CHECK: add i32 1, %{{.*}}
; CHECK-NOT: getelementptr
define spir_kernel void @aligned_byte_gep_chain(i64 %i) {
  %a = alloca [8 x double], align 8
  %p = getelementptr inbounds double, ptr %a, i64 %i
  %q = getelementptr inbounds i8, ptr %p, i64 8
  %v = load double, ptr %q, align 8
  store double %v, ptr %q, align 8
  ret void
}

; Offset 4 is inside a double, so the layout must stay per-lane contiguous (AoS)
; and the GEPs must be left alone.
; CHECK-LABEL: define spir_kernel void @subelement_offset(
; CHECK: mul i32 %{{.*}}, 64
; CHECK: getelementptr inbounds i8
define spir_kernel void @subelement_offset(i64 %i) {
  %a = alloca [8 x double], align 8
  %p = getelementptr inbounds double, ptr %a, i64 %i
  %q = getelementptr inbounds i8, ptr %p, i64 4
  %v = load double, ptr %q, align 4
  store double %v, ptr %q, align 4
  ret void
}

; The offset is element-aligned but the access is byte-granular - the memcpy-like
; pattern the guard exists for.
; CHECK-LABEL: define spir_kernel void @byte_typed_access(
; CHECK: mul i32 %{{.*}}, 64
; CHECK: getelementptr inbounds i8
define spir_kernel void @byte_typed_access() {
  %a = alloca [8 x double], align 8
  %q = getelementptr inbounds i8, ptr %a, i64 8
  %v = load i8, ptr %q, align 1
  store i8 %v, ptr %q, align 1
  ret void
}

; Scratch strides by the whole base type, so offset 2 is sub-element even though
; it is a multiple of the half. The register-promotion path indexes in scalar
; units and accepts the same IR - see LowerGEPForPrivMem/i8-gep-element-aligned.ll.
; CHECK-LABEL: define spir_kernel void @vector_subelement_offset(
; CHECK: mul i32 %{{.*}}, 64
; CHECK: getelementptr inbounds i8
define spir_kernel void @vector_subelement_offset() {
  %a = alloca [16 x <2 x half>], align 4
  %q = getelementptr inbounds i8, ptr %a, i64 2
  %v = load <2 x half>, ptr %q, align 2
  store <2 x half> %v, ptr %q, align 2
  ret void
}

!igc.functions = !{!1, !4, !5, !6}
!1 = !{ptr @aligned_byte_gep_chain, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{ptr @subelement_offset, !2}
!5 = !{ptr @byte_typed_access, !2}
!6 = !{ptr @vector_subelement_offset, !2}
