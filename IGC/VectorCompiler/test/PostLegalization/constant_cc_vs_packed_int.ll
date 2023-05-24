;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

;; This test verifies whether the materialization of a nonsimple constant is done at minimal cost.
;; The following notations for materialization algorithms are used:
;; P   - Packed.
;; CC  - Cyclic construction.
;; MIX - P & CC.

; Function Attrs: noinline nounwind
define <8 x i32> @test_vector_8_i32_P() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_8_i32_P(
  ; CHECK-NEXT:  [[CONST:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 1, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>)
  ; CHECK-NEXT:  [[SCALE:%.*]] = mul <8 x i32> [[CONST]], <i32 88, i32 88, i32 88, i32 88, i32 88, i32 88, i32 88, i32 88>
  ; CHECK-NEXT:  [[ADJUST:%.*]] = add <8 x i32> [[SCALE]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ; CHECK-NEXT:  ret <8 x i32> [[ADJUST]]
  ret <8 x i32> <i32 89, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
}

; Function Attrs: noinline nounwind
define <16 x i32> @test_vector_16_i32_P() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_16_i32_P(
  ; CHECK-NEXT:  [[CONST0:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 4, i32 2, i32 1, i32 8, i32 4, i32 2, i32 1>)
  ; CHECK-NEXT:  [[SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[CONST0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST1:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 8, i32 4, i32 2, i32 1, i32 8, i32 4, i32 2, i32 1>)
  ; CHECK-NEXT:  [[SPLIT1:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT0]], <8 x i32> [[CONST1]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT:  [[RESULT:%.*]] = mul <16 x i32> [[SPLIT1]], <i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10, i32 10>
  ; CHECK-NEXT:  ret <16 x i32> [[RESULT]]
  ret <16 x i32> <i32 80, i32 40, i32 20, i32 10, i32 80, i32 40, i32 20, i32 10, i32 80, i32 40, i32 20, i32 10, i32 80, i32 40, i32 20, i32 10>
}

; Function Attrs: noinline nounwind
define <16 x i32> @test_vector_16_i32_CC() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_16_i32_CC(
  ; CHECK-NEXT:  [[CONST0:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 1>)
  ; CHECK-NEXT:  [[SPLAT:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> [[CONST0]], i32 0, i32 16, i32 0, i16 0, i32 undef)
  ; CHECK-NEXT:  [[RESULT:%.*]] = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32> [[SPLAT]], <1 x i32> <i32 89>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT:  ret <16 x i32> [[RESULT]]
  ret <16 x i32> <i32 89, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
}

; Function Attrs: noinline nounwind
define <16 x i32> @test_vector_16_i32_MIX() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_16_i32_MIX(
  ; CHECK-NEXT:  [[CONST:%.*]] = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 0, i32 1, i32 0, i32 0, i32 0, i32 0, i32 0>)
  ; CHECK-NEXT:  [[SCALE:%.*]] = mul <8 x i32> [[CONST]], <i32 72, i32 72, i32 72, i32 72, i32 72, i32 72, i32 72, i32 72>
  ; CHECK-NEXT:  [[ADJUST:%.*]] = add <8 x i32> [[SCALE]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ; CHECK-NEXT:  [[SPLIT:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[ADJUST]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST0:%.*]] = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v8i32.i16.i1(<16 x i32> [[SPLIT]], <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, i32 1, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST1:%.*]] = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32> [[CONST0]], <1 x i32> <i32 3>, i32 1, i32 1, i32 1, i16 4, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST2:%.*]] = call <16 x i32> @llvm.genx.wrconstregion.v16i32.v1i32.i16.i1(<16 x i32> [[CONST1]], <1 x i32> <i32 89>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT:  ret <16 x i32> [[CONST2]]
  ret <16 x i32> <i32 89, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 73, i32 1, i32 1, i32 1, i32 1, i32 1>
}

; Function Attrs: noinline nounwind
define <32 x i32> @test_vector_32_i32_CC() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_32_i32_CC(
  ; CHECK-NEXT:  [[CONST0:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 89>)
  ; CHECK-NEXT:  [[SPLAT:%.*]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v1i32.i16(<1 x i32> [[CONST0]], i32 0, i32 32, i32 0, i16 0, i32 undef)
  ; CHECK-NEXT:  [[CONST1:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v8i32.i16.i1(<32 x i32> [[SPLAT]], <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, i32 1, i32 8, i32 1, i16 4, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST2:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v8i32.i16.i1(<32 x i32> [[CONST1]], <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, i32 1, i32 8, i32 1, i16 68, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST3:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v4i32.i16.i1(<32 x i32> [[CONST2]], <4 x i32> <i32 1, i32 1, i32 1, i32 1>, i32 1, i32 4, i32 1, i16 48, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST4:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v4i32.i16.i1(<32 x i32> [[CONST3]], <4 x i32> <i32 1, i32 1, i32 1, i32 1>, i32 1, i32 4, i32 1, i16 112, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST5:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v8i32.i16.i1(<32 x i32> [[CONST4]], <8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, i32 1, i32 8, i32 1, i16 12, i32 undef, i1 true)
  ; CHECK-NEXT:  [[CONST6:%.*]] = call <32 x i32> @llvm.genx.wrconstregion.v32i32.v2i32.i16.i1(<32 x i32> [[CONST5]], <2 x i32> <i32 1, i32 1>, i32 1, i32 2, i32 1, i16 100, i32 undef, i1 true)
  ; CHECK-NEXT:  ret <32 x i32> [[CONST6]]
  ret <32 x i32> <i32 89, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 89, i32 1, i32 1, i32 1, i32 1, i32 89, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 89, i32 1, i32 1, i32 1, i32 1>
}

; Function Attrs: noinline nounwind
define <32 x i8> @test_vector_32_i8_CC() local_unnamed_addr #1 {
  ; CHECK-LABEL: @test_vector_32_i8_CC(
  ; CHECK-NEXT:  [[CONST0:%.*]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 16843009>)
  ; CHECK-NEXT:  [[SPLAT:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> [[CONST0]], i32 0, i32 8, i32 0, i16 0, i32 undef)
  ; CHECK-NEXT:  [[BITCAST:%.*]] = bitcast <8 x i32> [[SPLAT]] to <32 x i8>
  ; CHECK-NEXT:  [[RESULT:%.*]] = call <32 x i8> @llvm.genx.wrconstregion.v32i8.v1i8.i16.i1(<32 x i8> [[BITCAST]], <1 x i8> <i8 89>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT:  ret <32 x i8> [[RESULT]]
  ret <32 x i8> <i8 89, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1, i8 1>
}

attributes #1 = { noinline nounwind }
