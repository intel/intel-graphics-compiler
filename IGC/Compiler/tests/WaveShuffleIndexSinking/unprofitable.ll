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
; Unprofitable to hoist the shl up even though it is allowed, because doing so will add 3 shl in exchange for removing 2 shl
; ------------------------------------------------

define void @test_unprofitable(i32* %dst0, i32* %dst1, i32 %a, i32 %b, i32 %c) {
; CHECK-NOT: shl i32 %a, 2
; CHECK: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
; CHECK-NOT: shl i32 %b, 2
; CHECK: [[ANCHOR0_WS0:%.*]] = add i32 [[WS0]], %b
  %add0 = add i32 %ws0, %b
; CHECK-NOT: shl i32 %c, 2
; CHECK: [[ANCHOR1_WS0:%.*]] = mul i32 [[ANCHOR0_WS0]], %c
  %mul0 = mul i32 %add0, %c
; CHECK: [[UNPROFITABLE_WS0:%.*]] = shl i32 [[ANCHOR1_WS0]], 2
  %shl0 = shl i32 %mul0, 2
; CHECK: store i32 [[UNPROFITABLE_WS0]], i32* %dst0
  store i32 %shl0, i32* %dst0
; CHECK: [[WS1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
; CHECK: [[ANCHOR0_WS1:%.*]] = add i32 [[WS1]], %b
  %add1 = add i32 %ws1, %b
; CHECK: [[ANCHOR1_WS1:%.*]] = mul i32 [[ANCHOR0_WS1]], %c
  %mul1 = mul i32 %add1, %c
; CHECK: [[UNPROFITABLE_WS1:%.*]] = shl i32 [[ANCHOR1_WS1]], 2
  %shl1 = shl i32 %mul1, 2
; CHECK: store i32 [[UNPROFITABLE_WS1]], i32* %dst1
  store i32 %shl1, i32* %dst1
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
