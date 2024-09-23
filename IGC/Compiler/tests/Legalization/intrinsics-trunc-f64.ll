;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-legalization -S -dce < %s | FileCheck %s
; ------------------------------------------------
; Legalization: intrinsics
; ------------------------------------------------

; Checks legalization of trunc intrinsic
; for double type

define double @test_trunc(double %s1) {
; CHECK-LABEL: define double @test_trunc(
; CHECK-SAME: double [[S1:%.*]]) {
; CHECK:    [[TMP1:%.*]] = bitcast double [[S1]] to i64
; CHECK:    [[TMP2:%.*]] = lshr i64 [[TMP1]], 32
; CHECK:    [[TMP32:%.*]] = trunc i64 [[TMP2]] to i32
; CHECK:    [[TMP4:%.*]] = lshr i64 [[TMP1]], 52
; CHECK:    [[TMP5:%.*]] = trunc i64 [[TMP4]] to i32
; CHECK:    [[TMP6:%.*]] = and i32 [[TMP5]], 2047
; CHECK:    [[TMP7:%.*]] = sub nsw i32 1023, [[TMP6]]
; CHECK:    [[TMP8:%.*]] = add nsw i32 [[TMP7]], 52
; CHECK:    [[TMP9:%.*]] = add nsw i32 [[TMP7]], 20
; CHECK:    [[TMP10:%.*]] = icmp sgt i32 [[TMP8]], 32
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP10]], i32 32, i32 [[TMP8]]
; CHECK:    [[TMP12:%.*]] = icmp sgt i32 [[TMP9]], 20
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP12]], i32 20, i32 [[TMP9]]
; CHECK:    [[TMP14:%.*]] = icmp sgt i32 [[TMP11]], 0
; CHECK:    [[TMP15:%.*]] = select i1 [[TMP14]], i32 [[TMP11]], i32 0
; CHECK:    [[TMP16:%.*]] = icmp sgt i32 [[TMP13]], 0
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP16]], i32 [[TMP13]], i32 0
; CHECK:    [[TMP18:%.*]] = and i32 [[TMP15]], 31
; CHECK:    [[TMP19:%.*]] = shl i32 -1, [[TMP18]]
; CHECK:    [[TMP20:%.*]] = and i32 [[TMP17]], 31
; CHECK:    [[TMP21:%.*]] = shl i32 -1, [[TMP20]]
; CHECK:    [[TMP22:%.*]] = icmp ne i32 [[TMP15]], 32
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP22]], i32 [[TMP19]], i32 0
; CHECK:    [[TMP24:%.*]] = icmp ult i32 [[TMP6]], 1023
; CHECK:    [[MASKVALHIGH32BIT_0_I:%.*]] = select i1 [[TMP24]], i32 -2147483648, i32 [[TMP21]]
; CHECK:    [[MASKVALLOW32BIT_0_I:%.*]] = select i1 [[TMP24]], i32 0, i32 [[TMP23]]
; CHECK:    [[TMP25:%.*]] = trunc i64 [[TMP1]] to i32
; CHECK:    [[TMP26:%.*]] = and i32 [[MASKVALLOW32BIT_0_I]], [[TMP25]]
; CHECK:    [[TMP27:%.*]] = and i32 [[MASKVALHIGH32BIT_0_I]], [[TMP32]]
; CHECK:    [[TMP28:%.*]] = zext i32 [[TMP27]] to i64
; CHECK:    [[TMP29:%.*]] = shl nuw i64 [[TMP28]], 32
; CHECK:    [[TMP30:%.*]] = zext i32 [[TMP26]] to i64
; CHECK:    [[TMP31:%.*]] = or i64 [[TMP29]], [[TMP30]]
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP31]] to double
; CHECK:    ret double [[TMP3]]
;
  %1 = call double @llvm.trunc.f64(double %s1)
  ret double %1
}

declare double @llvm.trunc.f64(double)

!igc.functions = !{!0}

!0 = !{double (double)* @test_trunc, !1}
!1 = !{}
