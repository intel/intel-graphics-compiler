;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXDeadVectorRemoval -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @two_selects
define <16 x i32> @two_selects(<16 x i32> %arg1, <16 x i1> %pred1, <16 x i1> %pred2) {
; CHECK-NOT: %bypass = select <16 x i1> %pred1, <16 x i32> %arg1, <16 x i32>  undef
  %bypass = select <16 x i1> %pred1, <16 x i32> %arg1, <16 x i32>  undef
; CHECK-NOT: %bypass2 = select <16 x i1> %pred2, <16 x i32> %bypass, <16 x i32>  undef
  %bypass2 = select <16 x i1> %pred2, <16 x i32>  undef, <16 x i32> %bypass
; CHECK-NEXT: ret <16 x i32> %arg1
  ret <16 x i32> %bypass2
}


declare { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #1
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>) #1

; CHECK-LABEL: @select_em
define dllexport void @select_em(<32 x i32>* %a, <32 x i1> %b) {
entry:
  %stack_data = load <32 x i32>, <32 x i32>* %a
  br label %lbl

lbl:                                              ; preds = %lbl, %entry
  %goto.call = call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer, <32 x i1> %b)
  %EM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto.call, 0
  %RM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto.call, 1
  %Pred = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto.call, 2
  br i1 %Pred, label %exit, label %lbl

; CHECK-LABEL: exit
exit:                                             ; preds = %lbl
  %const.cast = bitcast i32 1244252123 to <32 x i1>
; CHECK: %first_select = select <32 x i1> %EM, <32 x i32> %stack_data, <32 x i32> undef
  %first_select = select <32 x i1> %EM, <32 x i32> %stack_data, <32 x i32> undef
; CHECK-NEXT: %second_select = select <32 x i1> %EM, <32 x i32> undef, <32 x i32> %first_select
  %second_select = select <32 x i1> %EM, <32 x i32> undef, <32 x i32> %first_select
  store <32 x i32> %second_select, <32 x i32>* %a
  %join_call = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %EM, <32 x i1> %RM)
  ret void
}
