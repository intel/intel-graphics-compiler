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
; An initial ShuffleGroup's InstChain may get reduced to encompass a wider number of WaveShuffleIndex that have fewer similar instructions
; The pass is configured to run iteratively up to a maximum of the value specified by the WaveShuffleIndexSinkingMaxIterations regkey (default: 3)
; Rerunning ensures that any potential hoistable instructions that were kicked out of a ShuffleGroup that are still profitable to merge will get merged eventually
; ShuffleGroup consisting of %ws0, %ws1, and %ws2 gets trimmed in first iteration in order to accommodate %ws3
; ashr gets hoisted in next iteration when ShuffleGroup (%ws0, %ws1, %ws2) gets reconstructed and %ws3 has no more suitable instructions in order to join the ShuffleGroup
; Note: Hoisted instructions are to demonstrate functionality, InstCombine would reduce the shl by 2 and ashr by 1 to a single shl by 1
; ------------------------------------------------

define void @test_wave_shuffle_index_sinking(i32* %dst0, i32* %dst1, i32* %dst2, i32* %dst3, i32 %a, i32 %b, i32 %c) {
; CHECK: [[HOISTED_I1:%.*]] = shl i32 %a, 2
; CHECK: [[HOISTED_I2:%.*]] = ashr i32 [[HOISTED_I1]], 1
; CHECK: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED_I2]], i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
; CHECK: [[ANCHOR1_HOISTED_I1:%.*]] = shl i32 %b, 2
; CHECK: [[ANCHOR1_HOISTED_I2:%.*]] = ashr i32 [[ANCHOR1_HOISTED_I1]], 1
; CHECK-NEXT: [[ANCHOR1_WS0:%.*]] = add i32 [[WS0]], [[ANCHOR1_HOISTED_I2]]
  %add0 = add i32 %ws0, %b
  %shl0 = shl i32 %add0, 2
; CHECK: [[ANCHOR2_HOISTED_I2:%.*]] = ashr i32 %c, 1
; CHECK-NEXT: [[ANCHOR2_WS0:%.*]] = mul i32 [[ANCHOR1_WS0]], [[ANCHOR2_HOISTED_I2]]
  %mul0 = mul i32 %shl0, %c
  %ashr0 = ashr i32 %mul0, 1
; CHECK: store i32 [[ANCHOR2_WS0]], i32* %dst0
  store i32 %ashr0, i32* %dst0
; CHECK: [[WS1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED_I2]], i32 1, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
; CHECK: [[ANCHOR1_WS1:%.*]] = add i32 [[WS1]], [[ANCHOR1_HOISTED_I2]]
  %add1 = add i32 %ws1, %b
  %shl1 = shl i32 %add1, 2
; CHECK: [[ANCHOR2_WS1:%.*]] = mul i32 [[ANCHOR1_WS1]], [[ANCHOR2_HOISTED_I2]]
  %mul1 = mul i32 %shl1, %c
  %ashr1 = ashr i32 %mul1, 1
; CHECK: store i32 [[ANCHOR2_WS1]], i32* %dst1
  store i32 %ashr1, i32* %dst1
; CHECK: [[WS2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED_I2]], i32 2, i32 0)
  %ws2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 2, i32 0)
; CHECK: [[ANCHOR1_WS2:%.*]] = add i32 [[WS2]], [[ANCHOR1_HOISTED_I2]]
  %add2 = add i32 %ws2, %b
  %shl2 = shl i32 %add2, 2
; CHECK: [[ANCHOR2_WS2:%.*]] = mul i32 [[ANCHOR1_WS2]], [[ANCHOR2_HOISTED_I2]]
  %mul2 = mul i32 %shl2, %c
  %ashr2 = ashr i32 %mul2, 1
; CHECK: store i32 [[ANCHOR2_WS2]], i32* %dst2
  store i32 %ashr2, i32* %dst2
; CHECK: [[WS3:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED_I1]], i32 3, i32 0)
  %ws3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 3, i32 0)
; CHECK: [[ANCHOR1_WS3:%.*]] = add i32 [[WS3]], [[ANCHOR1_HOISTED_I1]]
  %add3 = add i32 %ws3, %b
  %shl3 = shl i32 %add3, 2
; CHECK: store i32 [[ANCHOR1_WS3]], i32* %dst3
  store i32 %shl3, i32* %dst3
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
