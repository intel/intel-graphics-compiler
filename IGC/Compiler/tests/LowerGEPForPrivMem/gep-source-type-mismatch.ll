;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S %s  | FileCheck %s

; A byte-array GEP `getelementptr [N x i8], ptr %p, i64 %i` steps N bytes per index.
; When N covers whole lanes of the alloca element, the alloca promotes and each index
; becomes a lane index. A stride that splits a lane, a lane wider than the stride and
; an access narrower than the lane must still block promotion.

; [4 x i8] over [16 x i32]: index k is lane k.
; CHECK-LABEL: define void @test(
; CHECK: alloca <16 x i32>
; CHECK-NOT: alloca [16 x i32]
; CHECK: extractelement <16 x i32> %{{.*}}, i32 0{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 0{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 1{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 1{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 2{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 2{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 3{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 3{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 4{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 4{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 5{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 5{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 6{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 6{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 7{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 7{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 8{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 8{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 9{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 9{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 10{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 10{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 11{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 11{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 12{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 12{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 13{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 13{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 14{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 14{{$}}
; CHECK: extractelement <16 x i32> %{{.*}}, i32 15{{$}}
; CHECK: insertelement <16 x i32> %{{.*}}, i32 %{{.*}}, i32 15{{$}}
define void @test() {
  %a = alloca [16 x i32], align 4
  %ptr0 = getelementptr inbounds [4 x i8], ptr %a, i64 0
  %ptr1 = getelementptr inbounds [4 x i8], ptr %a, i64 1
  %ptr2 = getelementptr inbounds [4 x i8], ptr %a, i64 2
  %ptr3 = getelementptr inbounds [4 x i8], ptr %a, i64 3
  %ptr4 = getelementptr inbounds [4 x i8], ptr %a, i64 4
  %ptr5 = getelementptr inbounds [4 x i8], ptr %a, i64 5
  %ptr6 = getelementptr inbounds [4 x i8], ptr %a, i64 6
  %ptr7 = getelementptr inbounds [4 x i8], ptr %a, i64 7
  %ptr8 = getelementptr inbounds [4 x i8], ptr %a, i64 8
  %ptr9 = getelementptr inbounds [4 x i8], ptr %a, i64 9
  %ptr10 = getelementptr inbounds [4 x i8], ptr %a, i64 10
  %ptr11 = getelementptr inbounds [4 x i8], ptr %a, i64 11
  %ptr12 = getelementptr inbounds [4 x i8], ptr %a, i64 12
  %ptr13 = getelementptr inbounds [4 x i8], ptr %a, i64 13
  %ptr14 = getelementptr inbounds [4 x i8], ptr %a, i64 14
  %ptr15 = getelementptr inbounds [4 x i8], ptr %a, i64 15

  %l1 = load i32, ptr %ptr0
  store i32 %l1, ptr %ptr0
  %l2 = load i32, ptr %ptr1
  store i32 %l2, ptr %ptr1
  %l3 = load i32, ptr %ptr2
  store i32 %l3, ptr %ptr2
  %l4 = load i32, ptr %ptr3
  store i32 %l4, ptr %ptr3
  %l5 = load i32, ptr %ptr4
  store i32 %l5, ptr %ptr4
  %l6 = load i32, ptr %ptr5
  store i32 %l6, ptr %ptr5
  %l7 = load i32, ptr %ptr6
  store i32 %l7, ptr %ptr6
  %l8 = load i32, ptr %ptr7
  store i32 %l8, ptr %ptr7
  %l9 = load i32, ptr %ptr8
  store i32 %l9, ptr %ptr8
  %l10 = load i32, ptr %ptr9
  store i32 %l10, ptr %ptr9
  %l11 = load i32, ptr %ptr10
  store i32 %l11, ptr %ptr10
  %l12 = load i32, ptr %ptr11
  store i32 %l12, ptr %ptr11
  %l13 = load i32, ptr %ptr12
  store i32 %l13, ptr %ptr12
  %l14 = load i32, ptr %ptr13
  store i32 %l14, ptr %ptr13
  %l15 = load i32, ptr %ptr14
  store i32 %l15, ptr %ptr14
  %l16 = load i32, ptr %ptr15
  store i32 %l16, ptr %ptr15

  ret void
}

; [2 x i8] over [16 x i32]: index 1 is byte 2, inside lane 0.
; CHECK-LABEL: define void @split_lane(
; CHECK: alloca [16 x i32]
; CHECK: getelementptr inbounds [2 x i8]
define void @split_lane() {
  %a = alloca [16 x i32], align 4
  %p = getelementptr inbounds [2 x i8], ptr %a, i64 1
  %l = load i32, ptr %p, align 2
  store i32 %l, ptr %a, align 4
  ret void
}

; [4 x i8] over [8 x i64]: a 4-byte step is half a lane.
; CHECK-LABEL: define void @wide_lane(
; CHECK: alloca [8 x i64]
; CHECK: getelementptr inbounds [4 x i8]
define void @wide_lane() {
  %a = alloca [8 x i64], align 8
  %p = getelementptr inbounds [4 x i8], ptr %a, i64 3
  %l = load i64, ptr %p, align 4
  store i64 %l, ptr %a, align 8
  ret void
}

; [4 x i8] over [16 x i32] with an i16 access: the access covers half a lane.
; CHECK-LABEL: define void @narrow_access(
; CHECK: alloca [16 x i32]
; CHECK: getelementptr inbounds [4 x i8]
define void @narrow_access() {
  %a = alloca [16 x i32], align 4
  %p = getelementptr inbounds [4 x i8], ptr %a, i64 3
  %l = load i16, ptr %p, align 4
  store i16 %l, ptr %a, align 4
  ret void
}

!igc.functions = !{!0, !3, !4, !5}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @split_lane, !1}
!4 = !{ptr @wide_lane, !1}
!5 = !{ptr @narrow_access, !1}
