;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-promoteint8type -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PromoteInt8Type : shuffledown
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Please note that shuffledown intrinsic is only expected to be promoted to i16 type
; if delta is not compilation-time constant.

define void @test_promote(i8 addrspace(1)* %src, i32 %delta) {
; CHECK-LABEL: @test_promote(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = load i8, i8 addrspace(1)* %src
; CHECK:    [[VEC8_0:%.*]] = insertelement <2 x i8> undef, i8 [[TMP0]], i32 0
; CHECK:    [[VEC8_1:%.*]] = insertelement <2 x i8> [[VEC8_0]], i8 [[TMP0]], i32 1
; CHECK:    [[TMP8_0:%.*]] = extractelement <2 x i8> [[VEC8_1:%.*]], i64 0
; CHECK:    [[TMP16_0:%.*]] = sext i8 [[TMP8_0]] to i16
; CHECK:    [[VEC16_0:%.*]] = insertelement <2 x i16> undef, i16 [[TMP16_0]], i64 0
; CHECK:    [[TMP8_1:%.*]] = extractelement <2 x i8> [[VEC8_1:%.*]], i64 1
; CHECK:    [[TMP16_1:%.*]] = sext i8 [[TMP8_1]] to i16
; CHECK:    [[VEC16_1:%.*]] = insertelement <2 x i16> [[VEC16_0]], i16 [[TMP16_1]], i64 1
; CHECK:    [[PROMOTE:%.*]] = call i16 @llvm.genx.GenISA.simdShuffleDown.i16.v2i16(<2 x i16> [[VEC16_1]], i32 %delta)
; CHECK:    [[TRUNC:%.*]] = trunc i16 [[PROMOTE]] to i8
; CHECK:    store i8 [[TRUNC]], i8 addrspace(1)* %src
; CHECK:    ret void
;
entry:
  %0 = load i8, i8 addrspace(1)* %src, align 1
  %assembled.vect = insertelement <2 x i8> undef, i8 %0, i32 0
  %assembled.vect2 = insertelement <2 x i8> %assembled.vect, i8 %0, i32 1
  %simdShuffleDown = call i8 @llvm.genx.GenISA.simdShuffleDown.i8.v2i8(<2 x i8> %assembled.vect2, i32 %delta)
  store i8 %simdShuffleDown, i8 addrspace(1)* %src, align 1
  ret void
}

declare i8 @llvm.genx.GenISA.simdShuffleDown.i8.v2i8(<2 x i8>, i32) #4
