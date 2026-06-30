;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-int-type-legalizer -S < %s | FileCheck %s

; Shl legalization for an illegal integer wider than the largest legal int.
; The datalayout below has n8:16:32, so the largest legal integer is 32 bits and
; i38 is decomposed into <2 x i32> (quotient > 1). Before the fix this hit the
; unhandled 'default' case in legalizeBinaryOperator and left NewInst null,
; asserting in debug ("... opcode:25") and null-deref segfaulting in release.
;
; This is the pattern from the original repro: SCEV/IndVarSimplify narrows a loop
; index (index * 64) to i38, i.e. shl i38 %x, 6.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

; Shift amount not a multiple of the promoted element width (32): each result
; element mixes two source elements (low part shifted up, OR'ed with the carry
; from the previous element shifted down). shl i38 %a, 6 is exactly index * 64.
define i38 @test_shl_i38(i38 %a) {
; CHECK-LABEL: @test_shl_i38(
; CHECK:    [[Z:%.*]] = zext i38 %a to i64
; CHECK:    [[VEC:%.*]] = bitcast i64 [[Z]] to <2 x i32>
; CHECK:    [[E0:%.*]] = extractelement <2 x i32> [[VEC]], i64 0
; CHECK:    [[LO:%.*]] = shl i32 [[E0]], 6
; CHECK:    [[LORES:%.*]] = or i32 [[LO]], 0
; CHECK:    [[INS0:%.*]] = insertelement <2 x i32> undef, i32 [[LORES]], i64 0
; CHECK:    [[E0B:%.*]] = extractelement <2 x i32> [[VEC]], i64 0
; CHECK:    [[CARRY:%.*]] = lshr i32 [[E0B]], 26
; CHECK:    [[E1:%.*]] = extractelement <2 x i32> [[VEC]], i64 1
; CHECK:    [[HI:%.*]] = shl i32 [[E1]], 6
; CHECK:    [[HIRES:%.*]] = or i32 [[HI]], [[CARRY]]
; CHECK:    [[INS1:%.*]] = insertelement <2 x i32> [[INS0]], i32 [[HIRES]], i64 1
; CHECK:    [[BC:%.*]] = bitcast <2 x i32> [[INS1]] to i64
; CHECK:    [[RES:%.*]] = trunc i64 [[BC]] to i38
; CHECK:    ret i38 [[RES]]

  %r = shl i38 %a, 6
  ret i38 %r
}

; Shift amount is a multiple of the promoted element width (32): only whole
; elements move, zeros are shifted in from below.
define i38 @test_shl_i38_whole(i38 %a) {
; CHECK-LABEL: @test_shl_i38_whole(
; CHECK:    [[Z:%.*]] = zext i38 %a to i64
; CHECK:    [[VEC:%.*]] = bitcast i64 [[Z]] to <2 x i32>
; CHECK:    [[E0:%.*]] = extractelement <2 x i32> [[VEC]], i64 0
; CHECK:    [[INS:%.*]] = insertelement <2 x i32> <i32 0, i32 undef>, i32 [[E0]], i64 1
; CHECK:    [[BC:%.*]] = bitcast <2 x i32> [[INS]] to i64
; CHECK:    [[RES:%.*]] = trunc i64 [[BC]] to i38
; CHECK:    ret i38 [[RES]]

  %r = shl i38 %a, 32
  ret i38 %r
}
