;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S %s  | FileCheck %s

define void @test() {
  ; CHECK: [[TMP1:%.*]] = alloca [16 x i32]
  ; CHECK: [[PTR0:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 0
  ; CHECK: [[PTR1:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 1
  ; CHECK: [[PTR2:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 2
  ; CHECK: [[PTR3:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 3
  ; CHECK: [[PTR4:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 4
  ; CHECK: [[PTR5:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 5
  ; CHECK: [[PTR6:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 6
  ; CHECK: [[PTR7:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 7
  ; CHECK: [[PTR8:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 8
  ; CHECK: [[PTR9:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 9
  ; CHECK: [[PTR10:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 10
  ; CHECK: [[PTR11:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 11
  ; CHECK: [[PTR12:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 12
  ; CHECK: [[PTR13:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 13
  ; CHECK: [[PTR14:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 14
  ; CHECK: [[PTR15:%.*]] = getelementptr inbounds [4 x i8], ptr [[TMP1]], i64 15
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

  ; CHECK: [[TMP2:%.*]] = load i32, ptr [[PTR0]]
  ; CHECK: store i32 [[TMP2]], ptr [[PTR0]]
  ; CHECK: [[TMP3:%.*]] = load i32, ptr [[PTR1]]
  ; CHECK: store i32 [[TMP3]], ptr [[PTR1]]
  ; CHECK: [[TMP4:%.*]] = load i32, ptr [[PTR2]]
  ; CHECK: store i32 [[TMP4]], ptr [[PTR2]]
  ; CHECK: [[TMP5:%.*]] = load i32, ptr [[PTR3]]
  ; CHECK: store i32 [[TMP5]], ptr [[PTR3]]
  ; CHECK: [[TMP6:%.*]] = load i32, ptr [[PTR4]]
  ; CHECK: store i32 [[TMP6]], ptr [[PTR4]]
  ; CHECK: [[TMP7:%.*]] = load i32, ptr [[PTR5]]
  ; CHECK: store i32 [[TMP7]], ptr [[PTR5]]
  ; CHECK: [[TMP8:%.*]] = load i32, ptr [[PTR6]]
  ; CHECK: store i32 [[TMP8]], ptr [[PTR6]]
  ; CHECK: [[TMP9:%.*]] = load i32, ptr [[PTR7]]
  ; CHECK: store i32 [[TMP9]], ptr [[PTR7]]
  ; CHECK: [[TMP10:%.*]] = load i32, ptr [[PTR8]]
  ; CHECK: store i32 [[TMP10]], ptr [[PTR8]]
  ; CHECK: [[TMP11:%.*]] = load i32, ptr [[PTR9]]
  ; CHECK: store i32 [[TMP11]], ptr [[PTR9]]
  ; CHECK: [[TMP12:%.*]] = load i32, ptr [[PTR10]]
  ; CHECK: store i32 [[TMP12]], ptr [[PTR10]]
  ; CHECK: [[TMP13:%.*]] = load i32, ptr [[PTR11]]
  ; CHECK: store i32 [[TMP13]], ptr [[PTR11]]
  ; CHECK: [[TMP14:%.*]] = load i32, ptr [[PTR12]]
  ; CHECK: store i32 [[TMP14]], ptr [[PTR12]]
  ; CHECK: [[TMP15:%.*]] = load i32, ptr [[PTR13]]
  ; CHECK: store i32 [[TMP15]], ptr [[PTR13]]
  ; CHECK: [[TMP16:%.*]] = load i32, ptr [[PTR14]]
  ; CHECK: store i32 [[TMP16]], ptr [[PTR14]]
  ; CHECK: [[TMP17:%.*]] = load i32, ptr [[PTR15]]
  ; CHECK: store i32 [[TMP17]], ptr [[PTR15]]
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

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
