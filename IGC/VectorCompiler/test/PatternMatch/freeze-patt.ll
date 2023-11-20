;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_freeze(
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 undef, i32 undef, i32 undef, i32 1, i16 0, i32 0, i1 undef)
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 undef, i32 undef, i32 poison, i32 1, i16 0, i32 0, i1 true)
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 undef, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
define spir_kernel void @test_freeze(i32 %arg) {
  %w = add i32 %arg, undef
  %x = freeze i32 %w
  %y = add i32 %w, %w         ; undef
  %z = add i32 %x, %x         ; even number because all uses of %x observe
                              ; the same value
  %x2 = freeze i32 %w
  %cmp = icmp eq i32 %x, %x2  ; can be true or false

  %wrreg1 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 %x2, i32 %y, i32 %z, i32 1, i16 0, i32 0, i1 %cmp)

  ; example with vectors
  %v = add <2 x i32> <i32 undef, i32 poison>, zeroinitializer
  %a = extractelement <2 x i32> %v, i32 0    ; undef
  %b = extractelement <2 x i32> %v, i32 1    ; poison
  %add = add i32 %a, %a                      ; undef

  %wrreg2 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 %add, i32 %a, i32 %b, i32 1, i16 0, i32 0, i1 true)

  %v.fr = freeze <2 x i32> %v                ; element-wise freeze
  %d = extractelement <2 x i32> %v.fr, i32 0 ; not undef
  %add.f = add i32 %d, %d                    ; even number

  %wrreg3 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32> undef, i32 %add.f, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

declare <16 x i32> @llvm.genx.wrregionf.v16i32.i32.i16.i1(<16 x i32>, i32, i32, i32, i32, i16, i32, i1)