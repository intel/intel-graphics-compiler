;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -mcpu=Xe3P -march=genx64 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=UseMadDDQ
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -mcpu=XeLPG -march=genx64 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; UseMadDDQ-LABEL: foo_bcast_I64ToI32
; UseMadDDQ: [[BI32:%[^ ]+]] = bitcast <8 x i64> %data to <16 x i32>
; UseMadDDQ: %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[BI32]], <16 x i32> [[BI32]], <16 x i32> [[BI32]])
define <16 x i32> @foo_bcast_I64ToI32(i64 %ibufA, <8 x i64> %data) {
entry:
  %0 = bitcast <8 x i64> %data to <16 x i32>
  %mul = mul <16 x i32> %0, %0
  %add = add <16 x i32> %mul, %0
  ret <16 x i32> %add
}

; CHECK-LABEL: foo_bcast_I32ToI16
; CHECK: [[BI16:%[^ ]+]] = bitcast <8 x i32> %data to <16 x i16>
; CHECK: %mad = call <16 x i16> @llvm.genx.ssmad.v16i16.v16i16(<16 x i16> [[BI16]], <16 x i16> [[BI16]], <16 x i16> [[BI16]])
define <16 x i16> @foo_bcast_I32ToI16(i64 %ibufA, <8 x i32> %data) {
entry:
  %0 = bitcast <8 x i32> %data to <16 x i16>
  %mul = mul <16 x i16> %0, %0
  %add = add <16 x i16> %mul, %0
  ret <16 x i16> %add
}

; COM: Bitcast related changes are not applicable to zext/sext insts.
; CHECK-LABEL: foo_sext_I16ToI32
; CHECK: [[SE:%[^ ]+]] = sext <8 x i16> %data to <8 x i32>
; CHECK: %mad = call <8 x i32> @llvm.genx.ssmad.v8i32.v8i16(<8 x i16> %data, <8 x i16> %data, <8 x i32> [[SE]])
define <8 x i32> @foo_sext_I16ToI32(i64 %ibufA, <8 x i16> %data) {
entry:
  %0 = sext <8 x i16> %data to <8 x i32>
  %mul = mul <8 x i32> %0, %0
  %add = add <8 x i32> %mul, %0
  ret <8 x i32> %add
}