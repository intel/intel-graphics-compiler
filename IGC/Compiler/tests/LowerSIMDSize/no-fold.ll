;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lower-simd-size -S < %s | FileCheck %s
; ------------------------------------------------
; LowerSIMDSize: no-op when SIMD mode is UNKNOWN or END
; ------------------------------------------------

; The pass is a no-op under two conditions:
;
;   1. SIMDMode::UNKNOWN (tested here)
;      -- ctx->m_CurSimdMode has not been set yet, which is the case in an
;         igc_opt session where no --igc-lower-simd-size-override is given.
;
;   2. SIMDMode::END (not directly testable via igc_opt)
;      -- ctx->m_CurSimdMode will be SIMDMode::END when multiple EmitVISA passes
;         with different SIMD modes are added to the same PassManager (e.g. SIMD16
;         + SIMD8). Because lowering GenISA_simdSize() to a specific constant simd
;         size would produce wrong code for all but one EmitPass.
;         so the pass opts out.
;
; In both cases the GenISA_simdSize() intrinsic must be preserved unchanged.

define spir_kernel void @test_no_fold(ptr %out) {
; CHECK-LABEL: @test_no_fold(
; CHECK:         [[SZ:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:         store i32 [[SZ]], ptr %out
; CHECK:         ret void

  %sz = call i32 @llvm.genx.GenISA.simdSize()
  store i32 %sz, ptr %out, align 4
  ret void
}

; Also verify that non-simdSize arithmetic is untouched.

define spir_kernel void @test_no_fold_arith(i32 %a, ptr %out) {
; CHECK-LABEL: @test_no_fold_arith(
; CHECK:         [[SZ:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:         [[MUL:%.*]] = mul i32 [[SZ]], %a
; CHECK:         store i32 [[MUL]], ptr %out
; CHECK:         ret void

  %sz  = call i32 @llvm.genx.GenISA.simdSize()
  %mul = mul i32 %sz, %a
  store i32 %mul, ptr %out, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.simdSize()
