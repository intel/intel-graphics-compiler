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

define i32 @wave_all_add_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 0, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = add nsw i32 %0, %simdShuffleXor
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = add nsw i32 %1, %simdShuffleXor27
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = add nsw i32 %2, %simdShuffleXor28
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = add nsw i32 %3, %simdShuffleXor29
  ret i32 %4
}

define i32 @wave_all_mul_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 1, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = mul nsw i32 %0, %simdShuffleXor
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = mul nsw i32 %1, %simdShuffleXor27
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = mul nsw i32 %2, %simdShuffleXor28
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = mul nsw i32 %3, %simdShuffleXor29
  ret i32 %4
}

define i32 @wave_all_umin_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 2, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = call i32 @llvm.umin.i32(i32 %simdShuffleXor, i32 %0)
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = call i32 @llvm.umin.i32(i32 %simdShuffleXor27, i32 %1)
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = call i32 @llvm.umin.i32(i32 %simdShuffleXor28, i32 %2)
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = call i32 @llvm.umin.i32(i32 %simdShuffleXor29, i32 %3)
  ret i32 %4
}

define i32 @wave_all_umax_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 3, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = call i32 @llvm.umax.i32(i32 %simdShuffleXor, i32 %0)
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = call i32 @llvm.umax.i32(i32 %simdShuffleXor27, i32 %1)
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = call i32 @llvm.umax.i32(i32 %simdShuffleXor28, i32 %2)
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = call i32 @llvm.umax.i32(i32 %simdShuffleXor29, i32 %3)
  ret i32 %4
}

define i32 @wave_all_smin_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 4, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = call i32 @llvm.smin.i32(i32 %simdShuffleXor, i32 %0)
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = call i32 @llvm.smin.i32(i32 %simdShuffleXor27, i32 %1)
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = call i32 @llvm.smin.i32(i32 %simdShuffleXor28, i32 %2)
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = call i32 @llvm.smin.i32(i32 %simdShuffleXor29, i32 %3)
  ret i32 %4
}

define i32 @wave_all_smax_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 5, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = call i32 @llvm.smax.i32(i32 %simdShuffleXor, i32 %0)
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = call i32 @llvm.smax.i32(i32 %simdShuffleXor27, i32 %1)
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = call i32 @llvm.smax.i32(i32 %simdShuffleXor28, i32 %2)
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = call i32 @llvm.smax.i32(i32 %simdShuffleXor29, i32 %3)
  ret i32 %4
}

define i32 @wave_all_or_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 6, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = or i32 %0, %simdShuffleXor
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = or i32 %1, %simdShuffleXor27
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = or i32 %2, %simdShuffleXor28
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = or i32 %3, %simdShuffleXor29
  ret i32 %4
}

define i32 @wave_all_xor_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 7, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = xor i32 %0, %simdShuffleXor
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = xor i32 %1, %simdShuffleXor27
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = xor i32 %2, %simdShuffleXor28
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = xor i32 %3, %simdShuffleXor29
  ret i32 %4
}

define i32 @wave_all_and_i32() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call i32 @get_i32()
; CHECK:         [[RESULT:%.*]] = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %0, i8 8, i1 true, i32 0)
; CHECK:         ret i32 [[RESULT]]
  %0 = call i32 @get_i32()
  %simdShuffleXor = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %0, i32 8)
  %1 = and i32 %0, %simdShuffleXor
  %simdShuffleXor27 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %1, i32 4)
  %2 = and i32 %1, %simdShuffleXor27
  %simdShuffleXor28 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %2, i32 2)
  %3 = and i32 %2, %simdShuffleXor28
  %simdShuffleXor29 = call i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32 %3, i32 1)
  %4 = and i32 %3, %simdShuffleXor29
  ret i32 %4
}

define float @wave_all_add_float() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float %0, i8 9, i1 true, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 8)
  %1 = fadd float %simdShuffleXor, %0
  %simdShuffleXor27 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 4)
  %2 = fadd float %1, %simdShuffleXor27
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %2, i32 2)
  %3 = fadd float %2, %simdShuffleXor28
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %3, i32 1)
  %4 = fadd float %3, %simdShuffleXor29
  ret float %4
}

define float @wave_all_mul_float() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float %0, i8 10, i1 true, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 8)
  %1 = fmul float %simdShuffleXor, %0
  %simdShuffleXor27 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 4)
  %2 = fmul float %1, %simdShuffleXor27
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %2, i32 2)
  %3 = fmul float %2, %simdShuffleXor28
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %3, i32 1)
  %4 = fmul float %3, %simdShuffleXor29
  ret float %4
}

define float @wave_all_min_float() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float %0, i8 11, i1 true, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 8)
  %1 = call float @llvm.minnum.f32(float %simdShuffleXor, float %0)
  %simdShuffleXor27 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 4)
  %2 = call float @llvm.minnum.f32(float %simdShuffleXor27, float %1)
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %2, i32 2)
  %3 = call float @llvm.minnum.f32(float %simdShuffleXor28, float %2)
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %3, i32 1)
  %4 = call float @llvm.minnum.f32(float %simdShuffleXor29, float %3)
  ret float %4
}

define float @wave_all_max_float() {
entry:
; CHECK-LABEL: entry:
; CHECK:         %0 = call float @get_float()
; CHECK:         [[RESULT:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float %0, i8 12, i1 true, i32 0)
; CHECK:         ret float [[RESULT]]
  %0 = call float @get_float()
  %simdShuffleXor = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %0, i32 8)
  %1 = call float @llvm.maxnum.f32(float %simdShuffleXor, float %0)
  %simdShuffleXor27 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %1, i32 4)
  %2 = call float @llvm.maxnum.f32(float %simdShuffleXor27, float %1)
  %simdShuffleXor28 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %2, i32 2)
  %3 = call float @llvm.maxnum.f32(float %simdShuffleXor28, float %2)
  %simdShuffleXor29 = call float @llvm.genx.GenISA.simdShuffleXor.f32(float %3, i32 1)
  %4 = call float @llvm.maxnum.f32(float %simdShuffleXor29, float %3)
  ret float %4
}

declare i32 @llvm.genx.GenISA.simdShuffleXor.i32(i32, i32)
declare float @llvm.genx.GenISA.simdShuffleXor.f32(float, i32)

declare i32 @llvm.umin.i32(i32, i32)
declare i32 @llvm.umax.i32(i32, i32)
declare i32 @llvm.smin.i32(i32, i32)
declare i32 @llvm.smax.i32(i32, i32)
declare float @llvm.minnum.f32(float, float)
declare float @llvm.maxnum.f32(float, float)

declare i32 @get_i32()
declare float @get_float()

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12}

!0 = !{i32 ()* @wave_all_add_i32, !100}
!1 = !{i32 ()* @wave_all_mul_i32, !100}
!2 = !{i32 ()* @wave_all_umin_i32, !100}
!3 = !{i32 ()* @wave_all_umax_i32, !100}
!4 = !{i32 ()* @wave_all_smin_i32, !100}
!5 = !{i32 ()* @wave_all_smax_i32, !100}
!6 = !{i32 ()* @wave_all_or_i32, !100}
!7 = !{i32 ()* @wave_all_xor_i32, !100}
!8 = !{i32 ()* @wave_all_and_i32, !100}
!9 = !{float ()* @wave_all_add_float, !100}
!10 = !{float ()* @wave_all_mul_float, !100}
!11 = !{float ()* @wave_all_min_float, !100}
!12 = !{float ()* @wave_all_max_float, !100}
!100 = !{!101, !102}
!101 = !{!"function_type", i32 0}
!102 = !{!"sub_group_size", i32 16}
