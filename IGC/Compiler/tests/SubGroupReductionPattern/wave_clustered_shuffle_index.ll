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

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveClustered.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define i32 @wave_clustered_4_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 4, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext2, i32 0)
  %1 = add nsw i32 %0, %simdShuffle26
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext1, i32 0)
  %2 = add nsw i32 %1, %simdShuffle29
  ret i32 %2
}

define i32 @wave_clustered_2_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 2, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext1, i32 0)
  %1 = add nsw i32 %0, %simdShuffle29
  ret i32 %1
}

define i32 @wave_clustered_8_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 8, i32 0)
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
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %zext1, i32 0)
  %3 = add nsw i32 %2, %simdShuffle29
  ret i32 %3
}

define i32 @wave_clustered_4_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 4, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext2, i32 0)
  %1 = add nsw i32 %0, %simdShuffle26
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext1, i32 0)
  %2 = add nsw i32 %1, %simdShuffle29
  ret i32 %2
}

define i32 @wave_clustered_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 2, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext1, i32 0)
  %1 = add nsw i32 %0, %simdShuffle29
  ret i32 %1
}

define i32 @wave_clustered_16_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 16, i32 0)
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
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %3, i32 %zext1, i32 0)
  %4 = add nsw i32 %3, %simdShuffle29
  ret i32 %4
}

define i32 @wave_clustered_8_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 8, i32 0)
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
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %2, i32 %zext1, i32 0)
  %3 = add nsw i32 %2, %simdShuffle29
  ret i32 %3
}

define i32 @wave_clustered_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 4, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext2, i32 0)
  %1 = add nsw i32 %0, %simdShuffle26
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %1, i32 %zext1, i32 0)
  %2 = add nsw i32 %1, %simdShuffle29
  ret i32 %2
}

define i32 @wave_clustered_2_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveClustered.i32(i32 %0, i8 0, i32 2, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call i32 @get_i32()
  %xor1 = xor i16 %simdLaneId, 1
  %zext1 = zext i16 %xor1 to i32
  %simdShuffle29 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %0, i32 %zext1, i32 0)
  %1 = add nsw i32 %0, %simdShuffle29
  ret i32 %1
}

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare i32 @get_i32()

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8}

!0 = !{i32 ()* @wave_clustered_4_simd8, !100}
!1 = !{i32 ()* @wave_clustered_2_simd8, !100}
!2 = !{i32 ()* @wave_clustered_8_simd16, !200}
!3 = !{i32 ()* @wave_clustered_4_simd16, !200}
!4 = !{i32 ()* @wave_clustered_2_simd16, !200}
!5 = !{i32 ()* @wave_clustered_16_simd32, !300}
!6 = !{i32 ()* @wave_clustered_8_simd32, !300}
!7 = !{i32 ()* @wave_clustered_4_simd32, !300}
!8 = !{i32 ()* @wave_clustered_2_simd32, !300}
!100 = !{!101}
!101 = !{!"function_type", i32 0}
!200 = !{!201}
!201 = !{!"function_type", i32 0}
!300 = !{!301}
!301 = !{!"function_type", i32 0}
!303 = !{!"requiredSubGroupSize", i32 8}
!304 = !{!"requiredSubGroupSize", i32 8}
!305 = !{!"requiredSubGroupSize", i32 16}
!306 = !{!"requiredSubGroupSize", i32 16}
!307 = !{!"requiredSubGroupSize", i32 16}
!308 = !{!"requiredSubGroupSize", i32 32}
!309 = !{!"requiredSubGroupSize", i32 32}
!310 = !{!"requiredSubGroupSize", i32 32}
!311 = !{!"requiredSubGroupSize", i32 32}
!312 = !{!"FuncMDMap[0]", i32 ()* @wave_clustered_4_simd8}
!313 = !{!"FuncMDValue[0]", !303}
!314 = !{!"FuncMDMap[1]", i32 ()* @wave_clustered_2_simd8}
!315 = !{!"FuncMDValue[1]", !304}
!316 = !{!"FuncMDMap[2]", i32 ()* @wave_clustered_8_simd16}
!317 = !{!"FuncMDValue[2]", !305}
!318 = !{!"FuncMDMap[3]", i32 ()* @wave_clustered_4_simd16}
!319 = !{!"FuncMDValue[3]", !306}
!320 = !{!"FuncMDMap[4]", i32 ()* @wave_clustered_2_simd16}
!321 = !{!"FuncMDValue[4]", !307}
!322 = !{!"FuncMDMap[5]", i32 ()* @wave_clustered_16_simd32}
!323 = !{!"FuncMDValue[5]", !308}
!324 = !{!"FuncMDMap[6]", i32 ()* @wave_clustered_8_simd32}
!325 = !{!"FuncMDValue[6]", !309}
!326 = !{!"FuncMDMap[7]", i32 ()* @wave_clustered_4_simd32}
!327 = !{!"FuncMDValue[7]", !310}
!328 = !{!"FuncMDMap[8]", i32 ()* @wave_clustered_2_simd32}
!329 = !{!"FuncMDValue[8]", !311}
!330 = !{!"FuncMD", !312, !313, !314, !315, !316, !317, !318, !319, !320, !321, !322, !323, !324, !325, !326, !327, !328, !329}
!331 = !{!"ModuleMD", !330}
!IGCMetadata = !{!331}
