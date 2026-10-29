;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S %s | FileCheck %s

; InstCombine canonicalizes a single-index GEP `getelementptr T, ptr %p, i64 %i`
; into `getelementptr [sizeof(T) x i8], ptr %p, i64 %i`. A dynamic index then
; counts N-byte steps; when N covers whole lanes the alloca must still promote,
; with the index scaled by N / lane size rather than by N.

; [4 x i8] over [18 x i32] steps one lane: P[i] = L; P[i + 1] = R; v = P[k].
; The i8 GEP of +4 bytes on top of P[i] is the next lane.
; CHECK-LABEL: define void @lane_stride(
; CHECK:       alloca <18 x i32>
; CHECK-NOT:   alloca [18 x i32]
; CHECK:       [[I:%.*]] = trunc i64 %i to i32
; CHECK-NEXT:  add i32 0, [[I]]
; CHECK:       insertelement <18 x i32>
; CHECK-NOT:   add i32 4,
; CHECK:       add i32 1,
; CHECK:       insertelement <18 x i32>
; CHECK:       [[K:%.*]] = trunc i64 %k to i32
; CHECK-NEXT:  add i32 0, [[K]]
; CHECK:       extractelement <18 x i32>
define void @lane_stride(i64 %i, i64 %k, i32 %l, i32 %r, ptr addrspace(1) %out) {
  %a = alloca [18 x i32], align 4
  %p = getelementptr inbounds [4 x i8], ptr %a, i64 %i
  store i32 %l, ptr %p, align 4
  %q = getelementptr inbounds i8, ptr %p, i64 4
  store i32 %r, ptr %q, align 4
  %s = getelementptr inbounds [4 x i8], ptr %a, i64 %k
  %v = load i32, ptr %s, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; [8 x i8] over [16 x i32] steps two lanes.
; CHECK-LABEL: define void @two_lane_stride(
; CHECK:       alloca <16 x i32>
; CHECK-NOT:   alloca [16 x i32]
; CHECK:       [[I:%.*]] = trunc i64 %i to i32
; CHECK-NEXT:  [[LANE:%.*]] = mul i32 [[I]], 2
; CHECK-NEXT:  add i32 0, [[LANE]]
; CHECK:       insertelement <16 x i32>
define void @two_lane_stride(i64 %i, i32 %l, ptr addrspace(1) %out) {
  %a = alloca [16 x i32], align 4
  %p = getelementptr inbounds [8 x i8], ptr %a, i64 %i
  store i32 %l, ptr %p, align 4
  %v = load i32, ptr %a, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; GEP indices are signed: i8 -1 from lane 8 is lane 7, not lane 263.
; CHECK-LABEL: define i32 @narrow_negative_constant(
; CHECK:       alloca <16 x i32>
; CHECK-NOT:   alloca [16 x i32]
; CHECK:       insertelement <16 x i32> %{{.*}}, i32 42, i32 7{{$}}
; CHECK:       extractelement <16 x i32> %{{.*}}, i32 7{{$}}
define i32 @narrow_negative_constant() {
  %a = alloca [16 x i32], align 4
  %e7 = getelementptr inbounds [16 x i32], ptr %a, i64 0, i64 7
  store i32 42, ptr %e7, align 4
  %e8 = getelementptr inbounds [16 x i32], ptr %a, i64 0, i64 8
  %p = getelementptr inbounds [4 x i8], ptr %e8, i8 -1
  %v = load i32, ptr %p, align 4
  ret i32 %v
}

; A dynamic narrow index is sign-extended as well.
; CHECK-LABEL: define i32 @narrow_dynamic(
; CHECK:       alloca <16 x i32>
; CHECK-NOT:   alloca [16 x i32]
; CHECK-NOT:   zext i8 %i
; CHECK:       sext i8 %i to i32
; CHECK:       extractelement <16 x i32>
define i32 @narrow_dynamic(i8 %i) {
  %a = alloca [16 x i32], align 4
  %e7 = getelementptr inbounds [16 x i32], ptr %a, i64 0, i64 7
  store i32 42, ptr %e7, align 4
  %e8 = getelementptr inbounds [16 x i32], ptr %a, i64 0, i64 8
  %p = getelementptr inbounds [4 x i8], ptr %e8, i8 %i
  %v = load i32, ptr %p, align 4
  ret i32 %v
}

!igc.functions = !{!0, !3, !4, !5}
!0 = !{ptr @lane_stride, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @two_lane_stride, !1}
!4 = !{ptr @narrow_negative_constant, !1}
!5 = !{ptr @narrow_dynamic, !1}
