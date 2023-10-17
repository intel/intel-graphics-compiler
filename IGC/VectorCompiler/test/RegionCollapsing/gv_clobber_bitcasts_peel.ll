;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s \
; RUN:   | FileCheck %s

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@All = external global <3584 x float> #0

declare <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare void @llvm.genx.raw.send2.noresult.v16i1.v32i16(i8, i8, <16 x i1>, i8, i8, i32, i32, <32 x i16>)
declare i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32)
declare <16 x float> @llvm.genx.rdregionf.v16f32.v3584f32.i16(<3584 x float>, i32, i32, i32, i16, i32)
attributes #0 = { "genx_volatile" }

define spir_kernel void @SGEMM_32x64() {
  %1 = load volatile <3584 x float>, <3584 x float>* @All, align 16384
  %.regioncollapsed6791 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v3584f32.i16(<3584 x float> %1, i32 0, i32 0, i32 0, i16 0, i32 0)
  %2 = bitcast <16 x float> %.regioncollapsed6791 to <16 x i32>
; COM: ------------------------------
; COM: The above sequence transforms to the below during RegionCollapsing execution:
; COM: ------------------------------
; COM:     %1 = load volatile <3584 x float>, <3584 x float>* @All, align 16384
; COM:     %2 = bitcast <3584 x float> %1 to <3584 x i32>
; COM:     %.regioncollapsed6791 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v3584i32.i16(<3584 x i32> %2, i32 0, i32 1, i32 0, i16 0, i32 undef)
; COM: ------------------------------
; COM: And later can get collapsed after the store to related global volatile value:
; COM: ------------------------------
; COM:     store volatile <3584 x float> zeroinitializer, <3584 x float>* @All, align 16384
; COM:     %sev.cast.2626157.regioncollapsed.regioncollapsed = call i32 @llvm.genx.rdregioni.i32.v3584i32.i16(<3584 x i32> %2, i32 0, i32 1, i32 0, i16 0, i32 undef)
; COM: ------------------------------
; COM: Further we check that the %.regioncollapsed6791 doesn't get collapsed with %sev.cast.2626157.regioncollapsed after the store to @All.
; COM: ------------------------------
; COM:
; CHECK: %1 = load volatile <3584 x float>, <3584 x float>* @All, align 16384
; CHECK: %2 = bitcast <3584 x float> %1 to <3584 x i32>
; CHECK: %.regioncollapsed6791 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v3584i32.i16(<3584 x i32> %2, i32 0, i32 1, i32 0, i16 0, i32 undef)
  store volatile <3584 x float> zeroinitializer, <3584 x float>* @All, align 16384
; CHECK: store volatile <3584 x float> zeroinitializer, <3584 x float>* @All, align 16384
; CHECK-NOT: %sev.cast.2626157.regioncollapsed.regioncollapsed =
; CHECK: %sev.cast.2626157.regioncollapsed =
  %sev.cast.2626157.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32> %2, i32 0, i32 0, i32 0, i16 0, i32 0)
  %sev.cast.189115 = bitcast i32 %sev.cast.2626157.regioncollapsed to <1 x i32>
  %3 = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32> zeroinitializer, <1 x i32> %sev.cast.189115, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %.cast12 = bitcast <16 x i32> %3 to <32 x i16>
  tail call void @llvm.genx.raw.send2.noresult.v16i1.v32i16(i8 0, i8 0, <16 x i1> zeroinitializer, i8 0, i8 0, i32 0, i32 0, <32 x i16> %.cast12)
  ret void
}
