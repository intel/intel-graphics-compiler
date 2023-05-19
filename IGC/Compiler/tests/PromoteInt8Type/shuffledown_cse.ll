;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-promoteint8type --early-cse -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : shuffledown
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; PromoteInt8Type can create duplicated vector sources if promoted shuffledown
; intrinsics use common source. These should be cleared up with CSE pass and shuffles
; should use common source.
;
; Please note that shuffledown intrinsic is only expected to be promoted to i16 type
; if delta is not compilation-time constant.

define void @test_promote_with_cse(i8 addrspace(1)* %src, i32 %delta) {
; CHECK-LABEL: @test_promote_with_cse(
; CHECK:  entry:
; CHECK:    [[TMP8:%.*]] = load i8, i8 addrspace(1)* %src
; CHECK:    [[TMP16:%.*]] = sext i8 [[TMP8]] to i16
; CHECK:    [[VEC16_0:%.*]] = insertelement <2 x i16> undef, i16 [[TMP16]], i64 0
; CHECK:    [[VEC16_1:%.*]] = insertelement <2 x i16> [[VEC16_0]], i16 [[TMP16]], i64 1
; CHECK:    [[PROMOTE1:%.*]] = call i16 @llvm.genx.GenISA.simdShuffleDown.i16.v2i16(<2 x i16> [[VEC16_1]], i32 %delta)
; CHECK:    [[DELTA2:%.*]] = add i32 %delta, 3
; CHECK:    [[PROMOTE2:%.*]] = call i16 @llvm.genx.GenISA.simdShuffleDown.i16.v2i16(<2 x i16> [[VEC16_1]], i32 [[DELTA2]])
;
entry:
  %0 = load i8, i8 addrspace(1)* %src, align 1
  %assembled.vect = insertelement <2 x i8> undef, i8 %0, i32 0
  %assembled.vect2 = insertelement <2 x i8> %assembled.vect, i8 %0, i32 1
  %simdShuffleDown = call i8 @llvm.genx.GenISA.simdShuffleDown.i8.v2i8(<2 x i8> %assembled.vect2, i32 %delta)
  %delta2 = add i32 %delta, 3
  %simdShuffleDown2 = call i8 @llvm.genx.GenISA.simdShuffleDown.i8.v2i8(<2 x i8> %assembled.vect2, i32 %delta2)
  %result = add i8 %simdShuffleDown, %simdShuffleDown2
  store i8 %result, i8 addrspace(1)* %src, align 1
  ret void
}

declare i8 @llvm.genx.GenISA.simdShuffleDown.i8.v2i8(<2 x i8>, i32) #4
