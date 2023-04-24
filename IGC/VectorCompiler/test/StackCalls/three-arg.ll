;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -dce -mattr=+ocl_runtime -march=genx64 \
; RUN: -mcpu=Gen9 -S -vc-arg-reg-size=32 -vc-ret-reg-size=12 < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM: note three arguments, 1 and 2 are used
define <16 x float> @foo3(<16 x float> %0, <16 x float> %1, <16 x float> %2) #0 {
entry:
  %add.i = fadd <16 x float> %1, %2
  ret <16 x float> %add.i
; COM: offset of 1 shall be 64
; CHECK: @llvm.genx.rdregionf.v16f32.v256f32.i16(<256 x float> %{{[a-zA-Z0-9]+}}, i32 0, i32 16, i32 1, i16 64, i32 undef)
; COM: offset of 2 shall be 128
; CHECK: @llvm.genx.rdregionf.v16f32.v256f32.i16(<256 x float> %{{[a-zA-Z0-9]+}}, i32 0, i32 16, i32 1, i16 128, i32 undef)
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }

