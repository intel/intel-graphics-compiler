;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --implicit-check-not="!vc.lce" %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --implicit-check-not="!vc.lce" %s

; CHECK-LABEL: define dllexport spir_kernel void @kernel(
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

; CHECK-LABEL: define dllexport spir_kernel void @kernel_fdiv_zero(
; CHECK: %div = fdiv float %F, 0.000000e+00
define dllexport spir_kernel void @kernel_fdiv_zero(float %F) local_unnamed_addr #0 {
  ; for (long i = 0; i < (int)(F / 0.0f); i++)
entry:
  %div = fdiv float %F, 0.000000e+00
  %conv = fptosi float %div to i32
  %cmp4 = icmp sgt i32 %conv, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %wide = zext i32 %conv to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp = icmp ult i64 %indvars.iv.next, %wide
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: define dllexport spir_kernel void @kernel_fdiv_subnormal(
; CHECK: %div = fdiv float %F, {{(0x36A0000000000000|1\.401300e-45)}}
define dllexport spir_kernel void @kernel_fdiv_subnormal(float %F) local_unnamed_addr #0 {
  ; for (long i = 0; i < (int)(F / 0x1p-149f); i++)
entry:
  %div = fdiv float %F, 0x36A0000000000000
  %conv = fptosi float %div to i32
  %cmp4 = icmp sgt i32 %conv, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %wide = zext i32 %conv to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp = icmp ult i64 %indvars.iv.next, %wide
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: define dllexport spir_kernel void @kernel_udiv_zero(
; CHECK: %div = udiv i32 %N, 0
define dllexport spir_kernel void @kernel_udiv_zero(i32 %N) local_unnamed_addr #0 {
  ; for (unsigned long i = 0; i != (unsigned)(N / 0u); i++)
entry:
  %div = udiv i32 %N, 0
  %cmp4.not = icmp eq i32 %div, 0
  br i1 %cmp4.not, label %for.end, label %for.body.lr.ph

for.body.lr.ph:
  %wide = zext i32 %div to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide
  br i1 %exitcond.not, label %for.end.loopexit, label %for.body

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: define dllexport spir_kernel void @kernel_sdiv_zero(
; CHECK: %div = sdiv i32 %N, 0
define dllexport spir_kernel void @kernel_sdiv_zero(i32 %N) local_unnamed_addr #0 {
  ; for (long i = 0; i < (long)(N / 0); i++)
entry:
  %div = sdiv i32 %N, 0
  %cmp4 = icmp sgt i32 %div, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %wide = sext i32 %div to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp = icmp slt i64 %indvars.iv.next, %wide
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: define dllexport spir_kernel void @kernel_nonconst_step(
define dllexport spir_kernel void @kernel_nonconst_step(i32 %N, i32 %S) local_unnamed_addr #0 {
  ; for (int i = 0; i < N; i += S)
entry:
  %cmp4 = icmp sgt i32 %N, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  br label %for.body

for.body:
  %i.05 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %for.body ]
  %inc = add nsw i32 %i.05, %S
  %cmp = icmp slt i32 %inc, %N
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: define dllexport spir_kernel void @kernel_step_overflow(
define dllexport spir_kernel void @kernel_step_overflow(i64 %N) local_unnamed_addr #0 {
  ; for (long i = 0; i < N; i += (1L << 32) + 1)
entry:
  %cmp4 = icmp sgt i64 %N, 0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nsw i64 %indvars.iv, 4294967297
  %cmp = icmp slt i64 %indvars.iv.next, %N
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

attributes #0 = { nofree noinline norecurse nosync nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
