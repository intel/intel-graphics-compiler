;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @f0(i32* %dst, i32* %src) {
entry:
  %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx1, i64 4, i1 true, i32 43)
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx2, i64 4, i1 true, i32 44)
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %0, i64 4, i1 true)
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx4, i32 %1, i64 4, i1 true)
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx5, i32 %2, i64 4, i1 true)
  ret void
}

 ; CHECK-LABEL: define void @f0
 ; CHECK: %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK: %1 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0v3i32.v3i32(<3 x i32>* %0, i64 4, i1 true, <3 x i32> <i32 42, i32 43, i32 44>)
 ; CHECK: %2 = bitcast i32* %dst to <3 x i32>*
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v3i32.v3i32(<3 x i32>* %2, <3 x i32> %1, i64 4, i1 true)
 ; CHECK: ret void

define void @f0_different_predicates(i32* %dst, i32* %src) {
entry:
  %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
  %arrayidx1 = getelementptr inbounds i32, i32* %src, i64 1
  %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx1, i64 4, i1 false, i32 43)
  %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 2
  %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx2, i64 4, i1 true, i32 44)
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %0, i64 4, i1 true)
  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 1
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx4, i32 %1, i64 4, i1 false)
  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx5, i32 %2, i64 4, i1 true)
  ret void
}

 ; with different predicates, we cannot merge all loads. We cannot merge stores.

 ; CHECK-LABEL: define void @f0_different_predicates
 ; CHECK:  %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK:  %1 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0v3i32.v3i32(<3 x i32>* %0, i64 4, i1 true, <3 x i32> <i32 42, i32 undef, i32 44>)
 ; CHECK:  %2 = extractelement <3 x i32> %1, i64 0
 ; CHECK:  %3 = extractelement <3 x i32> %1, i64 2
 ; CHECK:  %arrayidx1 = getelementptr inbounds i32, i32* %src, i32 1
 ; CHECK:  %4 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* nonnull %arrayidx1, i64 4, i1 false, i32 43)
 ; CHECK:  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %2, i64 4, i1 true)
 ; CHECK:  %arrayidx4 = getelementptr inbounds i32, i32* %dst, i32 1
 ; CHECK:  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* nonnull %arrayidx4, i32 %4, i64 4, i1 false)
 ; CHECK:  %arrayidx5 = getelementptr inbounds i32, i32* %dst, i32 2
 ; CHECK:  call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* nonnull %arrayidx5, i32 %3, i64 4, i1 true)
 ; CHECK:  ret void

 define void @f1(i32* %dst, i32* %src) {
 entry:
   %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %0, i64 4, i1 true)
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx2, i64 4, i1 true, i32 43)
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 1
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx3, i32 %1, i64 4, i1 true)
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx4, i64 4, i1 true, i32 44)
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx5, i32 %2, i64 4, i1 true)
   ret void
 }

 ; Without 'noalias' attribute, '%dst' may be alias to '%src'. Hence, we cannot
 ; merge loads/stores interleaved.

 ; CHECK-LABEL: define void @f1
 ; CHECK: %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %0, i64 4, i1 true)
 ; CHECK: %arrayidx2 = getelementptr inbounds i32, i32* %src, i32 1
 ; CHECK: %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* nonnull %arrayidx2, i64 4, i1 true, i32 43)
 ; CHECK: %arrayidx3 = getelementptr inbounds i32, i32* %dst, i32 1
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* nonnull %arrayidx3, i32 %1, i64 4, i1 true)
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %src, i32 2
 ; CHECK: %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* nonnull %arrayidx4, i64 4, i1 true, i32 44)
 ; CHECK: %arrayidx5 = getelementptr inbounds i32, i32* %dst, i32 2
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* nonnull %arrayidx5, i32 %2, i64 4, i1 true)
 ; CHECK: ret void


 define void @f2(i32* noalias %dst, i32* noalias %src) {
 entry:
   %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %0, i64 4, i1 true)
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx2, i64 4, i1 true, i32 43)
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 1
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx3, i32 %1, i64 4, i1 true)
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx4, i64 4, i1 true, i32 44)
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 2
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx5, i32 %2, i64 4, i1 true)
   ret void
 }

 ; CHECK-LABEL: define void @f2
 ; CHECK: %0 = bitcast i32* %src to <3 x i32>*
 ; CHECK: %1 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0v3i32.v3i32(<3 x i32>* %0, i64 4, i1 true, <3 x i32> <i32 42, i32 43, i32 44>)
 ; CHECK: %2 = bitcast i32* %dst to <3 x i32>*
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v3i32.v3i32(<3 x i32>* %2, <3 x i32> %1, i64 4, i1 true)
 ; CHECK: ret void


 define void @f3(i32 %i, i32 %j, i32* noalias %dst, i32* noalias %src) {
 entry:
   %mul = mul nsw i32 %j, %i
   %idxprom = sext i32 %mul to i64
   %arrayidx = getelementptr inbounds i32, i32* %src, i64 %idxprom
   %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx, i64 4, i1 true, i32 42)
   %add1 = add nsw i32 %j, %i
   %idxprom3 = sext i32 %add1 to i64
   %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 %idxprom3
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx4, i32 %0, i64 4, i1 true)
   %add6 = add nsw i32 %mul, 1
   %idxprom7 = sext i32 %add6 to i64
   %arrayidx8 = getelementptr inbounds i32, i32* %src, i64 %idxprom7
   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx8, i64 4, i1 true, i32 43)
   %add10 = add nsw i32 %add1, 1
   %idxprom11 = sext i32 %add10 to i64
   %arrayidx12 = getelementptr inbounds i32, i32* %dst, i64 %idxprom11
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx12, i32 %1, i64 4, i1 true)
   %add14 = add nsw i32 %mul, 2
   %idxprom15 = sext i32 %add14 to i64
   %arrayidx16 = getelementptr inbounds i32, i32* %src, i64 %idxprom15
   %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx16, i64 4, i1 true, i32 44)
   %add18 = add nsw i32 %add1, 2
   %idxprom19 = sext i32 %add18 to i64
   %arrayidx20 = getelementptr inbounds i32, i32* %dst, i64 %idxprom19
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx20, i32 %2, i64 4, i1 true)
   ret void
 }

 ; CHECK-LABEL: define void @f3
 ; CHECK: %mul = mul nsw i32 %j, %i
 ; CHECK: %arrayidx = getelementptr inbounds i32, i32* %src, i32 %mul
 ; CHECK: %0 = bitcast i32* %arrayidx to <3 x i32>*
 ; CHECK: %1 = call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.p0v3i32.v3i32(<3 x i32>* %0, i64 4, i1 true, <3 x i32> <i32 42, i32 43, i32 44>)
 ; CHECK: %add1 = add nsw i32 %j, %i
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %dst, i32 %add1
 ; CHECK: %2 = bitcast i32* %arrayidx4 to <3 x i32>*
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v3i32.v3i32(<3 x i32>* %2, <3 x i32> %1, i64 4, i1 true)
 ; CHECK: ret void


 define void @f4(i32 %i, i32 %j, i32* noalias %dst, i32* noalias %src) {
 entry:
   %mul = mul nsw i32 %j, %i
   %idxprom = sext i32 %mul to i64
   %arrayidx = getelementptr inbounds i32, i32* %src, i64 %idxprom
   %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx, i64 4, i1 true, i32 42)
   %add1 = add nsw i32 %j, %i
   %idxprom3 = sext i32 %add1 to i64
   %arrayidx4 = getelementptr inbounds i32, i32* %dst, i64 %idxprom3
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx4, i32 %0, i64 4, i1 true)
   fence seq_cst
   %add6 = add nsw i32 %mul, 1
   %idxprom7 = sext i32 %add6 to i64
   %arrayidx8 = getelementptr inbounds i32, i32* %src, i64 %idxprom7
   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx8, i64 4, i1 true, i32 43)
   %add10 = add nsw i32 %add1, 1
   %idxprom11 = sext i32 %add10 to i64
   %arrayidx12 = getelementptr inbounds i32, i32* %dst, i64 %idxprom11
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx12, i32 %1, i64 4, i1 true)
   %add14 = add nsw i32 %mul, 2
   %idxprom15 = sext i32 %add14 to i64
   %arrayidx16 = getelementptr inbounds i32, i32* %src, i64 %idxprom15
   %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx16, i64 4, i1 true, i32 44)
   %add18 = add nsw i32 %add1, 2
   %idxprom19 = sext i32 %add18 to i64
   %arrayidx20 = getelementptr inbounds i32, i32* %dst, i64 %idxprom19
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx20, i32 %2, i64 4, i1 true)
   ret void
 }

 ; As the fence is placed after the first pair of load/store, only the last 2
 ; consecutive loads/stores could be merged.

 ; CHECK-LABEL: define void @f4
 ; CHECK: %mul = mul nsw i32 %j, %i
 ; CHECK: %arrayidx = getelementptr inbounds i32, i32* %src, i32 %mul
 ; CHECK: %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx, i64 4, i1 true, i32 42)
 ; CHECK: %add1 = add nsw i32 %j, %i
 ; CHECK: %arrayidx4 = getelementptr inbounds i32, i32* %dst, i32 %add1
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx4, i32 %0, i64 4, i1 true)
 ; CHECK: fence seq_cst
 ; CHECK: %add6 = add nsw i32 %mul, 1
 ; CHECK: %arrayidx8 = getelementptr inbounds i32, i32* %src, i32 %add6
 ; CHECK: %1 = bitcast i32* %arrayidx8 to <2 x i32>*
 ; CHECK: %2 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.{{.*}}(<2 x i32>* %1, i64 4, i1 true, <2 x i32> <i32 43, i32 44>)
 ; CHECK: %add10 = add nsw i32 %add1, 1
 ; CHECK: %arrayidx12 = getelementptr inbounds i32, i32* %dst, i32 %add10
 ; CHECK: %3 = bitcast i32* %arrayidx12 to <2 x i32>*
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.{{.*}}(<2 x i32>* %3, <2 x i32> %2, i64 4, i1 true)
 ; CHECK: ret void


 define void @f5(i32* noalias %dst, i32* noalias %src) {
 entry:
   %0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %src, i64 4, i1 true, i32 42)
   %arrayidx1 = getelementptr inbounds i32, i32* %dst, i64 3
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx1, i32 %0, i64 4, i1 true)
   %arrayidx2 = getelementptr inbounds i32, i32* %src, i64 1
   %1 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx2, i64 4, i1 true, i32 43)
   %arrayidx3 = getelementptr inbounds i32, i32* %dst, i64 2
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx3, i32 %1, i64 4, i1 true)
   %arrayidx4 = getelementptr inbounds i32, i32* %src, i64 2
   %2 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx4, i64 4, i1 true, i32 44)
   %arrayidx5 = getelementptr inbounds i32, i32* %dst, i64 1
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %arrayidx5, i32 %2, i64 4, i1 true)
   %arrayidx6 = getelementptr inbounds i32, i32* %src, i64 3
   %3 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* %arrayidx6, i64 4, i1 true, i32 45)
   call void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32* %dst, i32 %3, i64 4, i1 true)
   ret void
 }

 ; CHECK-LABEL: define void @f5
 ; CHECK: %0 = bitcast i32* %src to <4 x i32>*
 ; CHECK: %1 = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p0v4i32.v4i32(<4 x i32>* %0, i64 4, i1 true, <4 x i32> <i32 42, i32 43, i32 44, i32 45>)
 ; CHECK: %2 = shufflevector <4 x i32> %1, <4 x i32> undef, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
 ; CHECK: %3 = bitcast i32* %dst to <4 x i32>*
 ; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p0v4i32.v4i32(<4 x i32>* %3, <4 x i32> %2, i64 4, i1 true)
 ; CHECK: ret void

; Function Attrs: nounwind readonly
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32*, i64, i1, i32) #0

declare void @llvm.genx.GenISA.PredicatedStore.p0i32.i32(i32*, i32, i64, i1)

attributes #0 = { nounwind readonly }

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6}

!0 = !{void (i32*, i32*)* @f0, !10}
!1 = !{void (i32*, i32*)* @f0_different_predicates, !10}
!2 = !{void (i32*, i32*)* @f1, !10}
!3 = !{void (i32*, i32*)* @f2, !10}
!4 = !{void (i32, i32, i32*, i32*)* @f3, !10}
!5 = !{void (i32, i32, i32*, i32*)* @f4, !10}
!6 = !{void (i32*, i32*)* @f5, !10}

!10 = !{!20}
!20 = !{!"function_type", i32 0}
