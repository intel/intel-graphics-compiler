;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <3 x i32> @llvm.genx.rdregioni.v3i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32)
declare i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32>, i32, i32, i32, i16, i32)
declare <16 x i32> @llvm.genx.wrregioni.v16i32.i32.i16.i1(<16 x i32>, i32, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: test
define <16 x i32> @test(<3 x i32> %arg) {
  ; CHECK: [[SPLIT0:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v3i32.i16(<3 x i32> %arg, i32 0, i32 1, i32 0, i16 0, i32 undef)
  ; CHECK: [[JOIN0:%[^ ]+]] = call <3 x i32> @llvm.genx.wrregioni.v3i32.v2i32.i16.i1(<3 x i32> undef, <2 x i32> [[SPLIT0]], i32 0, i32 2, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[SPLIT2:%[^ ]+]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v3i32.i16(<3 x i32> %arg, i32 0, i32 1, i32 0, i16 0, i32 undef)
  ; CHECK: [[JOIN2:%[^ ]+]] = call <3 x i32> @llvm.genx.wrregioni.v3i32.v1i32.i16.i1(<3 x i32> %shift.split0.join0, <1 x i32> %shift.split2, i32 0, i32 1, i32 1, i16 8, i32 undef, i1 true)
  %shift = call <3 x i32> @llvm.genx.rdregioni.v3i32.v3i32.i16(<3 x i32> %arg, i32 0, i32 1, i32 0, i16 0, i32 0)
  ; CHECK: %extr = call i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32> [[JOIN2]], i32 0, i32 1, i32 0, i16 0, i32 0)
  %extr = call i32 @llvm.genx.rdregioni.i32.v3i32.i16(<3 x i32> %shift, i32 0, i32 1, i32 0, i16 0, i32 0)
  %ins = call <16 x i32> @llvm.genx.wrregioni.v16i32.i32.i16.i1(<16 x i32> zeroinitializer, i32 %extr, i32 0, i32 1, i32 0, i16 0, i32 0, i1 false)
  ret <16 x i32> %ins
}
