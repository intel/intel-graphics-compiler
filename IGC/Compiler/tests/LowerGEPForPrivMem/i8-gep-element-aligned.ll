;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S %s | FileCheck %s

; InstCombine canonicalizes constant-index GEPs into the byte-offset ("ptradd")
; form `getelementptr i8, ptr %p, i64 <bytes>`. A byte offset that is an exact
; multiple of the promoted lane size still addresses whole elements, so it must
; not block register promotion. A sub-element offset, a byte-typed access or a
; dynamic offset must still block it.

; Byte offsets 4 and 60 into [16 x <2 x half>] are multiples of the 2-byte lane,
; so the array promotes to <32 x half> and the offsets become lanes 2 and 30.
; CHECK-LABEL: define void @aligned_vector_lane(
; CHECK: alloca <32 x half>
; CHECK-NOT: alloca [16 x <2 x half>]
; CHECK: insertelement <32 x half> %{{.*}}, half %{{.*}}, i32 2
; CHECK: extractelement <32 x half> %{{.*}}, i32 30
define void @aligned_vector_lane() {
  %a = alloca [16 x <2 x half>], align 4
  %p0 = getelementptr inbounds i8, ptr %a, i64 0
  %p1 = getelementptr inbounds i8, ptr %a, i64 4
  %p2 = getelementptr inbounds i8, ptr %a, i64 60
  %l0 = load <2 x half>, ptr %p0, align 4
  store <2 x half> %l0, ptr %p1, align 4
  %l2 = load <2 x half>, ptr %p2, align 4
  store <2 x half> %l2, ptr %p0, align 4
  ret void
}

; Offset 2 is a multiple of the half lane but not of the <2 x half> element. This
; path indexes the promoted vector in scalar units, so it still promotes - the
; mirror case in the PrivateMemoryResolution suite must reject it.
; CHECK-LABEL: define void @half_aligned_vector_offset(
; CHECK: alloca <32 x half>
; CHECK-NOT: alloca [16 x <2 x half>]
define void @half_aligned_vector_offset() {
  %a = alloca [16 x <2 x half>], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 2
  %l = load <2 x half>, ptr %p, align 2
  store <2 x half> %l, ptr %a, align 4
  ret void
}

; Byte offset 12 into [8 x float] is element 3.
; CHECK-LABEL: define void @aligned_scalar(
; CHECK: alloca <8 x float>
; CHECK: extractelement <8 x float> %{{.*}}, i32 3
define void @aligned_scalar() {
  %a = alloca [8 x float], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 12
  %l = load float, ptr %p, align 4
  store float %l, ptr %a, align 4
  ret void
}

; A dynamically indexed typed GEP followed by a byte-offset GEP - the shape
; InstCombine produces for `arr[i + 1]`. Byte offset 4 must become element index 1
; and be added to the dynamic index, since the promoted index counts float lanes.
; CHECK-LABEL: define void @typed_gep_then_byte_offset(
; CHECK: alloca <8 x float>
; CHECK: add i32 1, %{{.*}}
; CHECK-NOT: add i32 4, %{{.*}}
define void @typed_gep_then_byte_offset(i64 %i) {
  %a = alloca [8 x float], align 4
  %p = getelementptr inbounds float, ptr %a, i64 %i
  %q = getelementptr inbounds i8, ptr %p, i64 4
  %l = load float, ptr %q, align 4
  store float %l, ptr %a, align 4
  ret void
}

; Offset 2 is inside a float, not on an element boundary.
; CHECK-LABEL: define void @subelement_offset(
; CHECK: alloca [8 x float]
; CHECK-NOT: alloca <8 x float>
define void @subelement_offset() {
  %a = alloca [8 x float], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 2
  %l = load float, ptr %p, align 2
  store float %l, ptr %a, align 4
  ret void
}

; The offset is element-aligned but the access itself is byte-granular, i.e. the
; memcpy-like pattern the guard exists for. The promoted vector holds floats and
; cannot express a single-byte lane.
; CHECK-LABEL: define void @byte_typed_access(
; CHECK: alloca [8 x float]
; CHECK-NOT: alloca <8 x float>
define void @byte_typed_access() {
  %a = alloca [8 x float], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 4
  %l = load i8, ptr %p, align 1
  store i8 %l, ptr %a, align 1
  ret void
}

; A dynamic byte offset cannot be proven element-aligned.
; CHECK-LABEL: define void @dynamic_byte_offset(
; CHECK: alloca [8 x float]
; CHECK-NOT: alloca <8 x float>
define void @dynamic_byte_offset(i64 %i) {
  %a = alloca [8 x float], align 4
  %p = getelementptr inbounds i8, ptr %a, i64 %i
  %l = load float, ptr %p, align 4
  store float %l, ptr %a, align 4
  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6, !7, !8}
!0 = !{ptr @aligned_vector_lane, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @aligned_scalar, !1}
!7 = !{ptr @half_aligned_vector_offset, !1}
!8 = !{ptr @typed_gep_then_byte_offset, !1}
!4 = !{ptr @subelement_offset, !1}
!5 = !{ptr @byte_typed_access, !1}
!6 = !{ptr @dynamic_byte_offset, !1}
