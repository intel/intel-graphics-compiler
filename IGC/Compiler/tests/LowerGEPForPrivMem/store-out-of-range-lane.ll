;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

define void @past_end(i32 %v) {
; CHECK-LABEL: @past_end(
; CHECK:       [[ALLOCA:%.*]] = alloca <4 x i32>
; CHECK:       [[WHOLE:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NOT:   insertelement
; CHECK:       store <4 x i32> [[WHOLE]], ptr [[ALLOCA]]
; CHECK:       ret void
  %a = alloca [4 x i32], align 4
  %p = getelementptr [4 x i32], ptr %a, i64 0, i64 17
  store i32 %v, ptr %p, align 4
  ret void
}

define void @before_begin(i32 %v) {
; CHECK-LABEL: @before_begin(
; CHECK:       [[ALLOCA:%.*]] = alloca <4 x i32>
; CHECK:       [[WHOLE:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NOT:   insertelement
; CHECK:       store <4 x i32> [[WHOLE]], ptr [[ALLOCA]]
; CHECK:       ret void
  %a = alloca [4 x i32], align 4
  %p = getelementptr [4 x i32], ptr %a, i64 0, i64 -1
  store i32 %v, ptr %p, align 4
  ret void
}

define void @straddles_end(<3 x i32> %v) {
; CHECK-LABEL: @straddles_end(
; CHECK:       [[ALLOCA:%.*]] = alloca <6 x i32>
; CHECK:       [[WHOLE:%.*]] = load <6 x i32>, ptr [[ALLOCA]]
; CHECK:       [[INS3:%.*]] = insertelement <6 x i32> [[WHOLE]], i32 {{%.*}}, i32 3
; CHECK:       [[INS4:%.*]] = insertelement <6 x i32> [[INS3]], i32 {{%.*}}, i32 4
; CHECK:       [[INS5:%.*]] = insertelement <6 x i32> [[INS4]], i32 {{%.*}}, i32 5
; CHECK-NEXT:  store <6 x i32> [[INS5]], ptr [[ALLOCA]]
; CHECK-NEXT:  ret void
  %a = alloca [6 x i32], align 4
  %p = getelementptr [6 x i32], ptr %a, i64 0, i64 3
  %w = shufflevector <3 x i32> %v, <3 x i32> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 poison>
  store <4 x i32> %w, ptr %p, align 16
  ret void
}

define void @fits_at_end(<2 x i32> %v) {
; CHECK-LABEL: @fits_at_end(
; CHECK:       [[ALLOCA:%.*]] = alloca <4 x i32>
; CHECK:       [[WHOLE:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK:       [[INS2:%.*]] = insertelement <4 x i32> [[WHOLE]], i32 {{%.*}}, i32 2
; CHECK:       [[INS3:%.*]] = insertelement <4 x i32> [[INS2]], i32 {{%.*}}, i32 3
; CHECK-NEXT:  store <4 x i32> [[INS3]], ptr [[ALLOCA]]
; CHECK-NEXT:  ret void
  %a = alloca [4 x i32], align 4
  %p = getelementptr [4 x i32], ptr %a, i64 0, i64 2
  store <2 x i32> %v, ptr %p, align 4
  ret void
}

!igc.functions = !{!0, !3, !4, !5}

!0 = !{ptr @past_end, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @before_begin, !1}
!4 = !{ptr @straddles_end, !1}
!5 = !{ptr @fits_at_end, !1}
