;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-lower-simd-size --igc-lower-simd-size-override=8  -S < %s | FileCheck %s --check-prefix=SIMD8
; RUN: igc_opt --opaque-pointers --igc-lower-simd-size --igc-lower-simd-size-override=16 -S < %s | FileCheck %s --check-prefix=SIMD16
; RUN: igc_opt --opaque-pointers --igc-lower-simd-size --igc-lower-simd-size-override=32 -S < %s | FileCheck %s --check-prefix=SIMD32
; ------------------------------------------------
; LowerSIMDSize: basic constant replacement
; ------------------------------------------------

; Verify that GenISA_simdSize() is replaced by the lane count constant for
; each of the three supported SIMD modes.

define spir_kernel void @test_basic(ptr %out) {
; SIMD8-LABEL:  @test_basic(
; SIMD8:          store i32 8, ptr %out
; SIMD8-NOT:      call i32 @llvm.genx.GenISA.simdSize
; SIMD8:          ret void
;
; SIMD16-LABEL: @test_basic(
; SIMD16:         store i32 16, ptr %out
; SIMD16-NOT:     call i32 @llvm.genx.GenISA.simdSize
; SIMD16:         ret void
;
; SIMD32-LABEL: @test_basic(
; SIMD32:         store i32 32, ptr %out
; SIMD32-NOT:     call i32 @llvm.genx.GenISA.simdSize
; SIMD32:         ret void

  %sz = call i32 @llvm.genx.GenISA.simdSize()
  store i32 %sz, ptr %out, align 4
  ret void
}

; Multiple calls in the same function are all replaced.

define spir_kernel void @test_multiple_calls(ptr %out0, ptr %out1) {
; SIMD8-LABEL: @test_multiple_calls(
; SIMD8:         store i32 8, ptr %out0
; SIMD8:         store i32 8, ptr %out1
; SIMD8-NOT:     call i32 @llvm.genx.GenISA.simdSize
;
; SIMD16-LABEL: @test_multiple_calls(
; SIMD16:         store i32 16, ptr %out0
; SIMD16:         store i32 16, ptr %out1
; SIMD16-NOT:     call i32 @llvm.genx.GenISA.simdSize

  %sz0 = call i32 @llvm.genx.GenISA.simdSize()
  %sz1 = call i32 @llvm.genx.GenISA.simdSize()
  store i32 %sz0, ptr %out0, align 4
  store i32 %sz1, ptr %out1, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.simdSize()
