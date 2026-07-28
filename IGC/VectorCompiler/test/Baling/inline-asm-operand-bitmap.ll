;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info -disable-output \
; RUN: -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s 2>&1 | FileCheck %s

declare <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) nounwind readnone

; Warning check
; CHECK: Inline asm baling failed for{{.*}} operand #16 is not baled, at most 16 operands can be baled.

; The 34 argument asm bales in operands 0..15 and nothing beyond them.
; CHECK: bales in function: test
; CHECK: call void asm sideeffect "nop %0 {{.*}}: maininst 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15{{$}}

; Baling into an inline asm whose operands all fit is unaffected.
; CHECK: call void asm sideeffect "nop %0 %1 %2"{{.*}}: maininst 0 1 2{{$}}

define void @test(<8 x i32> %src) {
  ; 34 rdregions, one per inline asm argument.
  %rd0 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd1 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd2 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd3 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd4 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd5 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd6 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd7 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd8 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd9 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd10 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd11 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd12 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd13 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd14 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd15 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd16 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd17 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd18 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd19 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd20 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd21 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd22 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd23 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd24 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd25 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd26 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd27 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd28 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd29 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd30 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd31 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd32 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %rd33 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  call void asm sideeffect "nop %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16 %17 %18 %19 %20 %21 %22 %23 %24 %25 %26 %27 %28 %29 %30 %31 %32 %33", "r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r,r"(<8 x i32> %rd0, <8 x i32> %rd1, <8 x i32> %rd2, <8 x i32> %rd3, <8 x i32> %rd4, <8 x i32> %rd5, <8 x i32> %rd6, <8 x i32> %rd7, <8 x i32> %rd8, <8 x i32> %rd9, <8 x i32> %rd10, <8 x i32> %rd11, <8 x i32> %rd12, <8 x i32> %rd13, <8 x i32> %rd14, <8 x i32> %rd15, <8 x i32> %rd16, <8 x i32> %rd17, <8 x i32> %rd18, <8 x i32> %rd19, <8 x i32> %rd20, <8 x i32> %rd21, <8 x i32> %rd22, <8 x i32> %rd23, <8 x i32> %rd24, <8 x i32> %rd25, <8 x i32> %rd26, <8 x i32> %rd27, <8 x i32> %rd28, <8 x i32> %rd29, <8 x i32> %rd30, <8 x i32> %rd31, <8 x i32> %rd32, <8 x i32> %rd33)
  ; A small inline asm, for which every operand fits in the bitmap.
  %small0 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %small1 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %small2 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v8i32.i16(<8 x i32> %src, i32 0, i32 8, i32 1, i16 0, i32 undef)
  call void asm sideeffect "nop %0 %1 %2", "r,r,r"(<8 x i32> %small0, <8 x i32> %small1, <8 x i32> %small2)
  ret void
}
