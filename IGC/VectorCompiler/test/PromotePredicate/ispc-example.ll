;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromotePredicate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -logical-ops-threshold=2 -S < %s | FileCheck %s

; CHECK-LABEL: f_f
; CHECK-DAG: [[LESSEQUAL_A_LOAD_widened:%.*]] = sext <8 x i1> %lessequal_a_load_ to <8 x i16>
; CHECK-DAG: [[EQUAL_A_LOAD5_widened:%.*]] = sext <8 x i1> %equal_a_load5_ to <8 x i16>
; CHECK-DAG: [[LOGICAL_AND_promoted:%.*]] = and <8 x i16> [[LESSEQUAL_A_LOAD_widened]], [[EQUAL_A_LOAD5_widened]]
; CHECK-DAG: %0 = icmp ne <8 x i16> [[LOGICAL_AND_promoted]], zeroinitializer
; CHECK-DAG: [[RETURNED_LANES_MEMORY_0_promoted:%.*]] = phi <8 x i16> [ [[LOGICAL_AND_promoted]], %safe_if_run_true.safe_if_after_true_crit_edge ], [ zeroinitializer, %allocas.safe_if_after_true_crit_edge ]
; CHECK-DAG: [[NEG_RETURNED_LANES_promoted:%.*]] = xor <8 x i16> [[RETURNED_LANES_MEMORY_0_promoted]], <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
; CHECK-DAG: %1 = icmp ne <8 x i16> [[NEG_RETURNED_LANES_promoted]], zeroinitializer

declare void @llvm.genx.svm.scatter.v8i1.v8i64.v8f32(<8 x i1>, i32, <8 x i64>, <8 x float>)
declare i1 @llvm.genx.any.v8i1(<8 x i1>)
declare <8 x float> @llvm.genx.svm.block.ld.unaligned.v8f32.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v8f32(i64, <8 x float>)

define dllexport spir_kernel void @f_f(float* nocapture %RET, float* %aFOO, i64 %privBase) {
allocas:
  %svm_ld_ptrtoint = ptrtoint float* %aFOO to i64
  %aFOO_load_ptr2int_2void2021_masked_load22 = call <8 x float> @llvm.genx.svm.block.ld.unaligned.v8f32.i64(i64 %svm_ld_ptrtoint)
  %lessequal_a_load_ = fcmp ole <8 x float> %aFOO_load_ptr2int_2void2021_masked_load22, <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
  %equal_a_load5_ = fcmp oeq <8 x float> %aFOO_load_ptr2int_2void2021_masked_load22, <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>
  %logical_and = and <8 x i1> %lessequal_a_load_, %equal_a_load5_
  %v.i = call i1 @llvm.genx.any.v8i1(<8 x i1> %logical_and)
  %ptr_to_int.i.i31 = ptrtoint float* %RET to i64
  %base.i.i32 = insertelement <8 x i64> undef, i64 %ptr_to_int.i.i31, i32 0
  %shuffle.i.i33 = shufflevector <8 x i64> %base.i.i32, <8 x i64> undef, <8 x i32> zeroinitializer
  %new_offsets.i.i34 = add <8 x i64> %shuffle.i.i33, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28>
  br i1 %v.i, label %safe_if_run_true, label %allocas.safe_if_after_true_crit_edge

allocas.safe_if_after_true_crit_edge:
  br label %safe_if_after_true

safe_if_after_true:
  %returned_lanes_memory.0 = phi <8 x i1> [ %logical_and, %safe_if_run_true.safe_if_after_true_crit_edge ], [ zeroinitializer, %allocas.safe_if_after_true_crit_edge ]
  %"~returned_lanes" = xor <8 x i1> %returned_lanes_memory.0, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  call void @llvm.genx.svm.scatter.v8i1.v8i64.v8f32(<8 x i1> %"~returned_lanes", i32 0, <8 x i64> %new_offsets.i.i34, <8 x float> zeroinitializer)
  ret void

safe_if_run_true:
  call void @llvm.genx.svm.scatter.v8i1.v8i64.v8f32(<8 x i1> %logical_and, i32 0, <8 x i64> %new_offsets.i.i34, <8 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %v.i30 = bitcast <8 x i1> %logical_and to i8
  %"equal__old_mask|returned_lanes" = icmp eq i8 %v.i30, -1
  br i1 %"equal__old_mask|returned_lanes", label %do_return, label %safe_if_run_true.safe_if_after_true_crit_edge

safe_if_run_true.safe_if_after_true_crit_edge:
  br label %safe_if_after_true

do_return:
  ret void
}
