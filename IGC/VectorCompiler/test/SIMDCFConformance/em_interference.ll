;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -march=genx64 -mcpu=Gen9 -GenXModule -GenXLateSimdCFConformanceWrapper -S < %s | FileCheck %s

; COM: This test checks if GenXLateSimdCFConformance is able to fix
; COM: simple EM interference issues.
; COM: %EV2 and %EV4 are interfering Execution Mask values. Checking that
; COM: %EV4 is removed and all its uses are substituted with %EV2

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dllexport spir_kernel void @bitonicSort(i32 %numElements) "CMGenxMain" {
entry:
  br label %preheader
; CHECK-LABEL: preheader:
; CHECK: %EM.local.0 = phi <32 x i1> [ %EV2, %crit_edge_inc ]
preheader:                                        ; preds = %crit_edge_inc, %entry
  %A.0 = phi <16 x i32> [ %EV3, %crit_edge_inc ], [ undef, %entry ]
  %EM.local.0 = phi <32 x i1> [ %EV4, %crit_edge_inc ], [ <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, %entry ]
  %stride = phi i32 [ %inc_outer, %crit_edge_inc ], [ 0, %entry ]
  br label %body

; CHECK-LABEL: body:
body:                                             ; preds = %crit_edge_body, %preheader
  %A.1 = phi <16 x i32> [ %A.0, %preheader ], [ %EV1, %crit_edge_body ]
  %EM.local.1 = phi <32 x i1> [ %EM.local.0, %preheader ], [ %EV2, %crit_edge_body ]
  %vIndex = phi i32 [ 0, %preheader ], [ %inc, %crit_edge_body ]
  %call = tail call spir_func { <16 x i32>, <32 x i1> } @swap(<16 x i32> %A.1, <16 x i32> undef, <32 x i1> %EM.local.1)
  %EV1 = extractvalue { <16 x i32>, <32 x i1> } %call, 0
  %EV2 = extractvalue { <16 x i32>, <32 x i1> } %call, 1
  %inc = add nuw i32 %vIndex, 1
  %exitcond = icmp eq i32 %inc, %numElements
  br i1 %exitcond, label %for_inc, label %crit_edge_body

crit_edge_body:                                   ; preds = %body
  br label %body

; CHECK-LABEL: for_inc:
; CHECK-NOT: extractvalue { <16 x i32>, <32 x i1> } %call, 1
for_inc:                                          ; preds = %body
  %EV3 = extractvalue { <16 x i32>, <32 x i1> } %call, 0
  %EV4 = extractvalue { <16 x i32>, <32 x i1> } %call, 1
  %inc_outer = add nuw i32 %stride, 1
  %exitcond8 = icmp eq i32 %inc_outer, %numElements
  br i1 %exitcond8, label %end, label %crit_edge_inc

crit_edge_inc:                                    ; preds = %for_inc
  br label %preheader

end:                                              ; preds = %for_inc
  ret void
}

; COM: @swap is need only for correct detection of SIMD CF. Nothing to be checked here
define { <16 x i32>, <32 x i1> } @swap(<16 x i32> %arg, <16 x i32> %arg1, <32 x i1> %EM.in) {
  %cmp = icmp ugt <16 x i32> %arg, %arg1
  %goto = tail call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1> %EM.in, <16 x i1> zeroinitializer, <16 x i1> %cmp)
  %goto.extractcond = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto, 2
  %goto.extractrm = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto, 1
  %goto.extractem = extractvalue { <32 x i1>, <16 x i1>, i1 } %goto, 0
  br i1 %goto.extractcond, label %entry.cond_ev_true_split_crit_edge, label %cond_ev_false_split
entry.cond_ev_true_split_crit_edge:
  br label %cond_ev_true_split
cond_ev_false_split:
  %1 = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %goto.extractem, i32 0)
  %ref.load2.simdcfpred = select <16 x i1> %1, <16 x i32> %arg1, <16 x i32> %arg
  br label %cond_ev_true_split
cond_ev_true_split:
  %optimized_sel = phi <16 x i32> [ %arg, %entry.cond_ev_true_split_crit_edge ], [ %ref.load2.simdcfpred, %cond_ev_false_split ]
  %join = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1> %goto.extractem, <16 x i1> %goto.extractrm)
  %join.extractem = extractvalue { <32 x i1>, i1 } %join, 0
  br label %split_for_join
split_for_join:
  %insertvalue2 = insertvalue { <16 x i32>, <32 x i1> } undef, <16 x i32> %optimized_sel, 0
  %insertvalue = insertvalue { <16 x i32>, <32 x i1> } %insertvalue2, <32 x i1> %join.extractem, 1
  ret { <16 x i32>, <32 x i1> } %insertvalue
}

declare { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v16i1(<32 x i1>, <16 x i1>, <16 x i1>)
declare <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1>, i32)
declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v16i1(<32 x i1>, <16 x i1>)
