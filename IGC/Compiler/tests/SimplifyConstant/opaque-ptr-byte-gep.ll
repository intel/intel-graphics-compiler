;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -enable-debugify -SimplifyConstant -S < %s 2>&1 | FileCheck %s

; This test verifies that SimplifyConstant correctly handles byte-indexed GEPs
; created by SROA when using opaque pointers. The GEP uses byte offsets (i8 type)
; instead of element indices, so the pass must convert byte offsets to element
; indices before performing constant folding.

; Case 1: Constant offset
; Byte offset 4 should access element 1 (value 20), not element 0 (value 10)
@oddeven = private addrspace(2) constant [4 x i32] [i32 10, i32 20, i32 10, i32 20], align 4

define spir_kernel void @test_oddeven_byte_gep_const(ptr %out) {
; CHECK-LABEL: @test_oddeven_byte_gep_const(
; CHECK:  entry:
; Byte offset 4 / sizeof(i32) = 1, so select should pick 20.
; CHECK:    store i32 20, ptr [[OUT:%.*]], align 4
; CHECK:    ret void
;
entry:
  %ptr = getelementptr inbounds i8, ptr addrspace(2) @oddeven, i64 4
  %val = load i32, ptr addrspace(2) %ptr, align 4
  store i32 %val, ptr %out, align 4
  ret void
}

; Case 2: Dynamic offset
; Verifies that dynamic byte offsets are converted to element indices
define spir_kernel void @test_oddeven_byte_gep_dynamic(i64 %idx, ptr %out) {
; CHECK-LABEL: @test_oddeven_byte_gep_dynamic(
; CHECK:  entry:
; The byte offset should be divided by 4 (sizeof i32) to get element index
; CHECK:    [[ELT_IDX:%.*]] = udiv i64 [[IDX:%.*]], 4
; CHECK:    [[CMP:%.*]] = trunc i64 [[ELT_IDX]] to i1
; CHECK:    [[VAL:%.*]] = select i1 [[CMP]], i32 20, i32 10
; CHECK:    store i32 [[VAL]], ptr [[OUT:%.*]], align 4
; CHECK:    ret void
;
entry:
  %ptr = getelementptr inbounds i8, ptr addrspace(2) @oddeven, i64 %idx
  %val = load i32, ptr addrspace(2) %ptr, align 4
  store i32 %val, ptr %out, align 4
  ret void
}

; Case 3: Float array with byte-indexed GEP (reproducer for FFT twiddle factor bug)
@twi = private addrspace(2) constant [2 x float] [float 0x3FDA07F920000000, float 0x3FE7C7D7A0000000], align 4

define spir_kernel void @test_float_byte_gep_elem0(ptr %out) {
; CHECK-LABEL: @test_float_byte_gep_elem0(
; CHECK:  entry:
; Byte offset 0 = element 0
; CHECK:    store float 0x3FDA07F920000000, ptr [[OUT:%.*]], align 4
; CHECK:    ret void
;
entry:
  %ptr = getelementptr inbounds i8, ptr addrspace(2) @twi, i64 0
  %val = load float, ptr addrspace(2) %ptr, align 4
  store float %val, ptr %out, align 4
  ret void
}

define spir_kernel void @test_float_byte_gep_elem1(ptr %out) {
; CHECK-LABEL: @test_float_byte_gep_elem1(
; CHECK:  entry:
; Byte offset 4 = element 1 (this was the buggy case)
; CHECK:    store float 0x3FE7C7D7A0000000, ptr [[OUT:%.*]], align 4
; CHECK:    ret void
;
entry:
  %ptr = getelementptr inbounds i8, ptr addrspace(2) @twi, i64 4
  %val = load float, ptr addrspace(2) %ptr, align 4
  store float %val, ptr %out, align 4
  ret void
}

; Case 4: Ensure element-indexed GEPs still work (regression test)
define spir_kernel void @test_element_indexed_gep(i32 %idx, ptr %out) {
; CHECK-LABEL: @test_element_indexed_gep(
; CHECK:  entry:
; CHECK:    [[CMP:%.*]] = trunc i32 [[IDX:%.*]] to i1
; CHECK:    [[VAL:%.*]] = select i1 [[CMP]], i32 20, i32 10
; CHECK:    store i32 [[VAL]], ptr [[OUT:%.*]], align 4
; CHECK:    ret void
;
entry:
  %ptr = getelementptr inbounds [4 x i32], ptr addrspace(2) @oddeven, i64 0, i32 %idx
  %val = load i32, ptr addrspace(2) %ptr, align 4
  store i32 %val, ptr %out, align 4
  ret void
}
