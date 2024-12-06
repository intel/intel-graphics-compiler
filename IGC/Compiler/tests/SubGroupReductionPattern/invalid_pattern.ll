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

; Test if a pattern of repeated ShuffleXor + op is recognized and replaced with WaveAll.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define float @shuffle_xor_invalid_pattern() {
entry:
; COM: Check no change is applied.
;
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
; CHECK:         %1 = uitofp i32 %simdShuffleXor to float
; CHECK:         ret float %1
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = uitofp i32 %simdShuffleXor to float
  ret float %1
}

define float @shuffle_index_invalid_pattern() {
entry:
; COM: Check no change is applied.
;
; CHECK-LABEL: entry:
; CHECK:         %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         %xor16 = xor i16 %simdLaneId, 16
; CHECK:         %zext16 = zext i16 %xor16 to i32
; CHECK:         %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext16, i32 0)
; CHECK:         %1 = uitofp i32 %simdShuffle to float
; CHECK:         ret float %1
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor16 = xor i16 %simdLaneId, 16
  %zext16 = zext i16 %xor16 to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext16, i32 0)
  %1 = uitofp i32 %simdShuffle to float
  ret float %1
}

declare i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32, i32)
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare i32 @get_i32()

!igc.functions = !{!0, !1}

!0 = !{float ()* @shuffle_xor_invalid_pattern, !100}
!1 = !{float ()* @shuffle_index_invalid_pattern, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 16}
