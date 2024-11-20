;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; This test does not primarily demonstrate the benefits/profitability of the optimization
; This test focuses on the auxiliary functionality of splitting and merging various WaveShuffleIndex instructions
; Only the 4 combinations of N/N, N/Y, Y/N, and Y/Y for Split/Merge need to be considered, Sink is for informational purposes only
;
; Test Scenarios
; %ws0: Split: N, Sink: Y, Merge: N
; %ws1: Split: Y, Sink: Y/Y, Merge: N (profitable to sink both paths)
; %ws2: Split: Y, Sink: Y/N, Merge: N (profitable to sink one path)
; %ws3: Split: Y, Sink: N/N, Merge: Y (nothing to group and sink with)
; %ws4: Split: N, Sink: N, Merge: Y
; %ws5: Split: N, Sink: N, Merge: Y
; ------------------------------------------------

define void @test_split_sink_merge(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK: [[USE1_WS0_WS1C1_HOISTED:%.*]] = shl i32 %a, 2
; CHECK: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[USE1_WS0_WS1C1_HOISTED]], i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %use1_ws0 = shl i32 %ws0, 2
; CHECK: add i32 [[WS0]], %c
  %anchor1_ws0 = add i32 %use1_ws0, %c
; CHECK: [[WS1C1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[USE1_WS0_WS1C1_HOISTED]], i32 1, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
  %use1_ws1 = shl i32 %ws1, 2
; CHECK: add i32 [[WS1C1]], %c
  %anchor1_ws1 = add i32 %use1_ws1, %c
; CHECK: [[USE2_WS1C2_WS2C1_HOISTED:%.*]] = shl i32 %a, 3
; CHECK: [[WS1C2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[USE2_WS1C2_WS2C1_HOISTED]], i32 1, i32 0)
  %use2_ws1 = shl i32 %ws1, 3
; CHECK: add i32 [[WS1C2]], %d
  %anchor2_ws1 = add i32 %use2_ws1, %d
; CHECK: [[WS2C1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[USE2_WS1C2_WS2C1_HOISTED]], i32 2, i32 0)
  %ws2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 2, i32 0)
  %use1_ws2 = shl i32 %ws2, 3
; CHECK: add i32 [[WS2C1]], %d
  %anchor1_ws2 = add i32 %use1_ws2, %d
; CHECK: [[WS2C2:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 2, i32 0)
; CHECK: [[USE2_WS2C2_NOT_HOISTED:%.*]] = shl i32 [[WS2C2]], 4
  %use2_ws2 = shl i32 %ws2, 4
; CHECK: add i32 [[USE2_WS2C2_NOT_HOISTED]], %d
  %anchor2_ws2 = add i32 %use2_ws2, %d
; CHECK: [[WS3:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 0, i32 0)
  %ws3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 0, i32 0)
; CHECK-NEXT: add i32 [[WS3]], %c
  %use1_ws3 = add i32 %ws3, %c
; CHECK-NEXT: add i32 [[WS3]], %d
  %use2_ws3 = add i32 %ws3, %d
; CHECK: [[WS4:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 1, i32 0)
  %ws4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 1, i32 0)
; CHECK-NEXT: {{%.*}} = add i32 [[WS4]], %c
  %use1_ws4 = add i32 %ws4, %c
; CHECK-NOT: call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 1, i32 0)
  %ws5 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %b, i32 1, i32 0)
; CHECK-NEXT: {{%.*}} = add i32 [[WS4]], %d
  %use1_ws5 = add i32 %ws5, %d
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }