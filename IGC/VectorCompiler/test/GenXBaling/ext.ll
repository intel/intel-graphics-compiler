;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info=true -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck --match-full-lines %s
; ------------------------------------------------
; GenXBaling
; ------------------------------------------------
; This test checks that GenXBaling doesn't bale zext when the modifier is sext and vice versa

define <16 x i32> @sext_to_zext(<16 x i8> %arg) {
; CHECK: bales in function: sext_to_zext:
; CHECK-NEXT: %sext = sext <16 x i8> %arg to <16 x i16>: sext
; CHECK-NEXT: %zext = zext <16 x i16> %sext to <16 x i32>: zext{{$}}
  %sext = sext <16 x i8> %arg to <16 x i16>
  %zext = zext <16 x i16> %sext to <16 x i32>
  ret <16 x i32> %zext
}

define <16 x i32> @zext_to_sext(<16 x i8> %arg) {
; CHECK: bales in function: zext_to_sext:
; CHECK-NEXT: %zext = zext <16 x i8> %arg to <16 x i16>: zext
; CHECK-NEXT: %sext = sext <16 x i16> %zext to <16 x i32>: sext{{$}}
  %zext = zext <16 x i8> %arg to <16 x i16>
  %sext = sext <16 x i16> %zext to <16 x i32>
  ret <16 x i32> %sext
}
