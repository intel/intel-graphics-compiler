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
; RUN: igc_opt -platformpvc -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-PVC
; RUN: igc_opt -platformmtl -debugify -igc-subgroup-reduction-pattern -check-debugify -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-MTL

; Test if a pattern of repeated ShuffleIndex + op is recognized and replaced with WaveClusteredInterleave.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define float @wave_clustered_interleave_8_4_2_simd32() {
entry:
; CHECK-LABEL: entry:
;
; CHECK-PVC:     %0 = call float @get_f32()
; CHECK-PVC:     [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveClusteredInterleave.f32(float %0, i8 9, i32 16, i32 2, i32 0)
; CHECK-PVC:     ret float [[RESULT]]
;
; CHECK-MTL-NOT: call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext8, i32 0)
  %1 = fadd float %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %1, i32 %zext4, i32 0)
  %2 = fadd float %1, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %2, i32 %zext2, i32 0)
  %3 = fadd float %2, %simdShuffle26
  ret float %3
}

define float @wave_clustered_interleave_8_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext8, i32 0)
  %1 = fadd float %0, %simdShuffle20
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %1, i32 %zext4, i32 0)
  %2 = fadd float %1, %simdShuffle23
  ret float %2
}

define float @wave_clustered_interleave_8_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor8 = xor i16 %simdLaneId, 8
  %zext8 = zext i16 %xor8 to i32
  %simdShuffle20 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext8, i32 0)
  %1 = fadd float %0, %simdShuffle20
  ret float %1
}

define float @wave_clustered_interleave_4_2_simd32() {
entry:
; CHECK-LABEL: entry:
;
; CHECK-PVC:     %0 = call float @get_f32()
; CHECK-PVC:     [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveClusteredInterleave.f32(float %0, i8 9, i32 8, i32 2, i32 0)
; CHECK-PVC:     ret float [[RESULT]]
;
; CHECK-MTL-NOT: call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext4, i32 0)
  %1 = fadd float %0, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %1, i32 %zext2, i32 0)
  %2 = fadd float %1, %simdShuffle26
  ret float %2
}

define float @wave_clustered_interleave_4_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext4, i32 0)
  %1 = fadd float %0, %simdShuffle23
  ret float %1
}

define float @wave_clustered_interleave_2_simd32() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext2, i32 0)
  %1 = fadd float %0, %simdShuffle23
  ret float %1
}

define float @wave_clustered_interleave_4_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext4, i32 0)
  %1 = fadd float %0, %simdShuffle23
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle26 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %1, i32 %zext2, i32 0)
  %2 = fadd float %1, %simdShuffle26
  ret float %2
}

define float @wave_clustered_interleave_4_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor4 = xor i16 %simdLaneId, 4
  %zext4 = zext i16 %xor4 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext4, i32 0)
  %1 = fadd float %0, %simdShuffle23
  ret float %1
}

define float @wave_clustered_interleave_2_simd16() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext2, i32 0)
  %1 = fadd float %0, %simdShuffle23
  ret float %1
}

define float @wave_clustered_interleave_2_simd8() {
entry:
; CHECK-LABEL: entry:
; CHECK-NOT:     call float @llvm.genx.GenISA.WaveClusteredInterleave.f32
  %simdLaneId = call i16 @llvm.genx.GenISA.simdLaneId()
  %0 = call float @get_f32()
  %xor2 = xor i16 %simdLaneId, 2
  %zext2 = zext i16 %xor2 to i32
  %simdShuffle23 = call float @llvm.genx.GenISA.WaveShuffleIndex.f32(float %0, i32 %zext2, i32 0)
  %1 = fadd float %0, %simdShuffle23
  ret float %1
}

declare float @llvm.genx.GenISA.WaveShuffleIndex.f32(float, i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

declare float @get_f32()

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9}

!0 = !{float ()* @wave_clustered_interleave_8_4_2_simd32, !300}
!1 = !{float ()* @wave_clustered_interleave_8_4_simd32, !300}
!2 = !{float ()* @wave_clustered_interleave_8_simd32, !300}
!3 = !{float ()* @wave_clustered_interleave_4_2_simd32, !300}
!4 = !{float ()* @wave_clustered_interleave_4_simd32, !300}
!5 = !{float ()* @wave_clustered_interleave_2_simd32, !300}
!6 = !{float ()* @wave_clustered_interleave_4_2_simd16, !200}
!7 = !{float ()* @wave_clustered_interleave_4_simd16, !200}
!8 = !{float ()* @wave_clustered_interleave_2_simd16, !200}
!9 = !{float ()* @wave_clustered_interleave_2_simd8, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 8}
!200 = !{!201, !202}
!201 = !{!"function_type", i32 0}
!202 = !{!"sub_group_size", i32 16}
!300 = !{!301, !302}
!301 = !{!"function_type", i32 0}
!302 = !{!"sub_group_size", i32 32}
