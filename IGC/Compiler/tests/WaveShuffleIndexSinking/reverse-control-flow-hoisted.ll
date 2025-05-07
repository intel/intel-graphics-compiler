;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --typed-pointers -igc-wave-shuffle-index-sinking -S < %s | FileCheck %s
; ------------------------------------------------
; WaveShuffleIndexSinking
;
; Verifies if pass correctly moves hoisted variables and merged WaveShuffleIndex
; to common dominator basic block in correct order. In this case it is bb1.

define void @test(i32* %dst0, i32* %dst1, i32 %a, i32 %b) {
entry:
  br label %bb1

bb0:
; CHECK-LABEL: bb0:
; CHECK-NOT [[WS:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[TMP:.*]], i32 0, i32 0)
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %add0 = add i32 %b, %ws0
  %shl0 = shl i32 %add0, 2
  store i32 %shl0, i32* %dst0
  br label %exit

bb1:
; CHECK-LABEL: bb1:
; CHECK-NEXT: [[HOISTED:%.*]] = shl i32 %a, 2
; CHECK-NEXT: [[ANCHOR_HOISTED:%.*]] = shl i32 %b, 2
; CHECK-NEXT: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 [[HOISTED]], i32 0, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %add1 = add i32 %ws1, %b
  %shl1 = shl i32 %add1, 2
  store i32 %shl1, i32* %dst1
  br label %bb0

exit:
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
