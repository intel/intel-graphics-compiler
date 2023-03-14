;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --platformdg2 -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: dp4a
; ------------------------------------------------

define void @test_dp4(i8 %a, i8 %b, i8 %c, i8 %d, i8 %e, i8 %f, i8 %g, i8 %h, i32 %acc) {
; CHECK-LABEL: @test_dp4(
; CHECK:    [[TMP1:%.*]] = sext i8 [[A:%.*]] to i32
; CHECK:    [[TMP2:%.*]] = sext i8 [[B:%.*]] to i32
; CHECK:    [[TMP3:%.*]] = mul nsw i32 [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = sext i8 [[C:%.*]] to i32
; CHECK:    [[TMP5:%.*]] = sext i8 [[D:%.*]] to i32
; CHECK:    [[TMP6:%.*]] = mul nsw i32 [[TMP4]], [[TMP5]]
; CHECK:    [[TMP7:%.*]] = add nsw i32 [[TMP3]], [[TMP6]]
; CHECK:    [[TMP8:%.*]] = sext i8 [[E:%.*]] to i32
; CHECK:    [[TMP9:%.*]] = sext i8 [[F:%.*]] to i32
; CHECK:    [[TMP10:%.*]] = mul nsw i32 [[TMP8]], [[TMP9]]
; CHECK:    [[TMP11:%.*]] = add nsw i32 [[TMP7]], [[TMP10]]
; CHECK:    [[TMP12:%.*]] = sext i8 [[G:%.*]] to i32
; CHECK:    [[TMP13:%.*]] = sext i8 [[H:%.*]] to i32
; CHECK:    [[TMP14:%.*]] = mul nsw i32 [[TMP12]], [[TMP13]]
; CHECK:    [[TMP15:%.*]] = add nsw i32 [[TMP11]], [[TMP14]]
; CHECK:    [[TMP16:%.*]] = insertelement <4 x i8> undef, i8 [[A]], i64 0
; CHECK:    [[TMP17:%.*]] = insertelement <4 x i8> undef, i8 [[B]], i64 0
; CHECK:    [[TMP18:%.*]] = insertelement <4 x i8> [[TMP16]], i8 [[C]], i64 1
; CHECK:    [[TMP19:%.*]] = insertelement <4 x i8> [[TMP17]], i8 [[D]], i64 1
; CHECK:    [[TMP20:%.*]] = insertelement <4 x i8> [[TMP18]], i8 [[E]], i64 2
; CHECK:    [[TMP21:%.*]] = insertelement <4 x i8> [[TMP19]], i8 [[F]], i64 2
; CHECK:    [[TMP22:%.*]] = insertelement <4 x i8> [[TMP20]], i8 [[G]], i64 3
; CHECK:    [[TMP23:%.*]] = insertelement <4 x i8> [[TMP21]], i8 [[H]], i64 3
; CHECK:    [[TMP24:%.*]] = bitcast <4 x i8> [[TMP22]] to i32
; CHECK:    [[TMP25:%.*]] = bitcast <4 x i8> [[TMP23]] to i32
; CHECK:    [[TMP26:%.*]] = call i32 @llvm.genx.GenISA.dp4a.ss.i32(i32 0, i32 [[TMP24]], i32 [[TMP25]])
; CHECK:    [[TMP27:%.*]] = add nsw i32 [[ACC:%.*]], [[TMP26]]
; CHECK:    call void @use.i32(i32 [[TMP27]])
;
  %1 = sext i8 %a to i32
  %2 = sext i8 %b to i32
  %3 = mul nsw i32 %1, %2
  %4 = sext i8 %c to i32
  %5 = sext i8 %d to i32
  %6 = mul nsw i32 %4, %5
  %7 = add nsw i32 %3, %6
  %8 = sext i8 %e to i32
  %9 = sext i8 %f to i32
  %10 = mul nsw i32 %8, %9
  %11 = add nsw i32 %7, %10
  %12 = sext i8 %g to i32
  %13 = sext i8 %h to i32
  %14 = mul nsw i32 %12, %13
  %15 = add nsw i32 %11, %14
  %16 = add nsw i32 %acc, %15
;  store i32 %add13.i, i32 addrspace(1)* %18, align 4
  call void @use.i32(i32 %16)
  ret void
}

declare void @use.i32(i32)
