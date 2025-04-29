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
; This test checks if WaveShuffleIndex instructions are not merged when they are identical, but
; in separate basic blocks. This behavior helps to avoid potentially invalid LLVM IR being
; produced by not dominating all users.
; ------------------------------------------------

define void @test_compare_cases(i32* %dst0, i32* %dst1, i1 %condition) {
entry:
  br i1 %condition, label %bb0, label %bb1

bb0:
; CHECK: [[SHUFFLE0:%.*]] = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 1, i32 0, i32 0)
  %simdShuffle0 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 1, i32 0, i32 0)
  %and0 = and i8 %simdShuffle0, 1
  %cmp0 = icmp eq i8 %and0, 0
  %zxt0 = zext i1 %cmp0 to i32
  store i32 %zxt0, i32* %dst0
  br label %exit

bb1:
; CHECK: [[SHUFFLE1:%.*]] = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 1, i32 0, i32 0)
  %simdShuffle1 = call i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8 1, i32 0, i32 0)
  %and1 = and i8 %simdShuffle1, 1
  %cmp1 = icmp eq i8 %and1, 0
  %zxt1 = zext i1 %cmp1 to i32
  store i32 %zxt1, i32* %dst1
  br label %exit

exit:
  ret void
}

; Function Attrs: convergent nounwind readnone
declare i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8, i32, i32) #0

attributes #0 = { convergent nounwind readnone }