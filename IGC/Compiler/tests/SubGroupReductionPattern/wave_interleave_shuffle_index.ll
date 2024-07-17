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

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveInterleave.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define i32 @wave_interleave_2_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 2, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext4, i32 0)
  %1 = add nsw i32 %0, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext2, i32 0)
  %2 = add nsw i32 %1, %simdShuffle26
  ret i32 %2
}

define i32 @wave_interleave_4_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 4, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext4, i32 0)
  %1 = add nsw i32 %0, %simdShuffle23
  ret i32 %1
}

define i32 @wave_interleave_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 2, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext8, i32 0)
  %1 = add nsw i32 %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext4, i32 0)
  %2 = add nsw i32 %1, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %zext2, i32 0)
  %3 = add nsw i32 %2, %simdShuffle26
  ret i32 %3
}

define i32 @wave_interleave_4_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 4, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext8, i32 0)
  %1 = add nsw i32 %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext4, i32 0)
  %2 = add nsw i32 %1, %simdShuffle23
  ret i32 %2
}

define i32 @wave_interleave_8_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 8, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext8, i32 0)
  %1 = add nsw i32 %0, %simdShuffle20
  ret i32 %1
}

define i32 @wave_interleave_2_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 2, i32 0)
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
  ret i32 %4
}

define i32 @wave_interleave_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 4, i32 0)
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
  ret i32 %3
}

define i32 @wave_interleave_8_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 8, i32 0)
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
  ret i32 %2
}

define i32 @wave_interleave_16_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveInterleave.i32(i32 %0, i8 0, i32 16, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor16 = xor i16 %simdLaneId, 16
  %zext16 = zext i16 %xor16 to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext16, i32 0)
  %1 = add nsw i32 %0, %simdShuffle
  ret i32 %1
}

define i32 @simd8_not_wave_interleave() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call i32 @llvm.genx.GenISA.WaveInterleave.i32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor4 = xor i16 %simdLaneId, 2
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext4, i32 0)
  %1 = add nsw i32 %0, %simdShuffle23
  ret i32 %1
}

define i32 @simd16_not_wave_interleave() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call i32 @llvm.genx.GenISA.WaveInterleave.i32ISA.simdLaneId()
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor8 = xor i16 %simdLaneId, 4
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext8, i32 0)
  %1 = add nsw i32 %0, %simdShuffle20
  ret i32 %1
}

define i32 @simd32_not_wave_interleave() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call i32 @llvm.genx.GenISA.WaveInterleave.i32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor16 = xor i16 %simdLaneId, 8
  %zext16 = zext i16 %xor16 to i32
  %simdShuffle = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext16, i32 0)
  %1 = add nsw i32 %0, %simdShuffle
  ret i32 %1
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare i32 @get_i32()

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11}

!0 = !{i32 ()* @wave_interleave_2_simd8, !100}
!1 = !{i32 ()* @wave_interleave_4_simd8, !100}
!2 = !{i32 ()* @wave_interleave_2_simd16, !200}
!3 = !{i32 ()* @wave_interleave_4_simd16, !200}
!4 = !{i32 ()* @wave_interleave_8_simd16, !200}
!5 = !{i32 ()* @wave_interleave_2_simd32, !300}
!6 = !{i32 ()* @wave_interleave_4_simd32, !300}
!7 = !{i32 ()* @wave_interleave_8_simd32, !300}
!8 = !{i32 ()* @wave_interleave_16_simd32, !300}
!9 = !{i32 ()* @simd8_not_wave_interleave, !100}
!10 = !{i32 ()* @simd16_not_wave_interleave, !200}
!11 = !{i32 ()* @simd32_not_wave_interleave, !300}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 8}
!200 = !{!201, !202}
!201 = !{!"function_type", i32 0}
!202 = !{!"sub_group_size", i32 16}
!300 = !{!301, !302}
!301 = !{!"function_type", i32 0}
!302 = !{!"sub_group_size", i32 32}
