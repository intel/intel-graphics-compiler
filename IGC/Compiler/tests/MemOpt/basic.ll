;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i32* %dst, i32* %src) {
entry:
  %0 = load i32, i32* %src, align 4
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = load i32, i32* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = load i32, i32* %arrayidx2, align 4
  store i32 %0, i32* %dst, align 4
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  store i32 %1, i32* %arrayidx4, align 4
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  store i32 %2, i32* %arrayidx5, align 4
  ret void
}

 ; CHECK-LABEL: define void @f0
 ; CHECK: %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK: %1 = load <3 x i32>, <3 x i32>* %0, align 4
 ; CHECK: %2 = bitcast i32* %dst to <3 x i32>*
 ; CHECK: store <3 x i32> %1, <3 x i32>* %2, align 4
 ; CHECK: ret void


 define void @f1(i32* %dst, i32* %src) {
 entry:
   %0 = load i32, i32* %src, align 4
   store i32 %0, i32* %dst, align 4
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = load i32, i32* %arrayidx2, align 4
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 1
   store i32 %1, i32* %arrayidx3, align 4
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = load i32, i32* %arrayidx4, align 4
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
   store i32 %2, i32* %arrayidx5, align 4
   ret void
 }

 ; Without 'noalias' attribute, '%dst' may be alias to '%src'. Hence, we cannot
 ; merge loads/stores interleaved.

 ; CHECK-LABEL: define void @f1
 ; CHECK: %0 = load i32, i32* %src, align 4
 ; CHECK: store i32 %0, i32* %dst, align 4
 ; CHECK: %arrayidx2 = getelementptr inbounds i32, i32* %src, i32 1
 ; CHECK: %1 = load i32, i32* %arrayidx2, align 4
 ; CHECK: %arrayidx3 = getelementptr inbounds i32, i32* %dst, i32 1
 ; CHECK: store i32 %1, i32* %arrayidx3, align 4
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %src, i32 2
 ; CHECK: %2 = load i32, i32* %arrayidx4, align 4
 ; CHECK: %arrayidx5 = getelementptr inbounds i32, i32* %dst, i32 2
 ; CHECK: store i32 %2, i32* %arrayidx5, align 4
 ; CHECK: ret void


 define void @f2(i32* noalias %dst, i32* noalias %src) {
 entry:
   %0 = load i32, i32* %src, align 4
   store i32 %0, i32* %dst, align 4
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = load i32, i32* %arrayidx2, align 4
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 1
   store i32 %1, i32* %arrayidx3, align 4
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = load i32, i32* %arrayidx4, align 4
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
   store i32 %2, i32* %arrayidx5, align 4
   ret void
 }

 ; CHECK-LABEL: define void @f2
 ; CHECK: %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK: %1 = load <3 x i32>, <3 x i32>* %0, align 4
 ; CHECK: %2 = bitcast i32* %dst to <3 x i32>*
 ; CHECK: store <3 x i32> %1, <3 x i32>* %2, align 4
 ; CHECK: ret void


 define void @f3(i32 %i, i32 %j, i32* noalias %dst, i32* noalias %src) {
 entry:
   %mul = mul nsw i32 %j, %i
   %idxprom = sext i32 %mul to i64
   %arrayidx = getelementptr inbounds i32, i32* %src, i64 %idxprom
   %0 = load i32, i32* %arrayidx, align 4
   %add1 = add nsw i32 %j, %i
   %idxprom3 = sext i32 %add1 to i64
   %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 %idxprom3
   store i32 %0, i32* %arrayidx4, align 4
   %add6 = add nsw i32 %mul, 1
   %idxprom7 = sext i32 %add6 to i64
   %arrayidx8 = getelementptr inbounds i32, i32* %src, i64 %idxprom7
   %1 = load i32, i32* %arrayidx8, align 4
   %add10 = add nsw i32 %add1, 1
   %idxprom11 = sext i32 %add10 to i64
   %arrayidx12 = getelementptr inbounds i32, i32* %dst, i64 %idxprom11
   store i32 %1, i32* %arrayidx12, align 4
   %add14 = add nsw i32 %mul, 2
   %idxprom15 = sext i32 %add14 to i64
   %arrayidx16 = getelementptr inbounds i32, i32* %src, i64 %idxprom15
   %2 = load i32, i32* %arrayidx16, align 4
   %add18 = add nsw i32 %add1, 2
   %idxprom19 = sext i32 %add18 to i64
   %arrayidx20 = getelementptr inbounds i32, i32* %dst, i64 %idxprom19
   store i32 %2, i32* %arrayidx20, align 4
   ret void
 }

 ; CHECK-LABEL: define void @f3
 ; CHECK: %mul = mul nsw i32 %j, %i
 ; CHECK: %arrayidx = getelementptr inbounds i32, i32* %src, i32 %mul
 ; CHECK: %0 = bitcast i32* %arrayidx to <3 x i32>*
 ; CHECK: %1 = load <3 x i32>, <3 x i32>* %0, align 4
 ; CHECK: %add1 = add nsw i32 %j, %i
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %dst, i32 %add1
 ; CHECK: %2 = bitcast i32* %arrayidx4 to <3 x i32>*
 ; CHECK: store <3 x i32> %1, <3 x i32>* %2, align 4
 ; CHECK: ret void


 define void @f4(i32 %i, i32 %j, i32* noalias %dst, i32* noalias %src) {
 entry:
   %mul = mul nsw i32 %j, %i
   %idxprom = sext i32 %mul to i64
   %arrayidx = getelementptr inbounds i32, i32* %src, i64 %idxprom
   %0 = load i32, i32* %arrayidx, align 4
   %add1 = add nsw i32 %j, %i
   %idxprom3 = sext i32 %add1 to i64
   %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 %idxprom3
   store i32 %0, i32* %arrayidx4, align 4
   fence seq_cst
   %add6 = add nsw i32 %mul, 1
   %idxprom7 = sext i32 %add6 to i64
   %arrayidx8 = getelementptr inbounds i32, i32* %src, i64 %idxprom7
   %1 = load i32, i32* %arrayidx8, align 4
   %add10 = add nsw i32 %add1, 1
   %idxprom11 = sext i32 %add10 to i64
   %arrayidx12 = getelementptr inbounds i32, i32* %dst, i64 %idxprom11
   store i32 %1, i32* %arrayidx12, align 4
   %add14 = add nsw i32 %mul, 2
   %idxprom15 = sext i32 %add14 to i64
   %arrayidx16 = getelementptr inbounds i32, i32* %src, i64 %idxprom15
   %2 = load i32, i32* %arrayidx16, align 4
   %add18 = add nsw i32 %add1, 2
   %idxprom19 = sext i32 %add18 to i64
   %arrayidx20 = getelementptr inbounds i32, i32* %dst, i64 %idxprom19
   store i32 %2, i32* %arrayidx20, align 4
   ret void
 }

 ; As the fence is placed after the first pair of load/store, only the last 2
 ; consecutive loads/stores could be merged.

 ; CHECK-LABEL: define void @f4
 ; CHECK: %mul = mul nsw i32 %j, %i
 ; CHECK: %arrayidx = getelementptr inbounds i32, i32* %src, i32 %mul
 ; CHECK: %0 = load i32, i32* %arrayidx, align 4
 ; CHECK: %add1 = add nsw i32 %j, %i
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %dst, i32 %add1
 ; CHECK: store i32 %0, i32* %arrayidx4, align 4
 ; CHECK: fence seq_cst
 ; CHECK: %add6 = add nsw i32 %mul, 1
 ; CHECK: %arrayidx8 = getelementptr inbounds i32, i32* %src, i32 %add6
 ; CHECK: %1 = bitcast i32* %arrayidx8 to [[i64_TYPE:i64|<2 x i32>]]*
 ; CHECK: %2 = load [[i64_TYPE]], [[i64_TYPE]]* %1, align 4
 ; CHECK: %add10 = add nsw i32 %add1, 1
 ; CHECK: %arrayidx12 = getelementptr inbounds i32, i32* %dst, i32 %add10
 ; CHECK: %3 = bitcast i32* %arrayidx12 to [[i64_TYPE]]*
 ; CHECK: store [[i64_TYPE]] %2, [[i64_TYPE]]* %3, align 4
 ; CHECK: ret void


 define void @f5(i32* noalias %dst, i32* noalias %src) {
 entry:
   %0 = load i32, i32* %src, align 4
   %arrayidx1 = getelementptr inbounds i32, i32* %dst, i64 3
   store i32 %0, i32* %arrayidx1, align 4
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = load i32, i32* %arrayidx2, align 4
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 2
   store i32 %1, i32* %arrayidx3, align 4
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = load i32, i32* %arrayidx4, align 4
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 1
   store i32 %2, i32* %arrayidx5, align 4
   %arrayidx6 = getelementptr inbounds i32, i32* %src, i64 3
   %3 = load i32, i32* %arrayidx6, align 4
   store i32 %3, i32* %dst, align 4
   ret void
 }

 ; CHECK-LABEL: define void @f5
 ; CHECK: %0 = bitcast i32* %src to <4 x i32>*
 ; CHECK: %1 = load <4 x i32>, <4 x i32>* %0, align 4
 ; CHECK: %2 = shufflevector <4 x i32> %1, <4 x i32> undef, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
 ; CHECK: %3 = bitcast i32* %dst to <4 x i32>*
 ; CHECK: store <4 x i32> %2, <4 x i32>* %3, align 4
 ; CHECK: ret void

!igc.functions = !{!0, !3, !4, !5, !6, !7}

!0 = !{void (i32*, i32*)* @f0, !1}
!3 = !{void (i32*, i32*)* @f1, !1}
!4 = !{void (i32*, i32*)* @f2, !1}
!5 = !{void (i32, i32, i32*, i32*)* @f3, !1}
!6 = !{void (i32, i32, i32*, i32*)* @f4, !1}
!7 = !{void (i32*, i32*)* @f5, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
