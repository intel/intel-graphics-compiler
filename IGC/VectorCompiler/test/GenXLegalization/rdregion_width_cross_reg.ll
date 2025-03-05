
;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8>, <8 x i8>, i32, i32, i32, i16, i32, i1)

declare <4 x float> @llvm.genx.rdregionf.v4f32.v36f32.i16(<36 x float>, i32, i32, i32, i16, i32)

declare <8 x float> @llvm.genx.wrregionf.v8f32.v4f32.i16.i1(<8 x float>, <4 x float>, i32, i32, i32, i16, i32, i1)

declare <3 x float> @llvm.genx.rdregionf.v3f32.v8f32.i16(<8 x float>, i32, i32, i32, i16, i32)

declare <96 x i8> @llvm.genx.wrregioni.v96i8.v3i8.i16.i1(<96 x i8>, <3 x i8>, i32, i32, i32, i16, i32, i1)

declare <128 x i8> @llvm.genx.wrregioni.v128i8.v96i8.i16.i1(<128 x i8>, <96 x i8>, i32, i32, i32, i16, i32, i1)

declare <36 x i8> @llvm.genx.rdregioni.v36i8.v256i8.i16(<256 x i8>, i32, i32, i32, i16, i32)

declare <8 x i8> @llvm.genx.rdregioni.v8i8.v96i8.i16(<96 x i8>, i32, i32, i32, i16, i32)

declare void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v128i8(<3 x i8>, i32, i32, i32, i32, i32, <128 x i8>)

declare <3 x i8> @llvm.genx.rdregioni.v3i8.v12i8.i16(<12 x i8>, i32, i32, i32, i16, i32)

; CHECK-LABEL: @horizontal_blur
define spir_kernel void @horizontal_blur() {
.lr.ph.preheader:
  br label %.lr.ph

.lr.ph:                                           ; preds = %.lr.ph.preheader
; CHECK: [[SPLIT1:%[^ ]+]] = call <2 x i16> @llvm.genx.rdregioni.v2i16.v48i16.i16(<48 x i16> zeroinitializer, i32 1, i32 2, i32 1, i16 60, i32 undef)
; CHECK: [[JOIN1:%[^ ]+]] = call <128 x i16> @llvm.genx.wrregioni.v128i16.v2i16.i16.i1(<128 x i16> zeroinitializer, <2 x i16> [[SPLIT1]], i32 2, i32 2, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[SPLIT2:%[^ ]+]] = call <2 x i16> @llvm.genx.rdregioni.v2i16.v48i16.i16(<48 x i16> zeroinitializer, i32 1, i32 2, i32 1, i16 62, i32 undef)
; CHECK: [[JOIN2:%[^ ]+]] = call <128 x i16> @llvm.genx.wrregioni.v128i16.v2i16.i16.i1(<128 x i16> [[JOIN1]], <2 x i16> [[SPLIT2]], i32 2, i32 2, i32 1, i16 4, i32 undef, i1 true)
; CHECK: bitcast <128 x i16> [[JOIN2]] to <256 x i8>

  %.regioncollapsed337.regioncollapsed = tail call <8 x i8> @llvm.genx.rdregioni.v8i8.v96i8.i16(<96 x i8> zeroinitializer, i32 2, i32 4, i32 1, i16 60, i32 0)
  %0 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> %.regioncollapsed337.regioncollapsed, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  %1 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %2 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %3 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %4 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %5 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %6 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %7 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %8 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %9 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %10 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %11 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %12 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %13 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %14 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %15 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %16 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %17 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %18 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %19 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %20 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %21 = tail call <256 x i8> @llvm.genx.wrregioni.v256i8.v8i8.i16.i1(<256 x i8> zeroinitializer, <8 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %.regioncollapsed284 = tail call <36 x i8> @llvm.genx.rdregioni.v36i8.v256i8.i16(<256 x i8> %0, i32 0, i32 0, i32 0, i16 0, i32 0)
  %22 = uitofp <36 x i8> %.regioncollapsed284 to <36 x float>
  %23 = fmul <36 x float> zeroinitializer, %22
  %24 = tail call <4 x float> @llvm.genx.rdregionf.v4f32.v36f32.i16(<36 x float> %22, i32 0, i32 0, i32 0, i16 0, i32 0)
  %25 = fadd <4 x float> %24, zeroinitializer
  %26 = tail call <8 x float> @llvm.genx.wrregionf.v8f32.v4f32.i16.i1(<8 x float> zeroinitializer, <4 x float> %24, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %27 = tail call <3 x float> @llvm.genx.rdregionf.v3f32.v8f32.i16(<8 x float> %26, i32 0, i32 0, i32 0, i16 0, i32 0)
  %28 = fadd <3 x float> zeroinitializer, %27
  %29 = fptosi <3 x float> %27 to <3 x i32>
  %30 = bitcast <3 x i32> %29 to <12 x i8>
  %31 = call <3 x i8> @llvm.genx.rdregioni.v3i8.v12i8.i16(<12 x i8> %30, i32 0, i32 0, i32 0, i16 0, i32 0)
  %32 = tail call <96 x i8> @llvm.genx.wrregioni.v96i8.v3i8.i16.i1(<96 x i8> zeroinitializer, <3 x i8> %31, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %33 = tail call <96 x i8> @llvm.genx.wrregioni.v96i8.v3i8.i16.i1(<96 x i8> zeroinitializer, <3 x i8> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  %34 = tail call <128 x i8> @llvm.genx.wrregioni.v128i8.v96i8.i16.i1(<128 x i8> zeroinitializer, <96 x i8> %32, i32 0, i32 0, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v3i8.v128i8(<3 x i8> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i32 0, <128 x i8> %34)
  ret void
}
