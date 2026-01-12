;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLCECalculation -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

; CHECK-DAG: ![[#ARG:]] = !{i32 0, i32 0, i32 4, i1 false}

define dllexport spir_kernel void @kernel_1(i32 %N) local_unnamed_addr #0 {
  ; for (int i = 0; i < N; ++i)
entry:
  %cmp4.not = icmp eq i32 %N, 0
  br i1 %cmp4.not, label %for.end, label %for.body.lr.ph

for.body.lr.ph:
  %wide.trip.count = zext i32 %N to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond.not = icmp eq i64 %indvars.iv.next, %wide.trip.count
  ; CHECK-DAG: ![[#LCE_1:]] = !{float 1.000000e+00, ![[#ARG]], float 0.000000e+00}
  ; CHECK-DAG: br i1 %exitcond.not, label %for.end.loopexit, label %for.body, !vc.lce ![[#LCE_1]]
  br i1 %exitcond.not, label %for.end.loopexit, label %for.body

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_2(i32 %N) local_unnamed_addr #0 {
  ; for (int i = 0; i < (-N - 12) * 20; i += 4)
entry:
  %sub = sub i32 -12, %N
  %mul = mul i32 %sub, 20
  %cmp4.not = icmp eq i32 %mul, 0
  br i1 %cmp4.not, label %for.end, label %for.body.lr.ph

for.body.lr.ph:
  %0 = zext i32 %mul to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 4
  %cmp = icmp ult i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_2:]] = !{float -5.000000e+00, ![[#ARG]], float -6.000000e+01}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_2]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_3(i32 %N) local_unnamed_addr #0 {
  ; for (int i = 256; i > (N - 12) / 4 + 19; i -= 4)
entry:
  %sub = add i32 %N, -12
  %div = lshr i32 %sub, 2
  %add = add nuw nsw i32 %div, 19
  %cmp4 = icmp ult i32 %sub, 960
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = zext i32 %add to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 256, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nsw i64 %indvars.iv, -4
  %cmp = icmp ugt i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_3:]] = !{float -6.250000e-02, ![[#ARG]], float 6.000000e+01}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_3]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_4(i32 %N) local_unnamed_addr #0 {
  ; for (int i = N; i < 2 * N + 10; i -= 2)
entry:
  %mul = shl i32 %N, 1
  %add = add i32 %mul, 10
  %cmp3 = icmp ugt i32 %add, %N
  br i1 %cmp3, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  br label %for.body

for.body:
  %i.04 = phi i32 [ %N, %for.body.lr.ph ], [ %sub, %for.body ]
  %sub = add nsw i32 %i.04, -2
  %cmp = icmp ult i32 %sub, %add
  ; CHECK-DAG: ![[#LCE_4:]] = !{float -5.000000e-01, ![[#ARG]], float -5.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_4]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_5(i32 %N) local_unnamed_addr #0 {
  ; for (int i = 30; i < (int32_t)N / 4; i += 4)
entry:
  %div = sdiv i32 %N, 4
  %cmp4 = icmp sgt i32 %N, 4
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = sext i32 %div to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 30, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 4
  %cmp = icmp slt i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_5:]] = !{float 6.250000e-02, ![[#ARG]], float -7.500000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_5]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_6(i32 %N) local_unnamed_addr #0 {
  ; for (int i = 0; i < (uint32_t)N / 4; i += 4)
entry:
  %div = udiv i32 %N, 4
  %cmp4 = icmp sgt i32 %N, 4
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  %0 = sext i32 %div to i64
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ 0, %for.body.lr.ph ], [ %indvars.iv.next, %for.body ]
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 4
  %cmp = icmp slt i64 %indvars.iv.next, %0
  ; CHECK-DAG: ![[#LCE_6:]] = !{float 6.250000e-02, ![[#ARG]], float 0.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_6]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

attributes #0 = { nofree noinline norecurse nosync nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
