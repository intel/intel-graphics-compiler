;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; Regression test for the hoisted-instruction erase order. Each WaveShuffleIndex
; here feeds a chain of TWO adjacent hoistable instructions (mul by a constant,
; then shl by a constant) with no anchor in between. Both get hoisted above the
; broadcast and the originals are erased.
;
; The erase walked the chain front-to-back, so it deleted the mul while the
; not-yet-erased shl still used it, tripping the LLVM assertion
;   "Uses remain when a value is destroyed!" (Value.cpp)
; in debug builds (and a use-after-free in release). Erasing back-to-front fixes
; it: the shl (successor) is removed before the mul it consumes.
; ------------------------------------------------

define void @test_adjacent_hoistables(i32* %dst0, i32* %dst1, i32 %a) {
; CHECK-LABEL: @test_adjacent_hoistables(
; CHECK:    [[MUL:%.*]] = mul i32 %a, 5
; CHECK:    [[SHL:%.*]] = shl i32 [[MUL]], 2
; CHECK:    [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[SHL]], i32 0, i32 0)
; CHECK:    store i32 [[WS0]], i32* %dst0
; CHECK:    [[WS1:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[SHL]], i32 1, i32 0)
; CHECK:    store i32 [[WS1]], i32* %dst1

  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %mul0 = mul i32 %ws0, 5
  %shl0 = shl i32 %mul0, 2
  store i32 %shl0, i32* %dst0
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 1, i32 0)
  %mul1 = mul i32 %ws1, 5
  %shl1 = shl i32 %mul1, 2
  store i32 %shl1, i32* %dst1
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
