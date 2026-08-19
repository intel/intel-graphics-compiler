;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-all-joint-reduction -S < %s | FileCheck %s
; ------------------------------------------------
; WaveAllJointReduction: source types wider than 32 bits are not merged yet, 32-bit and
; narrower types are.
; ------------------------------------------------

; i64 sources are wider than 32 bits, no joint reduction is created.
define i64 @test_i64_not_merged(i64 %a, i64 %b, i64 %c) {
; CHECK-LABEL: @test_i64_not_merged(
; CHECK-NOT: insertelement
; CHECK: %res_a = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %a, i8 0, i1 true, i32 0)
; CHECK-NEXT: %res_b = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %b, i8 0, i1 true, i32 0)
; CHECK-NEXT: %res_c = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %c, i8 0, i1 true, i32 0)
; CHECK-NOT: extractelement
  %res_a = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %a, i8 0, i1 true, i32 0)
  %res_b = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %b, i8 0, i1 true, i32 0)
  %res_c = call i64 @llvm.genx.GenISA.WaveAll.i64(i64 %c, i8 0, i1 true, i32 0)
  %join_a_b = add i64 %res_a, %res_b
  %join_a_b_c = add i64 %join_a_b, %res_c
  ret i64 %join_a_b_c
}

; double sources are wider than 32 bits, no joint reduction is created.
define double @test_double_not_merged(double %a, double %b, double %c) {
; CHECK-LABEL: @test_double_not_merged(
; CHECK-NOT: insertelement
; CHECK: %res_a = call double @llvm.genx.GenISA.WaveAll.f64(double %a, i8 9, i1 true, i32 0)
; CHECK-NEXT: %res_b = call double @llvm.genx.GenISA.WaveAll.f64(double %b, i8 9, i1 true, i32 0)
; CHECK-NEXT: %res_c = call double @llvm.genx.GenISA.WaveAll.f64(double %c, i8 9, i1 true, i32 0)
; CHECK-NOT: extractelement
  %res_a = call double @llvm.genx.GenISA.WaveAll.f64(double %a, i8 9, i1 true, i32 0)
  %res_b = call double @llvm.genx.GenISA.WaveAll.f64(double %b, i8 9, i1 true, i32 0)
  %res_c = call double @llvm.genx.GenISA.WaveAll.f64(double %c, i8 9, i1 true, i32 0)
  %join_a_b = fadd double %res_a, %res_b
  %join_a_b_c = fadd double %join_a_b, %res_c
  ret double %join_a_b_c
}

; Types narrower than 32 bits are still merged.
define half @test_half_merged(half %a, half %b, half %c) {
; CHECK-LABEL: @test_half_merged(
; CHECK: [[IN_A:%.*]] = insertelement <3 x half> undef, half %a, i64 0
; CHECK-NEXT: [[IN_AB:%.*]] = insertelement <3 x half> [[IN_A]], half %b, i64 1
; CHECK-NEXT: [[IN_ABC:%.*]] = insertelement <3 x half> [[IN_AB]], half %c, i64 2
; CHECK-NEXT: [[WAVE_ALL:%.*]] = call <3 x half> @llvm.genx.GenISA.WaveAll.v3f16(<3 x half> [[IN_ABC]], i8 9, i1 true, i32 0)
; CHECK-NOT: call half @llvm.genx.GenISA.WaveAll.f16
  %res_a = call half @llvm.genx.GenISA.WaveAll.f16(half %a, i8 9, i1 true, i32 0)
  %res_b = call half @llvm.genx.GenISA.WaveAll.f16(half %b, i8 9, i1 true, i32 0)
  %res_c = call half @llvm.genx.GenISA.WaveAll.f16(half %c, i8 9, i1 true, i32 0)
; CHECK: [[RES_A:%.*]] = extractelement <3 x half> [[WAVE_ALL]], i64 0
; CHECK-NEXT: [[RES_B:%.*]] = extractelement <3 x half> [[WAVE_ALL]], i64 1
; CHECK-NEXT: [[RES_C:%.*]] = extractelement <3 x half> [[WAVE_ALL]], i64 2
; CHECK: %join_a_b = fadd half [[RES_A]], [[RES_B]]
  %join_a_b = fadd half %res_a, %res_b
; CHECK-NEXT: %join_a_b_c = fadd half %join_a_b, [[RES_C]]
  %join_a_b_c = fadd half %join_a_b, %res_c
  ret half %join_a_b_c
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare i64 @llvm.genx.GenISA.WaveAll.i64(i64, i8, i1, i32) #0
declare double @llvm.genx.GenISA.WaveAll.f64(double, i8, i1, i32) #0
declare half @llvm.genx.GenISA.WaveAll.f16(half, i8, i1, i32) #0

attributes #0 = { convergent inaccessiblememonly nounwind }
