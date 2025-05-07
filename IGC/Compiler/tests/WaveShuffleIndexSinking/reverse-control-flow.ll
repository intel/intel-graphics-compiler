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
; Verifies if pass correctly moves "main" WaveShuffleIndex function call to
; common dominator basic block, which in this case is bb1.

define void @test(i32* %dst0, i32* %dst1, i32 %a) {
entry:
  br label %bb1

bb0:
  %ws0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  store i32 %ws0, i32* %dst0
  br label %exit

bb1:
; CHECK-LABEL: bb1:
; CHECK-NEXT: [[WS0:%.*]] = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  %ws1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %a, i32 0, i32 0)
  store i32 %ws1, i32* %dst1
  br label %bb0

exit:
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #0

attributes #0 = { convergent nounwind readnone }
