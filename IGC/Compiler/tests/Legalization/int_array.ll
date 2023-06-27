;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-int-type-legalizer -S < %s | FileCheck %s

; Test checks arrays of illegal integers

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; CHECK-LABEL: @foo1(
; CHECK:    [[V2:%.*]] = extractelement <2 x i8> %0, i32 0
; CHECK:    [[V3:%.*]] = insertelement <12 x i8> undef, i8 [[V2]], i32 0
; CHECK:    [[V4:%.*]] = insertelement <12 x i8> [[V3]], i8 0, i32 1
; CHECK:    [[V5:%.*]] = insertelement <12 x i8> [[V4]], i8 0, i32 2
; CHECK:    [[V6:%.*]] = insertelement <12 x i8> [[V5]], i8 0, i32 3
; CHECK:    [[V7:%.*]] = insertelement <12 x i8> [[V6]], i8 0, i32 4
; CHECK:    [[V8:%.*]] = insertelement <12 x i8> [[V7]], i8 0, i32 5
; CHECK:    [[V9:%.*]] = extractelement <2 x i8> %0, i32 1
; CHECK:    [[V10:%.*]] = insertelement <12 x i8> [[V8]], i8 %9, i32 6
; CHECK:    [[V11:%.*]] = insertelement <12 x i8> [[V10]], i8 0, i32 7
; CHECK:    [[V12:%.*]] = insertelement <12 x i8> [[V11]], i8 0, i32 8
; CHECK:    [[V13:%.*]] = insertelement <12 x i8> [[V12]], i8 0, i32 9
; CHECK:    [[V14:%.*]] = insertelement <12 x i8> [[V13]], i8 0, i32 10
; CHECK:    [[V15:%.*]] = insertelement <12 x i8> [[V14]], i8 0, i32 11
; CHECK:    [[V16:%.*]] = bitcast <12 x i8> [[V15]] to <3 x i32>
; CHECK:    ret <3 x i32> [[V16]]
;
define <3 x i32> @foo1(<2 x i8>) #0 {
  %2 = zext <2 x i8> %0 to <2 x i48>
  %3 = bitcast <2 x i48> %2 to <3 x i32>
  ret <3 x i32> %3
}

; CHECK-LABEL: @foo2(
; CHECK:    [[V2:%.*]] = bitcast <2 x i32> %0 to <4 x i16>
; CHECK:    [[V3:%.*]] = extractelement <4 x i16> [[V2]], i32 0
; CHECK:    [[V4:%.*]] = insertelement <6 x i16> undef, i16 [[V3]], i32 0
; CHECK:    [[V5:%.*]] = extractelement <4 x i16> [[V2]], i32 1
; CHECK:    [[V6:%.*]] = insertelement <6 x i16> [[V4]], i16 [[V5]], i32 1
; CHECK:    [[V7:%.*]] = insertelement <6 x i16> [[V6]], i16 0, i32 2
; CHECK:    [[V8:%.*]] = extractelement <4 x i16> [[V2]], i32 2
; CHECK:    [[V9:%.*]] = insertelement <6 x i16> [[V7]], i16 [[V8]], i32 3
; CHECK:    [[V10:%.*]] = extractelement <4 x i16> [[V2]], i32 3
; CHECK:    [[V11:%.*]] = insertelement <6 x i16> [[V9]], i16 [[V10]], i32 4
; CHECK:    [[V12:%.*]] = insertelement <6 x i16> [[V11]], i16 0, i32 5
; CHECK:    ret <6 x i16> [[V12]]
;
define <6 x i16> @foo2(<2 x i32>) #0 {
  %2 = zext <2 x i32> %0 to <2 x i48>
  %3 = bitcast <2 x i48> %2 to <6 x i16>
  ret <6 x i16> %3
}

; CHECK-LABEL: @foo3(
; CHECK:    [[V2:%.*]] = bitcast <3 x i32> %0 to <6 x i16>
; CHECK:    [[V3:%.*]] = extractelement <6 x i16> [[V2]], i32 0
; CHECK:    [[V4:%.*]] = insertelement <2 x i16> undef, i16 [[V3]], i32 0
; CHECK:    [[V5:%.*]] = extractelement <6 x i16> %2, i32 3
; CHECK:    [[V6:%.*]] = insertelement <2 x i16> [[V4]], i16 [[V5]], i32 1
; CHECK:    ret <2 x i16> [[V6]]
;
define <2 x i16> @foo3(<3 x i32>) #0 {
  %2 = bitcast <3 x i32> %0 to <2 x i48>
  %3 = trunc <2 x i48> %2 to <2 x i16>
  ret <2 x i16> %3
}

; CHECK-LABEL: @foo4(
; CHECK:    [[V2:%.*]] = bitcast <12 x i8> %0 to <6 x i16>
; CHECK:    [[V3:%.*]] = extractelement <6 x i16> [[V2]], i32 0
; CHECK:    [[V4:%.*]] = insertelement <4 x i16> undef, i16 [[V3]], i32 0
; CHECK:    [[V5:%.*]] = extractelement <6 x i16> [[V2]], i32 1
; CHECK:    [[V6:%.*]] = insertelement <4 x i16> [[V4]], i16 [[V5]], i32 1
; CHECK:    [[V7:%.*]] = extractelement <6 x i16> [[V2]], i32 3
; CHECK:    [[V8:%.*]] = insertelement <4 x i16> [[V6]], i16 [[V7]], i32 2
; CHECK:    [[V9:%.*]] = extractelement <6 x i16> [[V2]], i32 4
; CHECK:    [[V10:%.*]] = insertelement <4 x i16> [[V8]], i16 [[V9]], i32 3
; CHECK:    [[V11:%.*]] = bitcast <4 x i16> [[V10]] to <2 x i32>
; CHECK:    ret <2 x i32> [[V11]]
;
define <2 x i32> @foo4(<12 x i8>) #0 {
  %2 = bitcast <12 x i8> %0 to <2 x i48>
  %3 = trunc <2 x i48> %2 to <2 x i32>
  ret <2 x i32> %3
}

attributes #0 = { convergent noinline nounwind }

!igc.functions = !{!1, !2, !3, !4}

!0 = !{}
!1 = !{<3 x i32> (<2 x i8>)* @foo1, !0}
!2 = !{<6 x i16> (<2 x i32>)* @foo2, !0}
!3 = !{<2 x i16> (<3 x i32>)* @foo3, !0}
!4 = !{<2 x i32> (<12 x i8>)* @foo4, !0}
