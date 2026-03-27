;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt -opaque-pointers -S -platformbmg -igc-memopt %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Basic: leading i32, candidate <2 x half> at +4 -> <2 x i32> merged load
; CHECK-LABEL: @merge_i32_v2half
; CHECK: load <2 x i32>
; CHECK: extractelement <2 x i32>
; CHECK: extractelement <2 x i32>
; CHECK: bitcast i32 %{{.*}} to <2 x half>
define spir_kernel void @merge_i32_v2half(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %ptr1, align 4
  store i32 %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Promotion: leading <2 x half>, candidate i32 at +4 -> <2 x i32> merged load
; CHECK-LABEL: @merge_v2half_i32
; CHECK: load <2 x i32>
; CHECK: extractelement <2 x i32>
; CHECK: bitcast i32 %{{.*}} to <2 x half>
; CHECK: extractelement <2 x i32>
define spir_kernel void @merge_v2half_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load i32, ptr addrspace(1) %ptr1, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store i32 %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Three-load merge: i32, <2 x half> at +4, i32 at +8 -> <3 x i32> merged load
; CHECK-LABEL: @merge_three_i32_v2half_i32
; CHECK: load <3 x i32>
define spir_kernel void @merge_three_i32_v2half_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %ptr1, align 4
  %ptr2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val2 = load i32, ptr addrspace(1) %ptr2, align 4
  store i32 %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  %dst2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store i32 %val2, ptr addrspace(1) %dst2, align 4
  ret void
}

; Negative: promotion only when leading load is sole entry.
; Two <2 x half> then i32: the two halfs merge among themselves, i32 stays separate.
; CHECK-LABEL: @no_merge_v2half_v2half_i32
; CHECK-NOT: load <3 x i32>
; CHECK: load <4 x half>
; CHECK: load i32
define spir_kernel void @no_merge_v2half_v2half_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %ptr1, align 4
  %ptr2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val2 = load i32, ptr addrspace(1) %ptr2, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  %dst2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store i32 %val2, ptr addrspace(1) %dst2, align 4
  ret void
}

; i64 as merged scalar, <2 x i32> promoted
; CHECK-LABEL: @merge_i64_v2i32
; CHECK: load <2 x i64>
; CHECK: extractelement <2 x i64>
; CHECK: extractelement <2 x i64>
; CHECK: bitcast i64 %{{.*}} to <2 x i32>
define spir_kernel void @merge_i64_v2i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i64, ptr addrspace(1) %src, align 8
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val1 = load <2 x i32>, ptr addrspace(1) %ptr1, align 8
  store i64 %val0, ptr addrspace(1) %dst, align 8
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store <2 x i32> %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Negative: pointer scalar type is not promotable
; CHECK-LABEL: @merge_ptr_v2i32
; CHECK-NOT: load <2 x i64>
define spir_kernel void @merge_ptr_v2i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load ptr addrspace(1), ptr addrspace(1) %src, align 8
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val1 = load <2 x i32>, ptr addrspace(1) %ptr1, align 8
  store ptr addrspace(1) %val0, ptr addrspace(1) %dst, align 8
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store <2 x i32> %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Negative: <4 x i16> exceeds MaxSmallVecMergeElts
; CHECK-LABEL: @no_merge_v4i16_i64
; CHECK-NOT: load <2 x i64>
define spir_kernel void @no_merge_v4i16_i64(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <4 x i16>, ptr addrspace(1) %src, align 8
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val1 = load i64, ptr addrspace(1) %ptr1, align 8
  store <4 x i16> %val0, ptr addrspace(1) %dst, align 8
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store i64 %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Negative: total size mismatch (<2 x half> = 4 bytes != i64 = 8 bytes)
; CHECK-LABEL: @no_merge_v2half_i64
; CHECK-NOT: load <2 x i64>
define spir_kernel void @no_merge_v2half_i64(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load i64, ptr addrspace(1) %ptr1, align 8
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store i64 %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Store test: basic store merge i32 then <2 x half>
; CHECK-LABEL: @store_merge_i32_v2half
; CHECK: bitcast <2 x half> %{{.*}} to i32
; CHECK: store <2 x i32>
define spir_kernel void @store_merge_i32_v2half(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %src1, align 4
  store i32 %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Store test: store promotion leading <2 x half>
; CHECK-LABEL: @store_merge_v2half_i32
; CHECK: bitcast <2 x half> %{{.*}} to i32
; CHECK: store <2 x i32>
define spir_kernel void @store_merge_v2half_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load i32, ptr addrspace(1) %src1, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store i32 %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Store test: promotion must keep the scan count in i32 space so
; subsequent i32 stores remain mergeable.
; CHECK-LABEL: @store_merge_v2half_i32_i32_i32
; CHECK: bitcast <2 x half> %{{.*}} to i32
; CHECK: store <4 x i32>
define spir_kernel void @store_merge_v2half_i32_i32_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load i32, ptr addrspace(1) %src1, align 4
  %src2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val2 = load i32, ptr addrspace(1) %src2, align 4
  %src3 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 12
  %val3 = load i32, ptr addrspace(1) %src3, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store i32 %val1, ptr addrspace(1) %dst1, align 4
  %dst2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store i32 %val2, ptr addrspace(1) %dst2, align 4
  %dst3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 12
  store i32 %val3, ptr addrspace(1) %dst3, align 4
  ret void
}

; Store test: 64-bit store merge
; CHECK-LABEL: @store_merge_i64_v2i32
; CHECK: bitcast <2 x i32> %{{.*}} to i64
; CHECK: store <2 x i64>
define spir_kernel void @store_merge_i64_v2i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i64, ptr addrspace(1) %src, align 8
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val1 = load <2 x i32>, ptr addrspace(1) %src1, align 8
  store i64 %val0, ptr addrspace(1) %dst, align 8
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store <2 x i32> %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Negative: store-side pointer scalar type is not promotable
; CHECK-LABEL: @store_no_merge_ptr_v2i32
; CHECK-NOT: store <2 x i64>
; CHECK: store ptr addrspace(1)
; CHECK: store <2 x i32>
define spir_kernel void @store_no_merge_ptr_v2i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load ptr addrspace(1), ptr addrspace(1) %src, align 8
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val1 = load <2 x i32>, ptr addrspace(1) %src1, align 8
  store ptr addrspace(1) %val0, ptr addrspace(1) %dst, align 8
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store <2 x i32> %val1, ptr addrspace(1) %dst1, align 8
  ret void
}

; Predicated load: i32 then <2 x half> at +4 with merge values
; CHECK-LABEL: @predicated_merge_i32_v2half
; CHECK: call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1.v2i32(
; CHECK: extractelement <2 x i32>
; CHECK: extractelement <2 x i32>
; CHECK: bitcast i32 %{{.*}} to <2 x half>
define spir_kernel void @predicated_merge_i32_v2half(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %src, i64 4, i1 true, i32 42)
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = call <2 x half> @llvm.genx.GenISA.PredicatedLoad.v2f16.p1.v2f16(ptr addrspace(1) %ptr1, i64 4, i1 true, <2 x half> <half 1.0, half 2.0>)
  store i32 %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Rollback load test: leading <2 x half>, non-consecutive i32 at +8 (gap at +4)
; should fail promotion and still merge subsequent same-type loads.
; The i32 at +8 is not consecutive to <2 x half> at +0 (gap), so promotion
; should not poison state for later <2 x half> at +4.
; CHECK-LABEL: @rollback_load_failed_promotion
; CHECK: load <4 x half>
; CHECK-NOT: load <{{[0-9]+}} x i32>
define spir_kernel void @rollback_load_failed_promotion(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  ; Gap: +4 is a <2 x half> (same type, should merge normally)
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %ptr1, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Rollback store test: leading <2 x half>, non-consecutive i32 at +8 (gap)
; should fail promotion and still merge subsequent same-type stores.
; CHECK-LABEL: @rollback_store_failed_promotion
; CHECK: store <4 x half>
; CHECK-NOT: store <{{[0-9]+}} x i32>
define spir_kernel void @rollback_store_failed_promotion(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x half>, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %src1, align 4
  store <2 x half> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Negative: <2 x i15> total size (30 bits = 4 bytes padded) != i32 (4 bytes store size)
; but the types are not cleanly bitcastable; i15 is not a standard width.
; CHECK-LABEL: @no_merge_v2i15_i32
; CHECK-NOT: load <2 x i32>
define spir_kernel void @no_merge_v2i15_i32(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load <2 x i15>, ptr addrspace(1) %src, align 4
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load i32, ptr addrspace(1) %ptr1, align 4
  store <2 x i15> %val0, ptr addrspace(1) %dst, align 4
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store i32 %val1, ptr addrspace(1) %dst1, align 4
  ret void
}

; Predicated store: i32 then <2 x half> at +4 with predicated stores
; CHECK-LABEL: @predicated_store_merge_i32_v2half
; CHECK: bitcast <2 x half> %{{.*}} to i32
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v2i32(
define spir_kernel void @predicated_store_merge_i32_v2half(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %src1, align 4
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %dst, i32 %val0, i64 4, i1 true)
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  call void @llvm.genx.GenISA.PredicatedStore.p1.v2f16(ptr addrspace(1) %dst1, <2 x half> %val1, i64 4, i1 true)
  ret void
}

; Negative predicated store: different predicates should not merge
; CHECK-LABEL: @predicated_store_no_merge_diff_pred
; CHECK-NOT: call void @llvm.genx.GenISA.PredicatedStore.p1.v2i32(
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.i32(
; CHECK: call void @llvm.genx.GenISA.PredicatedStore.p1.v2f16(
define spir_kernel void @predicated_store_no_merge_diff_pred(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 4
  %src1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %src1, align 4
  call void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1) %dst, i32 %val0, i64 4, i1 true)
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  call void @llvm.genx.GenISA.PredicatedStore.p1.v2f16(ptr addrspace(1) %dst1, <2 x half> %val1, i64 4, i1 false)
  ret void
}

; Low alignment: mixed-size merge still works at align 2 because the
; total merged size (12 bytes) fits the low-alignment path constraints.
; CHECK-LABEL: @low_align_mixed_merge
; CHECK: load <3 x i32>
; CHECK: bitcast i32 %{{.*}} to <2 x half>
define spir_kernel void @low_align_mixed_merge(ptr addrspace(1) %src, ptr addrspace(1) %dst, <8 x i32> %r0) {
entry:
  %val0 = load i32, ptr addrspace(1) %src, align 2
  %ptr1 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 4
  %val1 = load <2 x half>, ptr addrspace(1) %ptr1, align 2
  %ptr2 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 8
  %val2 = load i32, ptr addrspace(1) %ptr2, align 2
  store i32 %val0, ptr addrspace(1) %dst, align 2
  %dst1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 4
  store <2 x half> %val1, ptr addrspace(1) %dst1, align 2
  %dst2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8
  store i32 %val2, ptr addrspace(1) %dst2, align 2
  ret void
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32)
declare <2 x half> @llvm.genx.GenISA.PredicatedLoad.v2f16.p1.v2f16(ptr addrspace(1), i64, i1, <2 x half>)
declare void @llvm.genx.GenISA.PredicatedStore.p1.i32(ptr addrspace(1), i32, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1.v2f16(ptr addrspace(1), <2 x half>, i64, i1)

!igc.functions = !{!0, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22}

!0 = distinct !{ptr @merge_i32_v2half, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"implicit_arg_desc"}
!4 = distinct !{ptr @merge_v2half_i32, !1}
!5 = distinct !{ptr @merge_three_i32_v2half_i32, !1}
!6 = distinct !{ptr @no_merge_v2half_v2half_i32, !1}
!7 = distinct !{ptr @merge_i64_v2i32, !1}
!8 = distinct !{ptr @merge_ptr_v2i32, !1}
!9 = distinct !{ptr @no_merge_v4i16_i64, !1}
!10 = distinct !{ptr @no_merge_v2half_i64, !1}
!11 = distinct !{ptr @store_merge_i32_v2half, !1}
!12 = distinct !{ptr @store_merge_v2half_i32, !1}
!13 = distinct !{ptr @store_merge_i64_v2i32, !1}
!14 = distinct !{ptr @store_no_merge_ptr_v2i32, !1}
!15 = distinct !{ptr @predicated_merge_i32_v2half, !1}
!16 = distinct !{ptr @rollback_load_failed_promotion, !1}
!17 = distinct !{ptr @rollback_store_failed_promotion, !1}
!18 = distinct !{ptr @no_merge_v2i15_i32, !1}
!19 = distinct !{ptr @predicated_store_merge_i32_v2half, !1}
!20 = distinct !{ptr @predicated_store_no_merge_diff_pred, !1}
!21 = distinct !{ptr @low_align_mixed_merge, !1}
!22 = distinct !{ptr @store_merge_v2half_i32_i32_i32, !1}
