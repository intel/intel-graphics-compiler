;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -mcpu=Gen9 -march=genx64 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Function Attrs: nounwind
define <16 x i16> @foo(i32 %surfaceId) {
entry:
  %0 = call <16 x i16> @llvm.genx.wrregioni.v16i16.i16.i16.i1(<16 x i16> undef, i16 undef, i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
  %1 = shl <16 x i16> undef, <i16 4, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef>
  %2 = add <16 x i16> %1, %0
  %3 = shl <16 x i16> %2, <i16 2, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef, i16 undef>

; COM: really this test just checks that on this context anything (including no transformation) happen without bugs
; CHECK: ret <16 x i16> %{{[a-zA-Z0-9]+}}
  ret <16 x i16> %3
}

; Function Attrs: nounwind readnone
declare <16 x i16> @llvm.genx.wrregioni.v16i16.i16.i16.i1(<16 x i16>, i16, i32, i32, i32, i16, i32, i1) #1
