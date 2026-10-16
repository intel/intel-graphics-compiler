;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -opaque-pointers -S -platformbmg -basic-aa -igc-memopt %s | FileCheck %s --check-prefixes=CHECK,ALIGN

target datalayout = "e-p:64:64:64-i8:8:8-i16:16:16-i32:32:32-i64:64:64-v64:64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

; Same gap pattern as the scalarized record: removing the last two loads
; changes the span from seven to six to four, not seven to six to five.
; This scales the original DWORD offsets to bytes and uses the normal
; i8 profitable widths 16/12/8/4/2 without RT-specific setup.
; CHECK-LABEL: define spir_kernel void @trim_gap(
; CHECK-NOT: load
; CHECK: [[MERGED:%.*]] = load <4 x i8>, ptr addrspace(1) %{{.*}}, align 4
; CHECK-NOT: load
; CHECK: [[LAST:%.*]] = extractelement <4 x i8> [[MERGED]], i32 3
; CHECK-NEXT: %v6 = load i8, ptr addrspace(1) %p6, align 1
; CHECK: %v5 = load i8, ptr addrspace(1) %p5, align 1
; CHECK-NOT: load
; CHECK: %s2 = add i8 %{{.*}}, [[LAST]]
; CHECK-NOT: load
; CHECK: ret void
define spir_kernel void @trim_gap(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 2
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %p6 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 6
  %v6 = load i8, ptr addrspace(1) %p6, align 1
  %p5 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 5
  %v5 = load i8, ptr addrspace(1) %p5, align 1
  %v0 = load i8, ptr addrspace(1) %base, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 1
  %v1 = load i8, ptr addrspace(1) %p1, align 1
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 2
  %v2 = load i8, ptr addrspace(1) %p2, align 1
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = load i8, ptr addrspace(1) %p3, align 1
  %s0 = add i8 %v0, %v1
  %s1 = add i8 %s0, %v2
  %s2 = add i8 %s1, %v3
  %s3 = add i8 %s2, %v5
  %sum = add i8 %s3, %v6
  store i8 %sum, ptr addrspace(1) %dst, align 1
  ret void
}

; After dropping +6, the vector at +0 still ends at +4, beyond the scalar
; at +3. The remaining span must cover the entire vector, including overlap.
; CHECK-LABEL: define spir_kernel void @trim_overlap(
; CHECK-NOT: load
; CHECK: [[MERGED:%.*]] = load <4 x i8>, ptr addrspace(1) %{{.*}}, align 4
; CHECK: insertelement <4 x i8> %{{.*}}, i8 %{{.*}}, i32 3
; CHECK-NEXT: [[LAST:%.*]] = extractelement <4 x i8> [[MERGED]], i32 3
; CHECK: %v6 = load i8, ptr addrspace(1) %p6, align 1
; CHECK-NOT: load
; CHECK: %sum = add i8 %{{.*}}, [[LAST]]
define spir_kernel void @trim_overlap(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 2
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %p6 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 6
  %v6 = load i8, ptr addrspace(1) %p6, align 1
  %vector = load <4 x i8>, ptr addrspace(1) %base, align 4
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = load i8, ptr addrspace(1) %p3, align 1
  %first = extractelement <4 x i8> %vector, i32 0
  %partial = add i8 %first, %v6
  %sum = add i8 %partial, %v3
  store i8 %sum, ptr addrspace(1) %dst, align 1
  ret void
}

; The +8 load cannot move across the aliasing store. Only the accepted +0/+4
; loads should contribute to the merged span, even though no trimming is needed.
; CHECK-LABEL: define spir_kernel void @rejected_candidate(
; CHECK-NOT: load
; CHECK: [[MERGED:%.*]] = load <2 x i32>, ptr addrspace(1) %src, align 4
; CHECK: [[V0:%.*]] = extractelement <2 x i32> [[MERGED]], i32 0
; CHECK-NEXT: [[V4:%.*]] = extractelement <2 x i32> [[MERGED]], i32 1
; CHECK-NOT: load
; CHECK: store i32 77, ptr addrspace(1) %p8, align 4
; CHECK-NEXT: %v8 = load i32, ptr addrspace(1) %p8, align 4
; CHECK-NOT: load
; CHECK: %partial = add i32 [[V0]], [[V4]]
; CHECK: %sum = add i32 %partial, %v8
; CHECK-NOT: load
; CHECK: ret void
define spir_kernel void @rejected_candidate(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %v0 = load i32, ptr addrspace(1) %src, align 4
  %p4 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %v4 = load i32, ptr addrspace(1) %p4, align 4
  %p8 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  store i32 77, ptr addrspace(1) %p8, align 4
  %v8 = load i32, ptr addrspace(1) %p8, align 4
  %partial = add i32 %v0, %v4
  %sum = add i32 %partial, %v8
  store i32 %sum, ptr addrspace(1) %dst, align 4
  ret void
}

; Trimming +10 leaves an eight-byte span, but the i16 at +3 is not on a
; merged element boundary. Keep it separate while merging the aligned loads.
; ALIGN-LABEL: define spir_kernel void @trim_misaligned_i16(
; ALIGN-NOT: load
; ALIGN: [[MERGED:%.*]] = load <4 x i16>, ptr addrspace(1) %{{.*}}, align 4
; ALIGN-NOT: load
; ALIGN: insertelement <4 x i16> %{{.*}}, i16 %{{.*}}, i32 3
; ALIGN-NEXT: [[V2:%.*]] = extractelement <4 x i16> [[MERGED]], i32 1
; ALIGN-NEXT: %v10 = load <2 x i16>, ptr addrspace(1) %p10, align 2
; ALIGN-NOT: load
; ALIGN: %v3 = load i16, ptr addrspace(1) %p3, align 1
; ALIGN-NOT: load
; ALIGN: %partial = add i16 %first, [[V2]]
; ALIGN: %sum = add i16 %partial, %v3
; ALIGN-NOT: load
; ALIGN: ret void
define spir_kernel void @trim_misaligned_i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 4
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %p10 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 10
  %v10 = load <2 x i16>, ptr addrspace(1) %p10, align 2
  %vector = load <4 x i16>, ptr addrspace(1) %base, align 4
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 2
  %v2 = load i16, ptr addrspace(1) %p2, align 2
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = load i16, ptr addrspace(1) %p3, align 1
  %first = extractelement <4 x i16> %vector, i32 0
  %last = extractelement <2 x i16> %v10, i32 0
  %partial = add i16 %first, %v2
  %sum = add i16 %partial, %v3
  %result = add i16 %sum, %last
  store i16 %result, ptr addrspace(1) %dst, align 2
  ret void
}

; A divisible span also permits misaligned members without any trimming.
; The +3 load reads bytes +3/+4, not lane 1 (bytes +2/+3) of the vector.
; ALIGN-LABEL: define spir_kernel void @misaligned_i16(
; ALIGN-NOT: load
; ALIGN: %vector = load <4 x i16>, ptr addrspace(1) %base, align 4
; ALIGN-NOT: load
; ALIGN: %v3 = load i16, ptr addrspace(1) %p3, align 1
; ALIGN-NOT: load
; ALIGN: %sum = add i16 %first, %v3
; ALIGN-NOT: load
; ALIGN: ret void
define spir_kernel void @misaligned_i16(ptr addrspace(1) %src, ptr addrspace(1) %dst) {
entry:
  %id = call i16 @llvm.genx.GenISA.simdLaneId()
  %index = zext i16 %id to i64
  %offset = shl i64 %index, 4
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %offset
  %vector = load <4 x i16>, ptr addrspace(1) %base, align 4
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %base, i64 3
  %v3 = load i16, ptr addrspace(1) %p3, align 1
  %first = extractelement <4 x i16> %vector, i32 0
  %sum = add i16 %first, %v3
  store i16 %sum, ptr addrspace(1) %dst, align 2
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !3, !4, !5, !6}
!0 = !{ptr @trim_gap, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @trim_overlap, !1}
!4 = !{ptr @rejected_candidate, !1}
!5 = !{ptr @trim_misaligned_i16, !1}
!6 = !{ptr @misaligned_i16, !1}
