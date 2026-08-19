;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-all-joint-reduction -S < %s | FileCheck %s
; ------------------------------------------------
; WaveAllJointReduction: FMIN (i8 11) and FMAX (i8 12) are not merged yet, the other
; floating point reduction kinds still are.
; ------------------------------------------------

define float @test_fmin_not_merged(float %a, float %b, float %c) {
; CHECK-LABEL: @test_fmin_not_merged(
; CHECK-NOT: insertelement
; CHECK: %res_a = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 11, i1 true, i32 0)
; CHECK-NEXT: %res_b = call float @llvm.genx.GenISA.WaveAll.f32(float %b, i8 11, i1 true, i32 0)
; CHECK-NEXT: %res_c = call float @llvm.genx.GenISA.WaveAll.f32(float %c, i8 11, i1 true, i32 0)
; CHECK-NOT: extractelement
  %res_a = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 11, i1 true, i32 0)
  %res_b = call float @llvm.genx.GenISA.WaveAll.f32(float %b, i8 11, i1 true, i32 0)
  %res_c = call float @llvm.genx.GenISA.WaveAll.f32(float %c, i8 11, i1 true, i32 0)
  %join_a_b = fadd float %res_a, %res_b
  %join_a_b_c = fadd float %join_a_b, %res_c
  ret float %join_a_b_c
}

define float @test_fmax_not_merged(float %a, float %b, float %c) {
; CHECK-LABEL: @test_fmax_not_merged(
; CHECK-NOT: insertelement
; CHECK: %res_a = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 12, i1 true, i32 0)
; CHECK-NEXT: %res_b = call float @llvm.genx.GenISA.WaveAll.f32(float %b, i8 12, i1 true, i32 0)
; CHECK-NEXT: %res_c = call float @llvm.genx.GenISA.WaveAll.f32(float %c, i8 12, i1 true, i32 0)
; CHECK-NOT: extractelement
  %res_a = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 12, i1 true, i32 0)
  %res_b = call float @llvm.genx.GenISA.WaveAll.f32(float %b, i8 12, i1 true, i32 0)
  %res_c = call float @llvm.genx.GenISA.WaveAll.f32(float %c, i8 12, i1 true, i32 0)
  %join_a_b = fadd float %res_a, %res_b
  %join_a_b_c = fadd float %join_a_b, %res_c
  ret float %join_a_b_c
}

; An FMIN group in between two FSUM groups must not glue them together.
define float @test_fmin_separates_fsum_groups(float %a, float %b, float %c, float %d) {
; CHECK-LABEL: @test_fmin_separates_fsum_groups(
; CHECK: [[IN_A:%.*]] = insertelement <2 x float> undef, float %a, i64 0
; CHECK-NEXT: [[IN_AB:%.*]] = insertelement <2 x float> [[IN_A]], float %b, i64 1
; CHECK-NEXT: [[WAVE_ALL_AB:%.*]] = call <2 x float> @llvm.genx.GenISA.WaveAll.v2f32(<2 x float> [[IN_AB]], i8 9, i1 true, i32 0)
; CHECK-NEXT: [[RES_A:%.*]] = extractelement <2 x float> [[WAVE_ALL_AB]], i64 0
; CHECK-NEXT: [[RES_B:%.*]] = extractelement <2 x float> [[WAVE_ALL_AB]], i64 1
  %res_a = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 9, i1 true, i32 0)
  %res_b = call float @llvm.genx.GenISA.WaveAll.f32(float %b, i8 9, i1 true, i32 0)
; CHECK-NEXT: %res_min = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 11, i1 true, i32 0)
  %res_min = call float @llvm.genx.GenISA.WaveAll.f32(float %a, i8 11, i1 true, i32 0)
; CHECK-NEXT: [[IN_C:%.*]] = insertelement <2 x float> undef, float %c, i64 0
; CHECK-NEXT: [[IN_CD:%.*]] = insertelement <2 x float> [[IN_C]], float %d, i64 1
; CHECK-NEXT: [[WAVE_ALL_CD:%.*]] = call <2 x float> @llvm.genx.GenISA.WaveAll.v2f32(<2 x float> [[IN_CD]], i8 9, i1 true, i32 0)
; CHECK-NEXT: [[RES_C:%.*]] = extractelement <2 x float> [[WAVE_ALL_CD]], i64 0
; CHECK-NEXT: [[RES_D:%.*]] = extractelement <2 x float> [[WAVE_ALL_CD]], i64 1
  %res_c = call float @llvm.genx.GenISA.WaveAll.f32(float %c, i8 9, i1 true, i32 0)
  %res_d = call float @llvm.genx.GenISA.WaveAll.f32(float %d, i8 9, i1 true, i32 0)
; CHECK: %join_a_b = fadd float [[RES_A]], [[RES_B]]
  %join_a_b = fadd float %res_a, %res_b
; CHECK-NEXT: %join_c_d = fadd float [[RES_C]], [[RES_D]]
  %join_c_d = fadd float %res_c, %res_d
  %join_a_b_c_d = fadd float %join_a_b, %join_c_d
  %join_all = fadd float %join_a_b_c_d, %res_min
  ret float %join_all
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #0

attributes #0 = { convergent inaccessiblememonly nounwind }
