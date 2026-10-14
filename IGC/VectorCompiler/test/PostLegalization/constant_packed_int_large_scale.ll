;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

;; The packed int scale (2^31) does not fit a signed i32 and must wrap.

define <8 x i32> @test_scale_2pow31() {
  ; CHECK-LABEL: @test_scale_2pow31(
  ; CHECK: mul <8 x i32> %{{.*}}, {{(splat \(i32 -2147483648\)|<i32 -2147483648(, i32 -2147483648)*>)}}
  ret <8 x i32> <i32 -2147483648, i32 0, i32 0, i32 -2147483648, i32 0, i32 -2147483648, i32 -2147483648, i32 0>
}
