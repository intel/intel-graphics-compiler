;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveAll.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define i32 @wave_all_add_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 0, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor16 = xor i16 %simdLaneId, 16
  %zext16 = zext i16 %xor16 to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext16, i32 0)
  %1 = add nsw i32 %0, %simdShuffle
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext8, i32 0)
  %2 = add nsw i32 %1, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %zext4, i32 0)
  %3 = add nsw i32 %2, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %3, i32 %zext2, i32 0)
  %4 = add nsw i32 %3, %simdShuffle26
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %4, i32 %zext1, i32 0)
  %5 = add nsw i32 %4, %simdShuffle29
  ret i32 %5
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare i32 @get_i32()

!igc.functions = !{!0}

!0 = !{i32 ()* @wave_all_add_i32, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 32}
