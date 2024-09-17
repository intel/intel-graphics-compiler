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

define dllexport spir_kernel void @kernel_1(i32 addrspace(1)* nocapture readonly %A) local_unnamed_addr #0 {
  ; for (int i = 0; i < A[4]; i += 1)
entry:
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %A, i64 4
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %cmp4.not = icmp eq i32 %0, 0
  br i1 %cmp4.not, label %for.end, label %for.body.preheader

for.body.preheader:
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %2 = zext i32 %1 to i64
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp = icmp ult i64 %indvars.iv.next, %2
  ; CHECK-DAG: ![[#ARG_1:]] = !{i32 0, i32 16, i32 8, i1 true}
  ; CHECK-DAG: ![[#LCE_1:]] = !{float 1.000000e+00, ![[#ARG_1]], float 0.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_1]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_2(i32 addrspace(1)* nocapture readonly %B, i32 addrspace(3)* nocapture readonly %A) local_unnamed_addr #0 {
  ; for (int i = 0; i < arg[4][6]; i += 1)
entry:
  %arg = bitcast i32 addrspace(3)* %A to [10 x [20 x i32]] addrspace(3)*
  %arrayidx = getelementptr [10 x [20 x i32]], [10 x [20 x i32]] addrspace(3)*  %arg, i64 0, i64 4, i64 6
  %0 = load i32, i32 addrspace(3)* %arrayidx, align 4
  %cmp4.not = icmp eq i32 %0, 0
  br i1 %cmp4.not, label %for.end, label %for.body.preheader

for.body.preheader:
  br label %for.body

for.body:
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %1 = load i32, i32 addrspace(3)* %arrayidx, align 4
  %2 = zext i32 %1 to i64
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %cmp = icmp ult i64 %indvars.iv.next, %2
  ; CHECK-DAG: ![[#ARG_2:]] = !{i32 1, i32 344, i32 8, i1 true}
  ; CHECK-DAG: ![[#LCE_2:]] = !{float 1.000000e+00, ![[#ARG_2]], float 0.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_2]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

define dllexport spir_kernel void @kernel_3(i32 addrspace(1)* nocapture %In) local_unnamed_addr #0 {
  ; for (int i = 48 * In[0] - 16; i < In[0]; i -= 2)
entry:
  %0 = load i32, i32 addrspace(1)* %In, align 4
  %mul = mul i32 %0, 48
  %sub = add i32 %mul, -16
  %cmp4 = icmp ult i32 %sub, %0
  br i1 %cmp4, label %for.body.lr.ph, label %for.end

for.body.lr.ph:
  br label %for.body

for.body:
  %i.05 = phi i32 [ %sub, %for.body.lr.ph ], [ %sub4, %for.body ]
  %sub4 = add nsw i32 %i.05, -2
  %cmp = icmp ult i32 %sub4, %0
  ; CHECK-DAG: ![[#ARG_3:]] = !{i32 0, i32 0, i32 8, i1 true}
  ; CHECK-DAG: ![[#LCE_3:]] = !{float 2.350000e+01, ![[#ARG_3]], float -8.000000e+00}
  ; CHECK-DAG: br i1 %cmp, label %for.body, label %for.end.loopexit, !vc.lce ![[#LCE_3]]
  br i1 %cmp, label %for.body, label %for.end.loopexit

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}
