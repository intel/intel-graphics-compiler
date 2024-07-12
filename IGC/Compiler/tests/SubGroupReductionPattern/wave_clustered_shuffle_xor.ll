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

; Test if a pattern of repeated ShuffleXor + op is recognized and replaced with WaveClustered.

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

define float @wave_clustered_add_float_cluster_size_8() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveClustered.f32(float %0, i8 9, i32 8, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor27 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 4)
  %1 = fadd float %0, %simdShuffleXor27
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 2)
  %2 = fadd float %1, %simdShuffleXor28
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %2, i32 1)
  %3 = fadd float %2, %simdShuffleXor29
  ret float %3
}

define float @wave_clustered_add_float_cluster_size_4() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveClustered.f32(float %0, i8 9, i32 4, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 2)
  %1 = fadd float %0, %simdShuffleXor28
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 1)
  %2 = fadd float %1, %simdShuffleXor29
  ret float %2
}

define float @wave_clustered_add_float_cluster_size_2() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveClustered.f32(float %0, i8 9, i32 2, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 1)
  %1 = fadd float %0, %simdShuffleXor29
  ret float %1
}

declare float @llvm.genx.GenISA.simdShuffleXor.f32(float, i32)

declare float @get_float()

!igc.functions = !{!0, !1, !2}

!0 = !{float ()* @wave_clustered_add_float_cluster_size_8, !100}
!1 = !{float ()* @wave_clustered_add_float_cluster_size_4, !100}
!2 = !{float ()* @wave_clustered_add_float_cluster_size_2, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 16}
