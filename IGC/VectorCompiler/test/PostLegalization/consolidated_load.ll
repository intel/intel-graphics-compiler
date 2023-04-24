;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

;; Test that constant materilization works as expected in case of consolidated constants

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; CHECK: @consolidated_load_splat
; CHECK-NEXT: xor <16 x i8> %arg, <i8 -1, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef>
define <16 x i8> @consolidated_load_splat_with_undefs(<16 x i8> %arg) {
  %val = xor <16 x i8> %arg, <i8 -1, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef>
  ret <16 x i8> %val
}

; CHECK: @consolidated_load_simple
; CHECK-NEXT: [[INIT_CONST:%[^ ]+]] = call <1 x i32> @llvm.genx.constanti.v1i32(<1 x i32> <i32 2071558244>)
; CHECK-NEXT: [[RDREG_SPLAT:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[INIT_CONST]], i32 0, i32 4, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[C2:%[^ ]+]] = call <4 x i32> @llvm.genx.wrconstregion.v4i32.v1i32.i16.i1(<4 x i32> [[RDREG_SPLAT]], <1 x i32> <i32 -117835013>, i32 1, i32 1, i32 1, i16 8, i32 undef, i1 true)
; CHECK-NEXT: [[C3:%[^ ]+]] = call <4 x i32> @llvm.genx.wrconstregion.v4i32.v1i32.i16.i1(<4 x i32> [[C2]], <1 x i32> <i32 67305985>, i32 1,
; CHECK-NEXT: [[C4:%[^ ]+]] = call <4 x i32> @llvm.genx.wrconstregion.v4i32.v1i32.i16.i1(<4 x i32> [[C3]], <1 x i32> <i32 -50462977>, i32 1, i32 1, i32 1, i16 0, i32 undef, i1 true)
; CHECK-NEXT: [[BITCASTED:%[^ ]+]] = bitcast <4 x i32> [[C4]] to <16 x i8>
; CHECK-NEXT: xor <16 x i8> %arg, [[BITCASTED]]
define <16 x i8> @consolidated_load_simple(<16 x i8> %arg) {
  %val = xor <16 x i8> %arg, <i8 -1, i8 -2, i8 -3, i8 -4, i8 1, i8 2, i8 3, i8 4, i8 -5, i8 -6, i8 -7, i8 -8, i8 100, i8 120, i8 121, i8 123>
  ret <16 x i8> %val
}
