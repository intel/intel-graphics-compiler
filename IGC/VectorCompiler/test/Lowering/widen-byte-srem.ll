;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; widenByteOp widens a byte operation to short when its result feeds a zext.
; For a signed byte remainder the operands must be sign-extended (not zero-
; extended), otherwise negative bytes are corrupted.

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=Xe2 -S < %s | FileCheck %s

; CHECK-LABEL: @test
; CHECK-NOT: zext <16 x i8> %a to <16 x i16>
; CHECK: %[[EA:[^ ]+]] = sext <16 x i8> %a to <16 x i16>
; CHECK: %[[EB:[^ ]+]] = sext <16 x i8> %b to <16 x i16>
; CHECK: srem <16 x i16> %[[EA]], %[[EB]]

define <16 x i32> @test(<16 x i8> %a, <16 x i8> %b) {
  %rem = srem <16 x i8> %a, %b
  %ext = zext <16 x i8> %rem to <16 x i32>
  ret <16 x i32> %ext
}
