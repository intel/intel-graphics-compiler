;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-NOT: !vc.lce

define dllexport spir_kernel void @kernel(i32 addrspace(1)* nocapture %In) local_unnamed_addr #0 {
  ; for (int i = In[1]; i < In[0]; i++)
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %In, i64 1
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %1 = load i32, i32 addrspace(1)* %In, align 4
  %cmp4 = icmp ult i32 %0, %1
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  br label %for.body

for.body:
  %i.05 = phi i32 [ %0, %for.body.lr.ph ], [ %inc, %for.body ]
  %inc = add nuw nsw i32 %i.05, 1
  %exitcond.not = icmp eq i32 %inc, %1
  br i1 %exitcond.not, label %for.end.loopexit, label %for.body

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

attributes #0 = { nofree noinline norecurse nosync nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
