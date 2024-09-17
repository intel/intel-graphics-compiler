;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; CHECK-DAG: ![[#ARG:]] = !{i32 0, i32 0, i32 4, i1 false}

define dllexport spir_kernel void @kernel_1(float %F) local_unnamed_addr #0 {
  ; for (int i = 0; i < int(F / 2 + 10); i += 4)
entry:
  %div = fdiv float %F, 2.000000e+00
  %add = fadd float %div, 1.000000e+01
  %conv = fptosi float %add to i32
  %cmp4 = icmp sgt i32 %conv, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = zext i32 %conv to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 4
  %cmp = icmp ult i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_1:]] = !{float 1.250000e-01, ![[#ARG]], float 2.500000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_1]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_2(float %F) local_unnamed_addr #0 {
  ; for (int i = 0; i < int(F * 10); i += 4)
entry:
  %mul = fmul float %F, 1.000000e+01
  %conv = fptosi float %mul to i32
  %cmp4 = icmp sgt i32 %conv, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = zext i32 %conv to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 4
  %cmp = icmp ult i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_2:]] = !{float 2.500000e+00, ![[#ARG]], float 0.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_2]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

attributes #0 = { nofree noinline norecurse nosync nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
