;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-all-joint-reduction -S < %s | FileCheck %s
; ------------------------------------------------
; WaveAllJointReduction: merge consecutive independent WaveAll operations into a single WaveAll joint operation
; ------------------------------------------------

define void @test_wave_all_joint_reduction(i32* %dst, i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, i32 %f, i32 %g, i32 %h) {
; CHECK: [[IN_A:%.*]] = insertelement <8 x i32> undef, i32 %a, i64 0
; CHECK-NEXT: [[IN_AB:%.*]] = insertelement <8 x i32> [[IN_A]], i32 %b, i64 1
; CHECK-NEXT: [[IN_ABC:%.*]] = insertelement <8 x i32> [[IN_AB]], i32 %c, i64 2
; CHECK-NEXT: [[IN_ABCD:%.*]] = insertelement <8 x i32> [[IN_ABC]], i32 %d, i64 3
; CHECK-NEXT: [[IN_ABCDE:%.*]] = insertelement <8 x i32> [[IN_ABCD]], i32 %e, i64 4
; CHECK-NEXT: [[IN_ABCDEF:%.*]] = insertelement <8 x i32> [[IN_ABCDE]], i32 %f, i64 5
; CHECK-NEXT: [[IN_ABCDEFG:%.*]] = insertelement <8 x i32> [[IN_ABCDEF]], i32 %g, i64 6
; CHECK-NEXT: [[IN_ABCDEFGH:%.*]] = insertelement <8 x i32> [[IN_ABCDEFG]], i32 %h, i64 7
; CHECK-NEXT: [[WAVE_ALL:%.*]] = call <8 x i32> @llvm.genx.GenISA.WaveAll.v8i32(<8 x i32> [[IN_ABCDEFGH]], i8 0, i1 true, i32 0)
; CHECK-NOT: call i32 @llvm.genx.GenISA.WaveAll.i32
  %res_a = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
  %res_b = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %b, i8 0, i1 true, i32 0)
  %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %c, i8 0, i1 true, i32 0)
  %res_d = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %d, i8 0, i1 true, i32 0)
  %res_e = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %e, i8 0, i1 true, i32 0)
  %res_f = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %f, i8 0, i1 true, i32 0)
  %res_g = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %g, i8 0, i1 true, i32 0)
  %res_h = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %h, i8 0, i1 true, i32 0)
; CHECK: [[RES_A:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 0
; CHECK-NEXT: [[RES_B:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 1
; CHECK-NEXT: [[RES_C:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 2
; CHECK-NEXT: [[RES_D:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 3
; CHECK-NEXT: [[RES_E:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 4
; CHECK-NEXT: [[RES_F:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 5
; CHECK-NEXT: [[RES_G:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 6
; CHECK-NEXT: [[RES_H:%.*]] = extractelement <8 x i32> [[WAVE_ALL]], i64 7
; CHECK: %join_a_b = add i32 [[RES_A]], [[RES_B]]
  %join_a_b = add i32 %res_a, %res_b
; CHECK: %join_c_d = add i32 [[RES_C]], [[RES_D]]
  %join_c_d = add i32 %res_c, %res_d
; CHECK: %join_e_f = add i32 [[RES_E]], [[RES_F]]
  %join_e_f = add i32 %res_e, %res_f
; CHECK: %join_g_h = add i32 [[RES_G]], [[RES_H]]
  %join_g_h = add i32 %res_g, %res_h
  %join_a_b_c_d = add i32 %join_a_b, %join_c_d
  %join_e_f_g_h = add i32 %join_e_f, %join_g_h
  %join_a_b_c_d_e_f_g_h = add i32 %join_a_b_c_d, %join_e_f_g_h
  store i32 %join_a_b_c_d_e_f_g_h, i32* %dst
  ret void
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare i32 @llvm.genx.GenISA.WaveAll.i32(i32, i8, i1, i32) #0

attributes #0 = { convergent inaccessiblememonly nounwind }
