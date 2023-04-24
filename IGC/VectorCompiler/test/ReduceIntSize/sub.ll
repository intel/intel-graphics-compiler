;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK: %conv = zext <16 x i8> %val to <16 x i64>
; CHECK-NEXT: %sub = sub nuw nsw <16 x i64> <i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207>, %conv
; CHECK-NEXT: ret <16 x i64> %sub

define <16 x i64> @test(<16 x i8> %val) {
  %conv = zext <16 x i8> %val to <16 x i64>
  %sub = sub nuw nsw <16 x i64> <i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207, i64 7231966207>, %conv
  ret <16 x i64> %sub
}
