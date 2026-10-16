;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -opaque-pointers -S -platformbmg -igc-memopt %s | FileCheck %s

target datalayout = "e-p:64:64:64-i8:8:8-i16:16:16-i32:32:32-i64:64:64-v64:64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; Trimming +6 leaves an overlap at +3. The vector's element-3 fallback
; must remain 4, independently of the scalar's fallback of 30.
; CHECK-LABEL: define spir_kernel void @trim_overlap_false(
; CHECK: %vector = call <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1) %base, i64 4, i1 false, <4 x i8> <i8 1, i8 2, i8 3, i8 4>)
; CHECK: %last = extractelement <4 x i8> %vector, i32 3
; CHECK: %partial = add i8 %last, %{{.*}}
; CHECK: %sum = add i8 %partial, %{{.*}}
define spir_kernel void @trim_overlap_false(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 2
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %vector = call <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1) %base, i64 4, i1 false, <4 x i8> <i8 1, i8 2, i8 3, i8 4>)
  %p6 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 6
  %v6 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p6, i64 1, i1 false, i8 60)
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p3, i64 1, i1 false, i8 30)
  %last = extractelement <4 x i8> %vector, i32 3
  %partial = add i8 %last, %v6
  %sum = add i8 %partial, %v3
  store i8 %sum, ptr addrspace(1) %dst, align 1
  ret void
}

; The same fallback conflict occurs without trimming and with a runtime predicate.
; CHECK-LABEL: define spir_kernel void @overlap_runtime(
; CHECK: %vector = call <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1) %base, i64 4, i1 %predicate, <4 x i8> <i8 1, i8 2, i8 3, i8 4>)
; CHECK: %v3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p3, i64 1, i1 %predicate, i8 30)
; CHECK: %last = extractelement <4 x i8> %vector, i32 3
; CHECK: %sum = add i8 %last, %v3
define spir_kernel void @overlap_runtime(ptr addrspace(1) %src, ptr addrspace(1) %dst, i1 %predicate) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 2
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %vector = call <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1) %base, i64 4, i1 %predicate, <4 x i8> <i8 1, i8 2, i8 3, i8 4>)
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p3, i64 1, i1 %predicate, i8 30)
  %last = extractelement <4 x i8> %vector, i32 3
  %sum = add i8 %last, %v3
  store i8 %sum, ptr addrspace(1) %dst, align 1
  ret void
}

; Gaps do not conflict with fallback values, so non-overlapping predicated
; loads can still merge, preserving each scalar's fallback.
; CHECK-LABEL: define spir_kernel void @predicated_gap(
; CHECK: [[MERGED:%.*]] = call <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1) %base, i64 4, i1 %predicate, <4 x i8> <i8 10, i8 20, i8 undef, i8 30>)
; CHECK: [[V0:%.*]] = extractelement <4 x i8> [[MERGED]], i32 0
; CHECK: [[V1:%.*]] = extractelement <4 x i8> [[MERGED]], i32 1
; CHECK: [[V3:%.*]] = extractelement <4 x i8> [[MERGED]], i32 3
; CHECK: %partial = add i8 [[V0]], [[V1]]
; CHECK: %sum = add i8 %partial, [[V3]]
define spir_kernel void @predicated_gap(ptr addrspace(1) %src, ptr addrspace(1) %dst, i1 %predicate) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 2
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %v0 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %base, i64 4, i1 %predicate, i8 10)
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 1
  %v1 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p1, i64 1, i1 %predicate, i8 20)
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = call i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1) %p3, i64 1, i1 %predicate, i8 30)
  %partial = add i8 %v0, %v1
  %sum = add i8 %partial, %v3
  store i8 %sum, ptr addrspace(1) %dst, align 1
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <4 x i8> @llvm.genx.GenISA.PredicatedLoad.v4i8.p1.v4i8(ptr addrspace(1), i64, i1, <4 x i8>)
declare i8 @llvm.genx.GenISA.PredicatedLoad.i8.p1.i8(ptr addrspace(1), i64, i1, i8)

!igc.functions = !{!0, !3, !4}
!0 = !{ptr @trim_overlap_false, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @overlap_runtime, !1}
!4 = !{ptr @predicated_gap, !1}
