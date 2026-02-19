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
; Verifies that four WaveShuffleIndex instructions with the same source and a constant channel get subsequent instructions checked and hoisted
; Each WaveShuffleIndex instruction is in turn fed into an add, and then a shl
; The first operand of the add is not a constant, so the add is considered an anchor instruction
; The second operand of the shl is a constant, so the shl is considered a hoistable instruction
; Due to distributive properties, the shl is allowed to be hoisted above the add, and afterwards, above all the WaveShuffleIndex instructions
; Since there are 4 WaveShuffleIndex instructions in the ShuffleGroup, we can trade a shl on the source of the WaveShuffleIndex and a shl on the second operand of the add for removing all 4 shl instructions operating on the result of each add
; This changes the number of instructions from 4 * WSI + 4 * add + 4 * shl to shl(for %a) + shl(for %b) + 4 * WS + 4 * add, reducing the total number of instructions by 2, while preserving functionality
; ------------------------------------------------

define void @test_wave_shuffle_index_sinking(i32* %dst0, i32* %dst1, i32* %dst2, i32* %dst3, i32 %a, i32 %b) {
; CHECK: [[HOISTED:%.*]] = shl i32 %a, 2
; CHECK: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
; CHECK: [[ANCHOR_HOISTED:%.*]] = shl i32 %b, 2
; CHECK-NEXT: [[ANCHOR0:%.*]] = add i32 [[ANCHOR_HOISTED]], [[WS0]]
  %add0 = add i32 %b, %ws0
  %shl0 = shl i32 %add0, 2
; CHECK: store i32 [[ANCHOR0]], i32* %dst0
  store i32 %shl0, i32* %dst0
; CHECK: [[WS1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 1, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
; CHECK: [[ANCHOR1:%.*]] = add i32 [[WS1]], [[ANCHOR_HOISTED]]
  %add1 = add i32 %ws1, %b
  %shl1 = shl i32 %add1, 2
; CHECK: store i32 [[ANCHOR1]], i32* %dst1
  store i32 %shl1, i32* %dst1
; CHECK: [[WS2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 2, i32 0)
  %ws2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 2, i32 0)
; CHECK: [[ANCHOR2:%.*]] = add i32 [[ANCHOR_HOISTED]], [[WS2]]
  %add2 = add i32 %b, %ws2
  %shl2 = shl i32 %add2, 2
; CHECK: store i32 [[ANCHOR2]], i32* %dst2
  store i32 %shl2, i32* %dst2
; CHECK: [[WS3:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 3, i32 0)
  %ws3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 3, i32 0)
; CHECK: [[ANCHOR3:%.*]] = add i32 [[WS3]], [[ANCHOR_HOISTED]]
  %add3 = add i32 %ws3, %b
  %shl3 = shl i32 %add3, 2
; CHECK: store i32 [[ANCHOR3]], i32* %dst3
  store i32 %shl3, i32* %dst3
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
