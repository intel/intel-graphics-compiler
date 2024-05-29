;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i1 @llvm.genx.any.v16i1(<16 x i1>)

; CHECK-LABEL: @test(
define i1 @test(i16 %pred) {
; CHECK: [[CMP:%[^ ]+]] = icmp ne i16 %pred, 0
; CHECK: ret i1 [[CMP]]
  %cast = bitcast i16 %pred to <16 x i1>
  %any = call i1 @llvm.genx.any.v16i1(<16 x i1> %cast)
  ret i1 %any
}
