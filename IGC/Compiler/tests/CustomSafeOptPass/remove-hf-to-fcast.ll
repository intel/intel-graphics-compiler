;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: removeHftoFCast
; ------------------------------------------------

; A half fadd of two half->float fmul/add (fmad) operands that all originate
; from float->half fptrunc, and is consumed by a single half->float fpext, is
; rebuilt directly in float so the half round-trips disappear.

define float @test_hf_fmad(float %a, float %b, float %c) {
; CHECK-LABEL: define float @test_hf_fmad(
; CHECK:    [[MUL:%.*]] = fmul float %a, %b
; CHECK:    [[ADD:%.*]] = fadd fast float [[MUL]], %c
; CHECK-NOT: fpext
; CHECK:    ret float [[ADD]]
;
  %ta = fptrunc float %a to half
  %tb = fptrunc float %b to half
  %tc = fptrunc float %c to half
  %mul = fmul fast half %ta, %tb
  %add = fadd fast half %mul, %tc
  %ext = fpext half %add to float
  ret float %ext
}

; fmad pattern is detected, but the third (addend) operand is a constant rather
; than a fptrunc/instruction, so getFloatValue() returns null and the transform
; is abandoned: the half fadd and fpext are left untouched.

define float @test_hf_fmad_const_addend(float %a, float %b) {
; CHECK-LABEL: define float @test_hf_fmad_const_addend(
; CHECK:    [[ADD:%.*]] = fadd fast half %{{.*}}, 0xH3C00
; CHECK:    [[EXT:%.*]] = fpext half [[ADD]] to float
; CHECK:    ret float [[EXT]]
;
  %ta = fptrunc float %a to half
  %tb = fptrunc float %b to half
  %mul = fmul fast half %ta, %tb
  %add = fadd fast half %mul, 0xH3C00
  %ext = fpext half %add to float
  ret float %ext
}

; Not an fmad (no fmul operand) and only one operand is a float->half fptrunc,
; so S2 stays null and the non-fmad rewrite bails out: nothing changes.

define float @test_hf_add_one_trunc(float %a, half %b) {
; CHECK-LABEL: define float @test_hf_add_one_trunc(
; CHECK:    [[ADD:%.*]] = fadd half %{{.*}}, %b
; CHECK:    [[EXT:%.*]] = fpext half [[ADD]] to float
; CHECK:    ret float [[EXT]]
;
  %ta = fptrunc float %a to half
  %add = fadd half %ta, %b
  %ext = fpext half %add to float
  ret float %ext
}

; Both operands of a non-fmad half binary op are float->half fptrunc, so the op
; is rebuilt in float and the half round-trip is removed.

define float @test_hf_add_two_trunc(float %a, float %b) {
; CHECK-LABEL: define float @test_hf_add_two_trunc(
; CHECK:    [[ADD:%.*]] = fadd float %a, %b
; CHECK-NOT: fpext
; CHECK:    ret float [[ADD]]
;
  %ta = fptrunc float %a to half
  %tb = fptrunc float %b to half
  %add = fadd half %ta, %tb
  %ext = fpext half %add to float
  ret float %ext
}
