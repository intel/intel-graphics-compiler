;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

;; check legalization of constant vector == range(0,16)
define <16 x i32> @legalize_integer_range_0_16() {
; CHECK-LABEL: @legalize_integer_range_0_16(
; CHECK-NEXT:  [[CONST0:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT:  [[SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[CONST1:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 10, i32 9, i32 8, i32 7>)
; CHECK-NEXT:  [[JOIN:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT0]], <8 x i32> [[CONST1]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT:  ret <16 x i32> [[JOIN]]
  ret <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 10, i32 9, i32 8, i32 7>
}

;; check legalization of constant vector == 10 + range(0,16)
define <16 x i32> @legalize_integer_range_10_26() {
; CHECK-LABEL: @legalize_integer_range_10_26(
; CHECK-NEXT:  [[CONST0:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT:  [[SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[CONST1:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 9, i32 10, i32 9, i32 8, i32 7, i32 6, i32 5>)
; CHECK-NEXT:  [[JOIN:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT0]], <8 x i32> [[CONST1]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT:  [[ADD:%.*]] = add <16 x i32> [[JOIN]], <i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10>
; CHECK-NEXT:  ret <16 x i32> [[ADD]]
  ret <16 x i32> <i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 19, i32 18, i32 17, i32 16, i32 15>
}

;; check legalization of constant vector == 10 + range(0, 240, 16)
define <16 x i32> @legalize_integer_range_0_250_16() {
; CHECK-LABEL: @legalize_integer_range_0_250_16(
; CHECK-NEXT:  [[CONST0:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT:  [[SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[PRE_ADD:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>)
; CHECK-NEXT:  [[ADD:%.*]] = add <8 x i32> [[CONST0]], [[PRE_ADD]]
; CHECK-NEXT:  [[JOIN:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT0]], <8 x i32> [[ADD]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT:  [[MUL:%.*]] = mul <16 x i32> [[JOIN]], <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
; CHECK-NEXT:  [[ADD:%.*]] = add <16 x i32> [[MUL]], <i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10>
; CHECK-NEXT:  ret <16 x i32> [[ADD]]
  ret <16 x i32> <i32 10, i32 26, i32 42, i32 58, i32 74, i32 90, i32 106, i32 122, i32 138, i32 154, i32 170, i32 186, i32 202, i32 218, i32 234, i32 250>
}

;; check legalization of constant vector == range(0, 240, 16)
define <16 x i32> @legalize_integer_range_0_240_16() {
; CHECK-LABEL: @legalize_integer_range_0_240_16(
; CHECK-NEXT:  [[CONST0:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>)
; CHECK-NEXT:  [[SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  [[PRE_ADD:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>)
; CHECK-NEXT:  [[ADD:%.*]] = add <8 x i32> [[CONST0]], [[PRE_ADD]]
; CHECK-NEXT:  [[JOIN:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT0]], <8 x i32> [[ADD]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK-NEXT:  [[MUL:%.*]] = mul <16 x i32> [[JOIN]], <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
; CHECK-NEXT:  ret <16 x i32> [[MUL]]
  ret <16 x i32> <i32 0, i32 16, i32 32, i32 48, i32 64, i32 80, i32 96, i32 112, i32 128, i32 144, i32 160, i32 176, i32 192, i32 208, i32 224, i32 240>
}
