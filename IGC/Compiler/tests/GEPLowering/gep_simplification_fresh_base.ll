;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-18-plus, regkeys
;
; RUN: igc_opt -platformpvc --igc-gep-lowering -regkey=EnableGEPSimplification=1,TestGEPSimplification=1,EnableGEPSimplificationPoisonSafeBases=1 -S %s | FileCheck %s
; RUN: igc_opt -platformpvc --igc-gep-lowering -regkey=EnableGEPSimplification=1,TestGEPSimplification=1,EnableGEPSimplificationPoisonSafeBases=1,EnableGEPSimplificationFreshBases=0 -S %s | FileCheck %s --check-prefix=NOFRESH

; A row address may be poison where another is valid: share a fresh base built
; from each row's own flags.
; CHECK-LABEL: @prefetched_rows(
; CHECK: [[ROW:%.*]] = sext i32 %row to i64
; CHECK: [[ROW0:%.*]] = add nsw i64 [[ROW]], 1
; CHECK: [[STRIDE0:%.*]] = sext i32 %stride to i64
; CHECK: [[ROW0_OFF:%.*]] = mul i64 [[ROW0]], [[STRIDE0]]
; CHECK: [[COL:%.*]] = sext i32 %col to i64
; CHECK: [[ROW0_IDX:%.*]] = add i64 [[ROW0_OFF]], [[COL]]
; CHECK: [[ROW0_ADDR:%.*]] = getelementptr i16, ptr addrspace(1) %src, i64 [[ROW0_IDX]]
; CHECK: LSCSimdBlockPrefetch.p1(ptr addrspace(1) [[ROW0_ADDR]],
; CHECK: [[STRIDE1:%.*]] = sext i32 %stride to i64
; CHECK: [[ROW1_ADDR:%.*]] = getelementptr i16, ptr addrspace(1) [[ROW0_ADDR]], i64 [[STRIDE1]]
; CHECK: LSCSimdBlockPrefetch.p1(ptr addrspace(1) [[ROW1_ADDR]],
; NOFRESH-LABEL: @prefetched_rows(
; NOFRESH: %row1.addr = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %row1.idx.64
define spir_kernel void @prefetched_rows(ptr addrspace(1) %src, i32 %row, i32 %stride, i32 %col) {
  %row0 = or disjoint i32 %row, 1
  %row0.off = mul nsw i32 %row0, %stride
  %row0.idx = add nsw i32 %row0.off, %col
  %row0.idx.64 = sext i32 %row0.idx to i64
  %row0.addr = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %row0.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %row0.addr, i32 2, i32 2, i32 0)
  %row1 = or disjoint i32 %row, 2
  %row1.off = mul nsw i32 %row1, %stride
  %row1.idx = add nsw i32 %row1.off, %col
  %row1.idx.64 = sext i32 %row1.idx to i64
  %row1.addr = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %row1.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %row1.addr, i32 2, i32 2, i32 0)
  ret void
}

; Don't rebase %derived on %base: %y may be poison where %x isn't.
; CHECK-LABEL: @fresh_base_adds_poison(
; CHECK: %derived = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %x.64
define spir_kernel void @fresh_base_adds_poison(ptr addrspace(1) %src, i32 %x, i32 %y) {
  %base.idx = sub nsw i32 %x, %y
  %base.idx.64 = sext i32 %base.idx to i64
  %base = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %base.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %base, i32 2, i32 2, i32 0)
  %x.64 = sext i32 %x to i64
  %derived = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %x.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %derived, i32 2, i32 2, i32 0)
  ret void
}

; %x + 1 may wrap, so its sext isn't sext(%x) + 1: no common fresh base.
; CHECK-LABEL: @wrapping_inner_add(
; CHECK: %derived = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %derived.idx.64
define spir_kernel void @wrapping_inner_add(ptr addrspace(1) %src, i32 %x, i32 %c) {
  %x1 = add i32 %x, 1
  %base.idx = add nsw i32 %x1, %c
  %base.idx.64 = sext i32 %base.idx to i64
  %base = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %base.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %base, i32 2, i32 2, i32 0)
  %x2 = add i32 %x, 2
  %derived.idx = add nsw i32 %x2, %c
  %derived.idx.64 = sext i32 %derived.idx to i64
  %derived = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %derived.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %derived, i32 2, i32 2, i32 0)
  ret void
}

; Without disjoint, %x | %y isn't %x + %y: it stays a leaf of the fresh base.
; CHECK-LABEL: @overlapping_or(
; CHECK: [[O:%.*]] = sext i32 %o to i64
; CHECK: [[IDX:%.*]] = add nsw i64 [[O]], 1
; CHECK: [[BASE:%.*]] = getelementptr i16, ptr addrspace(1) %src, i64 [[IDX]]
; CHECK: getelementptr i16, ptr addrspace(1) [[BASE]], i64 1
define spir_kernel void @overlapping_or(ptr addrspace(1) %src, i32 %x, i32 %y) {
  %o = or i32 %x, %y
  %base.idx = add nsw i32 %o, 1
  %base.idx.64 = sext i32 %base.idx to i64
  %base = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %base.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %base, i32 2, i32 2, i32 0)
  %derived.idx = add nsw i32 %o, 2
  %derived.idx.64 = sext i32 %derived.idx to i64
  %derived = getelementptr inbounds i16, ptr addrspace(1) %src, i64 %derived.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %derived, i32 2, i32 2, i32 0)
  ret void
}

; Different index widths rule out a rebase before the poison-safety checks, so
; this GEP gets no fresh base either.
; CHECK-LABEL: @mixed_index_widths(
; CHECK: %derived = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %derived.idx.64
define spir_kernel void @mixed_index_widths(ptr addrspace(1) %src, i16 %x) {
  %base.idx = sext i16 %x to i32
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %base.idx
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %base, i32 2, i32 2, i32 0)
  %derived.idx = add nsw i32 %base.idx, 1
  %derived.idx.64 = sext i32 %derived.idx to i64
  %derived = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %derived.idx.64
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %derived, i32 2, i32 2, i32 0)
  ret void
}

; 258 * 258 overflows i16, but not the i64 index width the fresh base is built in.
; CHECK-LABEL: @narrow_index(
; CHECK: [[X:%.*]] = sext i16 %x to i64
; CHECK: [[IDX:%.*]] = mul nsw i64 [[X]], 66564
; CHECK: [[BASE:%.*]] = getelementptr i8, ptr addrspace(1) %src, i64 [[IDX]]
; CHECK: getelementptr i8, ptr addrspace(1) [[BASE]], i64 1
define spir_kernel void @narrow_index(ptr addrspace(1) %src, i16 %x) {
  %base.mul = mul nsw i16 %x, 258
  %base.idx.16 = mul nsw i16 %base.mul, 258
  %base.idx = sext i16 %base.idx.16 to i32
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %base.idx
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %base, i32 2, i32 2, i32 0)
  %derived.idx.16 = add nsw i16 %base.idx.16, 1
  %derived.idx = sext i16 %derived.idx.16 to i32
  %derived = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %derived.idx
  call void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1) %derived, i32 2, i32 2, i32 0)
  ret void
}

declare void @llvm.genx.GenISA.LSCSimdBlockPrefetch.p1(ptr addrspace(1), i32, i32, i32)

!igc.functions = !{!0, !3, !4, !5, !6, !7}
!0 = !{ptr @prefetched_rows, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @fresh_base_adds_poison, !1}
!4 = !{ptr @wrapping_inner_add, !1}
!5 = !{ptr @overlapping_or, !1}
!6 = !{ptr @mixed_index_widths, !1}
!7 = !{ptr @narrow_index, !1}
