;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lower-simd-size --igc-lower-simd-size-override=16 -S < %s | FileCheck %s
; ------------------------------------------------
; LowerSIMDSize: constant propagation into users
; ------------------------------------------------

; After replacing GenISA_simdSize() with a constant the pass runs a
; ConstantFoldInstruction closure loop (mirrors IGCConstProp) to fold
; downstream arithmetic.  All tests below use SIMD16 (lane count = 16).

; Arithmetic folded: simdSize * 4 => 64
define spir_kernel void @test_mul(ptr %out) {
; CHECK-LABEL: @test_mul(
; CHECK:         store i32 64, ptr %out
; CHECK-NOT:     call i32 @llvm.genx.GenISA.simdSize
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %mul = mul i32 %sz, 4
  store i32 %mul, ptr %out, align 4
  ret void
}

; Comparison folded: simdSize == 16 => true (i1 true => i1 1 stored as i8)
define spir_kernel void @test_icmp_eq_true(ptr %out) {
; CHECK-LABEL: @test_icmp_eq_true(
; CHECK:         store i1 true, ptr %out
; CHECK-NOT:     call i32 @llvm.genx.GenISA.simdSize
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %cmp = icmp eq i32 %sz, 16
  store i1 %cmp, ptr %out, align 1
  ret void
}

; Comparison folded: simdSize == 8 => false when running SIMD16
define spir_kernel void @test_icmp_eq_false(ptr %out) {
; CHECK-LABEL: @test_icmp_eq_false(
; CHECK:         store i1 false, ptr %out
; CHECK-NOT:     call i32 @llvm.genx.GenISA.simdSize
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %cmp = icmp eq i32 %sz, 8
  store i1 %cmp, ptr %out, align 1
  ret void
}

; Select folded through comparison: select (simdSize == 16), 1, 0 => 1
define spir_kernel void @test_select(ptr %out) {
; CHECK-LABEL: @test_select(
; CHECK:         store i32 1, ptr %out
; CHECK-NOT:     call i32 @llvm.genx.GenISA.simdSize
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %cmp = icmp eq i32 %sz, 16
  %sel = select i1 %cmp, i32 1, i32 0
  store i32 %sel, ptr %out, align 4
  ret void
}

; Chain: (simdSize + 4) * 2 => (16+4)*2 = 40
define spir_kernel void @test_chain(ptr %out) {
; CHECK-LABEL: @test_chain(
; CHECK:         store i32 40, ptr %out
; CHECK-NOT:     call i32 @llvm.genx.GenISA.simdSize
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %add = add i32 %sz, 4
  %mul = mul i32 %add, 2
  store i32 %mul, ptr %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.simdSize()
