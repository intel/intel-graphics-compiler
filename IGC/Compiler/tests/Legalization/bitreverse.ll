;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-legalization -verify -S %s -o - | FileCheck %s

define i8 @f1(i8 %a) #0 {
  %r = call i8 @llvm.bitreverse.i8(i8 %a)
  ret i8 %r
}

; CHECK-LABEL: define i8 @f1
; CHECK: %1 = zext i8 %a to i32
; CHECK: %2 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %1)
; CHECK: %3 = lshr i32 %2, 24
; CHECK: %4 = trunc i32 %3 to i8
; CHECK: ret i8 %4

define i64 @f2(i64 %a) #0 {
  %r = call i64 @llvm.bitreverse.i64(i64 %a)
  ret i64 %r
}

; CHECK-LABEL: define i64 @f2
; CHECK: %1 = trunc i64 %a to i32
; CHECK: %2 = lshr i64 %a, 32
; CHECK: %3 = trunc i64 %2 to i32
; CHECK: %4 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %1)
; CHECK: %5 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %3)
; CHECK: %6 = zext i32 %4 to i64
; CHECK: %7 = shl i64 %6, 32
; CHECK: %8 = zext i32 %5 to i64
; CHECK: %9 = or i64 %7, %8
; CHECK: ret i64 %9

define <4 x i32> @f3(<4 x i32> %a) #0 {
  %r = call <4 x i32> @llvm.bitreverse.v4i32(<4 x i32> %a)
  ret <4 x i32> %r
}

; CHECK-LABEL: define <4 x i32> @f3
; CHECK: %1 = extractelement <4 x i32> %a, i32 0
; CHECK: %2 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %1)
; CHECK: %3 = insertelement <4 x i32> undef, i32 %2, i32 0
; CHECK: %4 = extractelement <4 x i32> %a, i32 1
; CHECK: %5 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %4)
; CHECK: %6 = insertelement <4 x i32> %3, i32 %5, i32 1
; CHECK: %7 = extractelement <4 x i32> %a, i32 2
; CHECK: %8 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %7)
; CHECK: %9 = insertelement <4 x i32> %6, i32 %8, i32 2
; CHECK: %10 = extractelement <4 x i32> %a, i32 3
; CHECK: %11 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %10)
; CHECK: %12 = insertelement <4 x i32> %9, i32 %11, i32 3
; CHECK: ret <4 x i32> %12

define i4 @f4(i4 %a) #0 {
  %r = call i4 @llvm.bitreverse.i4(i4 %a)
  ret i4 %r
}

; CHECK-LABEL: define i4 @f4
; CHECK: %1 = zext i4 %a to i32
; CHECK: %2 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %1)
; CHECK: %3 = lshr i32 %2, 28
; CHECK: %4 = trunc i32 %3 to i4
; CHECK: ret i4 %4

define i44 @f5(i44 %a) #0 {
  %r = call i44 @llvm.bitreverse.i44(i44 %a)
  ret i44 %r
}

; CHECK-LABEL: define i44 @f5
; CHECK: %1 = zext i44 %a to i64
; CHECK: %2 = trunc i64 %1 to i32
; CHECK: %3 = lshr i64 %1, 32
; CHECK: %4 = trunc i64 %3 to i32
; CHECK: %5 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %2)
; CHECK: %6 = call i32 @llvm.genx.GenISA.bfrev.i32(i32 %4)
; CHECK: %7 = zext i32 %5 to i64
; CHECK: %8 = shl i64 %7, 12
; CHECK: %9 = zext i32 %6 to i64
; CHECK: %10 = lshr i64 %9, 20
; CHECK: %11 = or i64 %8, %10
; CHECK: %12 = trunc i64 %11 to i44
; CHECK: ret i44 %12

declare i4 @llvm.bitreverse.i4(i4) #1

declare i8 @llvm.bitreverse.i8(i8) #1

declare i44 @llvm.bitreverse.i44(i44) #1

declare i64 @llvm.bitreverse.i64(i64) #1

declare <4 x i32> @llvm.bitreverse.v4i32(<4 x i32>) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone speculatable }

!igc.functions = !{!0, !1, !2, !4, !5}

!0 = !{i8 (i8)* @f1, !3}
!1 = !{i64 (i64)* @f2, !3}
!2 = !{<4 x i32> (<4 x i32>)* @f3, !3}
!3 = !{}
!4 = !{i4 (i4)* @f4, !3}
!5 = !{i44 (i44)* @f5, !3}
