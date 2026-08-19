;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-wave-all-joint-reduction -S < %s | FileCheck %s
; ------------------------------------------------
; WaveAllJointReduction: a WaveAll whose source is produced by a WaveAll already in the
; merge list must not join that list, otherwise the joint operation would consume its own
; result.
; ------------------------------------------------

; Fully chained WaveAll operations, nothing can be merged.
define i32 @test_chain_not_merged(i32 %a) {
; CHECK-LABEL: @test_chain_not_merged(
; CHECK-NOT: insertelement
; CHECK: %res_a = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
; CHECK-NEXT: %res_b = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_a, i8 0, i1 true, i32 0)
; CHECK-NEXT: %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_b, i8 0, i1 true, i32 0)
; CHECK-NOT: extractelement
  %res_a = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
  %res_b = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_a, i8 0, i1 true, i32 0)
  %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_b, i8 0, i1 true, i32 0)
  ret i32 %res_c
}

; The chained WaveAll ends the merge list, the preceding independent ones are still merged.
define i32 @test_chain_ends_merge_list(i32 %a, i32 %b) {
; CHECK-LABEL: @test_chain_ends_merge_list(
; CHECK: [[IN_A:%.*]] = insertelement <2 x i32> undef, i32 %a, i64 0
; CHECK-NEXT: [[IN_AB:%.*]] = insertelement <2 x i32> [[IN_A]], i32 %b, i64 1
; CHECK-NEXT: [[WAVE_ALL:%.*]] = call <2 x i32> @llvm.genx.GenISA.WaveAll.v2i32(<2 x i32> [[IN_AB]], i8 0, i1 true, i32 0)
; CHECK-NEXT: [[RES_A:%.*]] = extractelement <2 x i32> [[WAVE_ALL]], i64 0
; CHECK-NEXT: [[RES_B:%.*]] = extractelement <2 x i32> [[WAVE_ALL]], i64 1
  %res_a = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
  %res_b = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %b, i8 0, i1 true, i32 0)
; CHECK-NEXT: %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 [[RES_A]], i8 0, i1 true, i32 0)
  %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_a, i8 0, i1 true, i32 0)
; CHECK: %join_b_c = add i32 [[RES_B]], %res_c
  %join_b_c = add i32 %res_b, %res_c
  ret i32 %join_b_c
}

; The source WaveAll is not part of the merge list being built, so merging still happens.
define i32 @test_chain_outside_merge_list(i32 %a, i32 %c) {
; CHECK-LABEL: @test_chain_outside_merge_list(
; CHECK: %res_pre = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
  %res_pre = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %a, i8 0, i1 true, i32 0)
; CHECK-NEXT: [[IN_PRE:%.*]] = insertelement <2 x i32> undef, i32 %res_pre, i64 0
; CHECK-NEXT: [[IN_PRE_C:%.*]] = insertelement <2 x i32> [[IN_PRE]], i32 %c, i64 1
; CHECK-NEXT: [[WAVE_ALL:%.*]] = call <2 x i32> @llvm.genx.GenISA.WaveAll.v2i32(<2 x i32> [[IN_PRE_C]], i8 0, i1 true, i32 0)
; CHECK-NEXT: [[RES_B:%.*]] = extractelement <2 x i32> [[WAVE_ALL]], i64 0
; CHECK-NEXT: [[RES_C:%.*]] = extractelement <2 x i32> [[WAVE_ALL]], i64 1
; CHECK-NOT: call i32 @llvm.genx.GenISA.WaveAll.i32
  %res_b = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %res_pre, i8 0, i1 true, i32 0)
  %res_c = call i32 @llvm.genx.GenISA.WaveAll.i32(i32 %c, i8 0, i1 true, i32 0)
; CHECK: %join_b_c = add i32 [[RES_B]], [[RES_C]]
  %join_b_c = add i32 %res_b, %res_c
  ret i32 %join_b_c
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare i32 @llvm.genx.GenISA.WaveAll.i32(i32, i8, i1, i32) #0

attributes #0 = { convergent inaccessiblememonly nounwind }
