;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-type-legalizer -verify -S %s -o - | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define i8 @f1(i8 %a) #0 {
  %b = trunc i8 %a to i3
  %c = call i3 @llvm.bitreverse.i3(i3 %b)
  %r = zext i3 %c to i8
  ret i8 %r
}

; CHECK-LABEL: define i8 @f1
; CHECK: %1 = and i8 %a, 7
; CHECK: %2 = call i8 @llvm.bitreverse.i8(i8 %1)
; CHECK: %3 = lshr i8 %2, 5
; CHECK: ret i8 %3

define i8 @f2(i8 %a) #0 {
  %b = trunc i8 %a to i4
  %c = call i4 @llvm.bitreverse.i4(i4 %b)
  %r = zext i4 %c to i8
  ret i8 %r
}

; CHECK-LABEL: define i8 @f2
; CHECK: %1 = and i8 %a, 15
; CHECK: %2 = call i8 @llvm.bitreverse.i8(i8 %1)
; CHECK: %3 = lshr i8 %2, 4
; CHECK: ret i8 %3

define i32 @f3(i32 %a) #0 {
  %b = zext i32 %a to i33
  %c = call i33 @llvm.bitreverse.i33(i33 %b)
  %r = trunc i33 %c to i32
  ret i32 %r
}

; CHECK-LABEL: define i32 @f3
; CHECK: %b.promote = zext i32 %a to i64
; CHECK: %1 = call i64 @llvm.bitreverse.i64(i64 %b.promote)
; CHECK: %2 = lshr i64 %1, 31
; CHECK: %r = trunc i64 %2 to i32
; CHECK: ret i32 %r

define <4 x i8> @f4(<4 x i8> %a) #0 {
  %b = trunc <4 x i8> %a to <4 x i5>
  %c = call <4 x i5> @llvm.bitreverse.v4i5(<4 x i5> %b)
  %r = zext <4 x i5> %c to <4 x i8>
  ret <4 x i8> %r
}

; CHECK-LABEL: define <4 x i8> @f4
; CHECK: %1 = and <4 x i8> %a, <i8 31, i8 31, i8 31, i8 31>
; CHECK: %2 = call <4 x i8> @llvm.bitreverse.v4i8(<4 x i8> %1)
; CHECK: %3 = lshr <4 x i8> %2, <i8 3, i8 3, i8 3, i8 3>
; CHECK: ret <4 x i8> %3

define i32 @f5(i64 %a) #0 {
  %b = trunc i64 %a to i4
  %c = call i4 @llvm.bitreverse.i4(i4 %b)
  %r = zext i4 %c to i32
  ret i32 %r
}

; CHECK-LABEL: define i32 @f5
; CHECK: %b.promote = trunc i64 %a to i8
; CHECK: %1 = and i8 %b.promote, 15
; CHECK: %2 = call i8 @llvm.bitreverse.i8(i8 %1)
; CHECK: %3 = lshr i8 %2, 4
; CHECK: %.zext = and i8 %3, 15
; CHECK: %r = zext i8 %.zext to i32
; CHECK: ret i32 %r

declare i3 @llvm.bitreverse.i3(i3) #1

declare i4 @llvm.bitreverse.i4(i4) #1

declare i33 @llvm.bitreverse.i33(i33) #1

declare <4 x i5> @llvm.bitreverse.v4i5(<4 x i5>) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone speculatable }   

!igc.functions = !{!0, !1, !2, !3, !4}

!0 = !{i8 (i8)* @f1, !5}
!1 = !{i8 (i8)* @f2, !5}
!2 = !{i32 (i32)* @f3, !5}
!3 = !{<4 x i8> (<4 x i8>)* @f4, !5}
!4 = !{i32 (i64)* @f5, !5}
!5 = !{}
