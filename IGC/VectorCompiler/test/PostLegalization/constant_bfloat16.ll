;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

;; Test legalization of bfloat16 constant

; CHECK-LABEL: @const_vector
; CHECK-NEXT: [[OP1:%.+]] = bitcast <4 x i64> %0 to <16 x bfloat>
; CHECK-NEXT: [[CONST1:%.+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 -1049607840>)
; CHECK-NEXT: [[CONSTPLAT:%.+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v1i32.i16(<1 x i32> [[CONST1]], i32 0, i32 8, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[CONST2:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONSTPLAT:%.+]], <1 x i32> <i32 -1051705024>, i32 1, i32 1, i32 1, i16 24, i32 undef, i1 true)
; CHECK-NEXT: [[CONST3:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST2]], <1 x i32> <i32 -1053802208>, i32 1, i32 1, i32 1, i16 20, i32 undef, i1 true)
; CHECK-NEXT: [[CONST4:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST3]], <1 x i32> <i32 -1055899392>, i32 1, i32 1, i32 1, i16 16, i32 undef, i1 true)
; CHECK-NEXT: [[CONST5:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST4]], <1 x i32> <i32 -1059045184>, i32 1, i32 1, i32 1, i16 12, i32 undef, i1 true)
; CHECK-NEXT: [[CONST6:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST5]], <1 x i32> <i32 -1063239552>, i32 1, i32 1, i32 1, i16 8, i32 undef, i1 true)
; CHECK-NEXT: [[CONST7:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST6]], <1 x i32> <i32 -1069531136>, i32 1, i32 1, i32 1, i16 4, i32 undef, i1 true)
; CHECK-NEXT: [[CONST8:%.+]] = call <8 x i32> @llvm.genx.wrconstregion.v8i32.v1i32.i16.i1(<8 x i32> [[CONST7]], <1 x i32> <i32 -1082130432>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[CONST9:%.+]] = bitcast <8 x i32> [[CONST8]] to <16 x bfloat>
; CHECK-NEXT: fadd <16 x bfloat> [[OP1]], [[CONST9]]
define <4 x i64> @const_vector(<4 x i64> %0) {
  %2 = bitcast <4 x i64> %0 to <16 x bfloat>
  %3 = fadd <16 x bfloat> %2, <bfloat 0xR0000, bfloat 0xRBF80, bfloat 0xR4000, bfloat 0xRC040, bfloat 0xR4080, bfloat 0xRC0A0, bfloat 0xR40C0, bfloat 0xRC0E0, bfloat 0xR4100, bfloat 0xRC110, bfloat 0xR4120, bfloat 0xRC130, bfloat 0xR4140, bfloat 0xRC150, bfloat 0xR4160, bfloat 0xRC170>
  %4 = bitcast <16 x bfloat> %3 to <4 x i64>
  ret <4 x i64> %4
}
