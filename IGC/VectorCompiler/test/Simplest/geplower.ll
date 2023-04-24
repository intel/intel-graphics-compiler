;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; CHECK: llvm.genx.thread.x

define dllexport spir_kernel void @linear(i32 %0, i32 %1) {
  %3 = tail call i16 @llvm.genx.thread.x()
  %4 = zext i16 %3 to i32
  %5 = mul nuw nsw i32 %4, 24
  %6 = tail call i16 @llvm.genx.thread.y()
  %7 = zext i16 %6 to i32
  %8 = mul nuw nsw i32 %7, 6
  %9 = tail call <256 x i8> @llvm.genx.media.ld.v256i8(i32 0, i32 %0, i32 0, i32 32, i32 %5, i32 %8)
  %.regioncollapsed58 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 35, i32 undef)
  %10 = uitofp <144 x i8> %.regioncollapsed58 to <144 x float>
  %.regioncollapsed57 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 0, i32 undef)
  %11 = uitofp <144 x i8> %.regioncollapsed57 to <144 x float>
  %12 = fadd <144 x float> %10, %11
  %.regioncollapsed56 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 3, i32 undef)
  %13 = uitofp <144 x i8> %.regioncollapsed56 to <144 x float>
  %14 = fadd <144 x float> %12, %13
  %.regioncollapsed55 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 6, i32 undef)
  %15 = uitofp <144 x i8> %.regioncollapsed55 to <144 x float>
  %16 = fadd <144 x float> %14, %15
  %.regioncollapsed54 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 32, i32 undef)
  %17 = uitofp <144 x i8> %.regioncollapsed54 to <144 x float>
  %18 = fadd <144 x float> %16, %17
  %.regioncollapsed53 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 38, i32 undef)
  %19 = uitofp <144 x i8> %.regioncollapsed53 to <144 x float>
  %20 = fadd <144 x float> %18, %19
  %.regioncollapsed52 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 64, i32 undef)
  %21 = uitofp <144 x i8> %.regioncollapsed52 to <144 x float>
  %22 = fadd <144 x float> %20, %21
  %.regioncollapsed51 = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 67, i32 undef)
  %23 = uitofp <144 x i8> %.regioncollapsed51 to <144 x float>
  %24 = fadd <144 x float> %22, %23
  %.regioncollapsed = tail call <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8> %9, i32 32, i32 24, i32 1, i16 70, i32 undef)
  %25 = uitofp <144 x i8> %.regioncollapsed to <144 x float>
  %26 = fadd <144 x float> %24, %25
  %27 = fmul <144 x float> %26, <float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000, float 0x3FBC6A7F00000000>
  %28 = fptosi <144 x float> %27 to <144 x i32>
  %29 = trunc <144 x i32> %28 to <144 x i8>
  %30 = tail call <192 x i8> @llvm.genx.wrregioni.v192i8.v144i8.i16.i1(<192 x i8> undef, <144 x i8> %29, i32 32, i32 24, i32 1, i16 0, i32 32, i1 true)
  tail call void @llvm.genx.media.st.v192i8(i32 0, i32 %1, i32 0, i32 24, i32 %5, i32 %8, <192 x i8> %30)
  ret void
}

declare i16 @llvm.genx.thread.x()

declare i16 @llvm.genx.thread.y()

declare <256 x i8> @llvm.genx.media.ld.v256i8(i32, i32, i32, i32, i32, i32)

declare <192 x i8> @llvm.genx.wrregioni.v192i8.v144i8.i16.i1(<192 x i8>, <144 x i8>, i32, i32, i32, i16, i32, i1)

declare <144 x i8> @llvm.genx.rdregioni.v144i8.v256i8.i16(<256 x i8>, i32, i32, i32, i16, i32)

declare void @llvm.genx.media.st.v192i8(i32, i32, i32, i32, i32, i32, <192 x i8>)

