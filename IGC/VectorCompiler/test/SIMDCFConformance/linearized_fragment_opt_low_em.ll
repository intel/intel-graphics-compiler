;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

; COM: SIMD CF Conformance should restore actual SIMD CF and apply
; COM: linearized fragment optimization: move code into
; COM: SIMD CF block that will be created.
; COM: Note that there is only one BB in original IR.

; COM: Current test checks correct work in case when instruction
; COM: that is being moved doesn't work with real EM:
; COM: lowered one should be placed here.

define dllexport spir_kernel void @test_kernel(i32 %0) {
  %old_value = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 0)
  %goto_cond = icmp ult <32 x i32> %old_value, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %newEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %newRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %branchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
; COM: branch for restored SIMD CF
; CHECK: br i1 %branchCond, label %cond_ev_true_split, label %cond_ev_false_split

; COM: optimized fragment: changed value should be calculated here.
; COM: active_lanes_value also should be placed here
; COM; and it should use getEM to lower EM
; CHECK-LABEL: cond_ev_false_split:
; CHECK: %getEM = call <32 x i1> @llvm.genx.simdcf.get.em.v32i1(<32 x i1> %newEM)
; CHECK: %active_lanes_value = zext <32 x i1> %getEM to <32 x i32>
; CHECK: %changed_value = select <32 x i1> %newEM, <32 x i32> %active_lanes_value, <32 x i32> %old_value
; CHECK: br label %cond_ev_true_split

  %active_lanes_value = zext <32 x i1> %newEM to <32 x i32>
  %changed_value = select <32 x i1> %newEM, <32 x i32> %active_lanes_value, <32 x i32> %old_value
  %final_value = select i1 %branchCond, <32 x i32> %old_value, <32 x i32> %changed_value

; COM: restored JP
; CHECK-LABEL: cond_ev_true_split:
; CHECK: %optimized_sel = phi <32 x i32> [ %old_value, %1 ], [ %changed_value, %cond_ev_false_split ]
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM, <32 x i1> %newRM)
  ret void
}

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #2
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
declare <32 x i32> @llvm.genx.oword.ld.v32i32(i32, i32, i32)
