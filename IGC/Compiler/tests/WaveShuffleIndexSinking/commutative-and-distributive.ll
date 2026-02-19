;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; Test distributive property for commutative mul instruction, and also test unhoistable add with constant operand since add does not distribute over mul
; However, without any preceding anchor, add i32 %ws, 1 is hoistable since it is just an add with a constant
; ------------------------------------------------

define void @test_compare_cases(i32* %dst0, i32* %dst1, i32* %dst2, i32* %dst3, i32* %dst4, i32* %dst5, i32 %a, i32 %b) {
; CHECK: [[HOISTED:%.*]] = mul i32 %a, 2
; CHECK: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
; CHECK: [[ANCHOR0_HOISTED:%.*]] = mul i32 %b, 2
; CHECK: [[ANCHOR0_WS0:%.*]] = add i32 [[ANCHOR0_HOISTED]], [[WS0]]
  %add0 = add i32 %b, %ws0
  %mul0 = mul i32 %add0, 2
; CHECK: [[ANCHOR1_WS0:%.*]] = add i32 [[ANCHOR0_WS0]], 1
  %add0_2 = add i32 %mul0, 1
; CHECK: store i32 [[ANCHOR1_WS0]], i32* %dst0
  store i32 %add0_2, i32* %dst0
; CHECK: [[WS1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 1, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
; CHECK: [[ANCHOR0_WS1:%.*]] = add i32 [[ANCHOR0_HOISTED]], [[WS1]]
  %add1 = add i32 %b, %ws1
  %mul1 = mul i32 %add1, 2
; CHECK: [[ANCHOR1_WS1:%.*]] = add i32 [[ANCHOR0_WS1]], 1
  %add1_2 = add i32 %mul1, 1
; CHECK: store i32 [[ANCHOR1_WS1]], i32* %dst1
  store i32 %add1_2, i32* %dst1
; CHECK: [[WS2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 2, i32 0)
  %ws2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 2, i32 0)
; CHECK: [[ANCHOR0_WS2:%.*]] = add i32 [[ANCHOR0_HOISTED]], [[WS2]]
  %add2 = add i32 %b, %ws2
  %mul2 = mul i32 %add2, 2
; CHECK: [[ANCHOR1_WS2:%.*]] = add i32 [[ANCHOR0_WS2]], 1
  %add2_2 = add i32 %mul2, 1
; CHECK: store i32 [[ANCHOR1_WS2]], i32* %dst2
  store i32 %add2_2, i32* %dst2
; CHECK: [[WS3:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 3, i32 0)
  %ws3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 3, i32 0)
; CHECK: [[ANCHOR0_WS3:%.*]] = add i32 [[ANCHOR0_HOISTED]], [[WS3]]
  %add3 = add i32 %b, %ws3
  %mul3 = mul i32 %add3, 2
; CHECK: [[ANCHOR1_WS3:%.*]] = add i32 [[ANCHOR0_WS3]], 1
  %add3_2 = add i32 %mul3, 1
; CHECK: store i32 [[ANCHOR1_WS3]], i32* %dst3
  store i32 %add3_2, i32* %dst3

; CHECK: [[HOISTED2:%.*]] = add i32 %a, 4
; CHECK: [[WS4:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED2]], i32 4, i32 0)
  %ws4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 4, i32 0)
  %add4 = add i32 %ws4, 4
; CHECK: store i32 [[WS4]], i32* %dst4
  store i32 %add4, i32* %dst4
; CHECK: [[WS5:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED2]], i32 5, i32 0)
  %ws5 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 5, i32 0)
  %add5 = add i32 %ws5, 4
; CHECK: store i32 [[WS5]], i32* %dst5
  store i32 %add5, i32* %dst5
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
