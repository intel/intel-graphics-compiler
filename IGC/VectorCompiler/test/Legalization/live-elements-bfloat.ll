;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; SPLIT 1 FOR BFLOAT
; Unlike half, bfloat does not require exec size 8 or 16 in the finalizer,
; so dead element splitting is allowed.
; CHECK-LABEL: @test1
; CHECK: fadd <1 x bfloat>
define <16 x i16> @test1(<16 x bfloat> %arg) {
  %add = fadd <16 x bfloat> %arg, <bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0, bfloat 1.0>
  %int = fptoui <16 x bfloat> %add to <16 x i16>
  %and = and <16 x i16> %int, <i16 1, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0, i16 0>
  ret <16 x i16> %and
}
