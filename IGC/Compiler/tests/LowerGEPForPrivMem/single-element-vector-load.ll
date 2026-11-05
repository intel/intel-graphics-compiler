;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; A <1 x T> load from a promoted alloca must keep its vector type.

; Element as wide as the promoted lane.
define void @same_width(ptr %output, i32 %idx) {
; CHECK-LABEL: @same_width(
; CHECK: [[ELEM:%.*]] = extractelement <2 x float> {{%.*}}, i32 {{%.*}}
; CHECK: [[VEC:%.*]] = insertelement <1 x float> poison, float [[ELEM]], i32 0
; CHECK: store <1 x float> [[VEC]], ptr %output
  %private = alloca [2 x <1 x float>], align 4, !uniform !4
  %elem.ptr = getelementptr <1 x float>, ptr %private, i32 %idx
  %loaded = load <1 x float>, ptr %elem.ptr, align 4
  store <1 x float> %loaded, ptr %output, align 4
  ret void
}

; Element narrower than the promoted lane.
define void @narrower_element(ptr %output) {
; CHECK-LABEL: @narrower_element(
; CHECK: [[LANE:%.*]] = extractelement <2 x i32> {{%.*}}, i32 0
; CHECK: [[PACK:%.*]] = insertelement <1 x i32> poison, i32 [[LANE]], i32 0
; CHECK: [[INT:%.*]] = bitcast <1 x i32> [[PACK]] to i32
; CHECK: [[ELEM:%.*]] = trunc i32 [[INT]] to i16
; CHECK: [[VEC:%.*]] = bitcast i16 [[ELEM]] to <1 x i16>
; CHECK: store <1 x i16> [[VEC]], ptr %output
  %private = alloca [2 x i32], align 4, !uniform !4
  %elem.ptr = getelementptr <1 x i16>, ptr %private, i32 0
  %loaded = load <1 x i16>, ptr %elem.ptr, align 2
  store <1 x i16> %loaded, ptr %output, align 2
  ret void
}

!igc.functions = !{!0, !5}

!0 = !{ptr @same_width, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{i1 true}
!5 = !{ptr @narrower_element, !1}
