;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define dllexport spir_kernel void @kernel_1() #0 {
  ; for (int i = 0; i < 256; ++i)
entry:
  br label %for.body

for.body:
  %ind.var = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %inc = add nuw nsw i32 %ind.var, 1
  %exitcond.not = icmp eq i32 %inc, 256
  ; CHECK-DAG: ![[#K_1:]] = !{float 2.560000e+02}
  ; CHECK-DAG: br i1 %exitcond.not, label %for.end, label %for.body, !vc.lce ![[#K_1]]
  br i1 %exitcond.not, label %for.end, label %for.body

for.end:
  ret void
}

declare i32 @read_i32()
define dllexport spir_kernel void @kernel_2() #0 {
  ; for (int i = 256; i > 1; i -= 2) { ... if(...) break ...}
entry:
  br label %for.body

for.body:
  %indvars.iv = phi i32 [ 256, %entry ], [ %dec, %if.end ]
  %break.val = call i32 @read_i32()
  %cmp = icmp eq i32 %break.val, 42
  br i1 %cmp, label %for.end, label %if.end

if.end:
  %dec = add nsw i32 %indvars.iv, -2
  %exitcond = icmp ugt i32 %indvars.iv, 1
  ; CHECK-DAG: ![[#K_2:]] = !{float 1.275000e+02}
  ; CHECK-DAG: br i1 %exitcond, label %for.body, label %for.end, !vc.lce ![[#K_2]]
  br i1 %exitcond, label %for.body, label %for.end

for.end:
  ret void
}

attributes #0 = { nofree noinline norecurse nosync nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
