;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -S < %s | FileCheck %s

; CHECK-LABEL: test
; CHECK-NEXT: %.sext.reduceintsize = sext <16 x i8> %arg to <16 x i16>
; CHECK-NEXT: %.sext.reduceintsize1 = sext <16 x i8> %arg to <16 x i16>
; CHECK-NEXT: %res = mul <16 x i16> %.sext.reduceintsize, %.sext.reduceintsize1
define <16 x i16> @test(<16 x i8> %arg) {
  %.sext = sext <16 x i8> %arg to <16 x i32>
  %res = mul nsw <16 x i32> %.sext, %.sext
  %.trunc= trunc <16 x i32> %res to <16 x i16>
  ret <16 x i16> %.trunc
}

