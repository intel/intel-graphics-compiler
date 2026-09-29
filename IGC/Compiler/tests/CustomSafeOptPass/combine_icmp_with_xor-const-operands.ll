;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt -igc-custom-safe-opt -opaque-pointers -S < %s | FileCheck %s
; ------------------------------------------------
;
; visitXor folds `xor (icmp P, a, b), true` into the inverted compare. When both
; compare operands are constants, IRBuilder::CreateICmp constant-folds and hands
; back a ConstantInt instead of an ICmpInst, so the result must be treated as a
; plain Value. The pass used to cast it unconditionally to ICmpInst and assert.
;
; The inverted-compare result is still propagated, and the compensating
; branch-successor / select-operand swaps on the original compare's users must
; still happen, so the folded constant stays semantically equivalent.
; ------------------------------------------------

; The reduced repro: the xor is the compare's only user and is itself dead, so
; both instructions fold away entirely.
define spir_kernel void @test_const_icmp_dead_xor() {
; CHECK-LABEL: @test_const_icmp_dead_xor(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: ret void
entry:
  %cmp = icmp eq i32 0, 0
  %not = xor i1 %cmp, true
  ret void
}

; The xor feeds a conditional branch. `icmp eq 0, 0` is true, so the xor is
; false and control must reach %bb2.
define spir_kernel void @test_const_icmp_xor_br(ptr addrspace(1) %res) {
; CHECK-LABEL: @test_const_icmp_xor_br(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: br i1 false, label %bb1, label %bb2
entry:
  %cmp = icmp eq i32 0, 0
  %not = xor i1 %cmp, true
  br i1 %not, label %bb1, label %bb2
bb1:
  store i32 11, ptr addrspace(1) %res
  br label %bb3
bb2:
  store i32 22, ptr addrspace(1) %res
  br label %bb3
bb3:
  ret void
}

; The original compare also feeds a branch directly. That branch's successors
; get swapped to compensate for the inversion; with the folded `false`
; condition, control must still reach %bb1 as it did before the transform.
define spir_kernel void @test_const_icmp_br_user(ptr addrspace(1) %res) {
; CHECK-LABEL: @test_const_icmp_br_user(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: br i1 false, label %bb2, label %bb1
entry:
  %cmp = icmp eq i32 0, 0
  %not = xor i1 %cmp, true
  br i1 %cmp, label %bb1, label %bb2
bb1:
  store i32 11, ptr addrspace(1) %res
  br label %bb3
bb2:
  store i32 22, ptr addrspace(1) %res
  br label %bb3
bb3:
  ret void
}

; The original compare also feeds a select. The select's arms get swapped to
; compensate for the inversion; with the folded `false` condition the select
; must still yield 11.
define spir_kernel void @test_const_icmp_select_user(ptr addrspace(1) %res) {
; CHECK-LABEL: @test_const_icmp_select_user(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: %sel = select i1 false, i32 22, i32 11
entry:
  %cmp = icmp eq i32 0, 0
  %not = xor i1 %cmp, true
  %sel = select i1 %cmp, i32 11, i32 22
  store i32 %sel, ptr addrspace(1) %res
  ret void
}

; A folded vector compare yields a ConstantAggregateZero rather than a
; ConstantInt, so the result must not be assumed to be either. `icmp eq
; zeroinitializer, zeroinitializer` is all-true, its inverse is all-false, and
; the swapped select arms must still yield zeroinitializer.
define spir_kernel void @test_const_vector_icmp(<4 x i32> %v, ptr addrspace(1) %res) {
; CHECK-LABEL: @test_const_vector_icmp(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: %sel = select <4 x i1> zeroinitializer, <4 x i32> %v, <4 x i32> zeroinitializer
entry:
  %cmp = icmp eq <4 x i32> zeroinitializer, zeroinitializer
  %not = xor <4 x i1> %cmp, <i1 true, i1 true, i1 true, i1 true>
  %sel = select <4 x i1> %cmp, <4 x i32> zeroinitializer, <4 x i32> %v
  store <4 x i32> %sel, ptr addrspace(1) %res
  ret void
}

; A folded undef compare yields an UndefValue, which is likewise neither an
; ICmpInst nor a ConstantInt.
define spir_kernel void @test_undef_icmp(ptr addrspace(1) %res) {
; CHECK-LABEL: @test_undef_icmp(
; CHECK-NOT: icmp
; CHECK-NOT: xor
; CHECK: br i1 undef, label %bb2, label %bb1
entry:
  %cmp = icmp eq i32 undef, undef
  %not = xor i1 %cmp, true
  br i1 %cmp, label %bb1, label %bb2
bb1:
  store i32 11, ptr addrspace(1) %res
  ret void
bb2:
  store i32 22, ptr addrspace(1) %res
  ret void
}

; Only one operand is constant, so nothing folds.
define spir_kernel void @test_single_const_operand(i32 %x, ptr addrspace(1) %res) {
; CHECK-LABEL: @test_single_const_operand(
; CHECK: [[INV:%[a-zA-Z0-9.]+]] = icmp ne i32 %x, 0
; CHECK-NOT: xor
; CHECK: br i1 [[INV]], label %bb2, label %bb1
entry:
  %cmp = icmp eq i32 %x, 0
  %not = xor i1 %cmp, true
  br i1 %cmp, label %bb1, label %bb2
bb1:
  store i32 11, ptr addrspace(1) %res
  ret void
bb2:
  store i32 22, ptr addrspace(1) %res
  ret void
}

; Negative control: with non-constant operands nothing folds, so a real
; inverted ICmpInst is still created and the successors still swap.
define spir_kernel void @test_nonconst_icmp_still_inverted(i32 %x, i32 %y, ptr addrspace(1) %res) {
; CHECK-LABEL: @test_nonconst_icmp_still_inverted(
; CHECK: [[INV:%[a-zA-Z0-9.]+]] = icmp ne i32 %x, %y
; CHECK-NOT: xor
; CHECK: br i1 [[INV]], label %bb2, label %bb1
entry:
  %cmp = icmp eq i32 %x, %y
  %not = xor i1 %cmp, true
  br i1 %cmp, label %bb1, label %bb2
bb1:
  store i32 11, ptr addrspace(1) %res
  br label %bb3
bb2:
  store i32 22, ptr addrspace(1) %res
  br label %bb3
bb3:
  ret void
}
