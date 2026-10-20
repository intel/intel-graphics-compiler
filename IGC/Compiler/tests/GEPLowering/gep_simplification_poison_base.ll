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
; RUN: igc_opt -platformpvc --igc-gep-lowering -regkey=EnableGEPSimplification=1,TestGEPSimplification=1 -S %s | FileCheck %s
;

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"

; %base is poison for odd %x, where only %derived is used: no rebase on it.
; CHECK-LABEL: @disjoint_or_base(
; CHECK: %derived = getelementptr inbounds float, ptr addrspace(1) %src, i64 %derived.idx.64
define spir_kernel void @disjoint_or_base(ptr addrspace(1) %src, ptr addrspace(1) %dst, i32 %x) {
  %base.idx = or disjoint i32 %x, 1
  %base.idx.64 = zext i32 %base.idx to i64
  %base = getelementptr inbounds float, ptr addrspace(1) %src, i64 %base.idx.64
  %derived.idx = add nuw i32 %x, 2
  %derived.idx.64 = zext i32 %derived.idx to i64
  %derived = getelementptr inbounds float, ptr addrspace(1) %src, i64 %derived.idx.64
  %odd = trunc i32 %x to i1
  %ptr = select i1 %odd, ptr addrspace(1) %derived, ptr addrspace(1) %base
  %v = load float, ptr addrspace(1) %ptr, align 4
  store float %v, ptr addrspace(1) %dst, align 4
  ret void
}

; %base may be poison only through its flags where %derived is not: drop them.
; CHECK-LABEL: @droppable_flags_base(
; CHECK: %base.idx = add i64 %x, 2
; CHECK: %base = getelementptr float, ptr addrspace(1) %src, i64 %base.idx
; CHECK: getelementptr float, ptr addrspace(1) %base, i64 -1
define spir_kernel void @droppable_flags_base(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %x, i1 %use.base) {
  %base.idx = add nuw i64 %x, 2
  %base = getelementptr inbounds float, ptr addrspace(1) %src, i64 %base.idx
  %derived.idx = add nuw i64 %x, 1
  %derived = getelementptr inbounds float, ptr addrspace(1) %src, i64 %derived.idx
  %ptr = select i1 %use.base, ptr addrspace(1) %base, ptr addrspace(1) %derived
  %v = load float, ptr addrspace(1) %ptr, align 4
  store float %v, ptr addrspace(1) %dst, align 4
  ret void
}

; Without its nuw, %base.idx would wrap before the zext where %derived.idx
; doesn't: no rebase.
; CHECK-LABEL: @extended_index_base(
; CHECK: %derived = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %derived.idx.64
define spir_kernel void @extended_index_base(ptr addrspace(1) %src, ptr addrspace(1) %dst, i8 %x, i1 %use.base) {
  %base.idx = add nuw i8 %x, 10
  %base.idx.64 = zext i8 %base.idx to i64
  %base = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %base.idx.64
  %derived.idx = add nuw i8 %x, 2
  %derived.idx.64 = zext i8 %derived.idx to i64
  %derived = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %derived.idx.64
  %ptr = select i1 %use.base, ptr addrspace(1) %base, ptr addrspace(1) %derived
  %v = load i8, ptr addrspace(1) %ptr, align 1
  store i8 %v, ptr addrspace(1) %dst, align 1
  ret void
}

; %base.offset is poison where only %derived is used: no rebase on %base.
; CHECK-LABEL: @poison_only_in_base(
; CHECK: %derived = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx
define spir_kernel void @poison_only_in_base(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %idx, i64 %shift.input, i64 %shift.amount) {
  %base.offset = shl i64 %shift.input, %shift.amount
  %base.idx = add i64 %idx, %base.offset
  %base = getelementptr inbounds float, ptr addrspace(1) %src, i64 %base.idx
  %derived = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx
  %in.range = icmp ult i64 %shift.amount, 64
  %ptr = select i1 %in.range, ptr addrspace(1) %base, ptr addrspace(1) %derived
  %v = load float, ptr addrspace(1) %ptr, align 4
  store float %v, ptr addrspace(1) %dst, align 4
  ret void
}

; Drop %y's inbounds before another bucket rewrites it, or the rewrite keeps it.
; CHECK-LABEL: @base_index_through_other_bucket(
; CHECK: [[Y:%.*]] = getelementptr i8, ptr addrspace(1) %y0, i64 1
; CHECK: %base.idx = ptrtoint ptr addrspace(1) [[Y]] to i64
define spir_kernel void @base_index_through_other_bucket(ptr addrspace(1) %src, ptr addrspace(1) %dst, i64 %i, i1 %use.base) {
  %y0 = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %i
  %y.idx = add nuw i64 %i, 1
  %y = getelementptr inbounds i8, ptr addrspace(1) %src, i64 %y.idx
  %base.idx = ptrtoint ptr addrspace(1) %y to i64
  %base = getelementptr inbounds float, ptr addrspace(1) %dst, i64 %base.idx
  %src.int = ptrtoint ptr addrspace(1) %src to i64
  %derived.off = add nuw i64 %src.int, %i
  %derived.idx = add nuw i64 %derived.off, 3
  %derived = getelementptr inbounds float, ptr addrspace(1) %dst, i64 %derived.idx
  %ptr = select i1 %use.base, ptr addrspace(1) %base, ptr addrspace(1) %derived
  %v = load float, ptr addrspace(1) %ptr, align 4
  store float %v, ptr addrspace(1) %dst, align 4
  %w = load i8, ptr addrspace(1) %y0, align 1
  store i8 %w, ptr addrspace(1) %dst, align 1
  ret void
}

!igc.functions = !{!0, !3, !4, !5, !6}
!0 = !{ptr @disjoint_or_base, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @droppable_flags_base, !1}
!4 = !{ptr @extended_index_base, !1}
!5 = !{ptr @poison_only_in_base, !1}
!6 = !{ptr @base_index_through_other_bucket, !1}
