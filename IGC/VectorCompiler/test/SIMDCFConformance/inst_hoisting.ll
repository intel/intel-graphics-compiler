;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

; COM: SIMD CF Conformance pass should hoist instructions that are located
; COM: after goto to ensure conformance. Also it should move down instructions
; COM: that are located before join.

define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1, i32 %2) {
; CHECK-LABEL: entry:
; CHECK: tail call void @llvm.genx.oword.st.v32i32(i32 %1, i32 0, <32 x i32> %old_value)
; CHECK-NEXT: tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1
entry:
  %old_value = tail call <32 x i32> @llvm.genx.oword.ld.v32i32(i32 0, i32 %0, i32 0)
  %goto_cond = icmp ult <32 x i32> %old_value, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %goto_cond)
  %newEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %newRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %branchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
  tail call void @llvm.genx.oword.st.v32i32(i32 %1, i32 0, <32 x i32> %old_value)
  br i1 %branchCond, label %join_point, label %simd_block

simd_block:
  %active_lanes_value = add <32 x i32> %old_value, <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>
  %changed_value = select <32 x i1> %newEM, <32 x i32> %active_lanes_value, <32 x i32> %old_value
  br label %join_point

; CHECK-LABEL: join_point:
; CHECK: tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1
; COM: SIMD CF Conf adds EM EVs for missing values for proper LR interference check
; CHECK-NEXT: extractvalue { <32 x i1>, i1 } %join_struct, 0
; CHECK-NEXT: tail call void @llvm.genx.oword.st.v32i32(i32 %2, i32 0, <32 x i32> %optimized_sel)
join_point:
  %optimized_sel = phi <32 x i32> [ %old_value, %entry ], [ %changed_value, %simd_block ]
  tail call void @llvm.genx.oword.st.v32i32(i32 %2, i32 0, <32 x i32> %optimized_sel)
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM, <32 x i1> %newRM)
  ret void
}

declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #2
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
declare <32 x i32> @llvm.genx.oword.ld.v32i32(i32, i32, i32)
declare void @llvm.genx.oword.st.v32i32(i32, i32, <32 x i32>)
