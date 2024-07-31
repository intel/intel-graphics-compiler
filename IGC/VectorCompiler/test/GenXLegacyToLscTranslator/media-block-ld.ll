;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegacyToLscTranslator -march=genx64 -mcpu=Xe2 \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPG \
; RUN: -mattr=+translate_legacy_message -mtriple=spir64-unknown-unknown  -S < %s | \
; RUN: FileCheck --check-prefix=NOTYPED %s

; COM: media.ld -> llvm.vc.internal.lsc.load.2d.tgm.bti

declare <8 x i32> @llvm.genx.media.ld.v8i32(i32, i32, i32, i32, i32, i32)
declare <16 x i32> @llvm.genx.media.ld.v16i32(i32, i32, i32, i32, i32, i32)

define <16 x i32> @test.v16i32() {
; COM: surface 10, width 32, offset x 24 offset y 42
  %ret = tail call <16 x i32> @llvm.genx.media.ld.v16i32(i32 0, i32 10, i32 0, i32 32, i32 24, i32 42)
; COM: L1, L3, other params
; CHECK: %ret = call <16 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i32.v2i8(<2 x i8> zeroinitializer, i32 10, i32 2, i32 8, i32 24, i32 42)
; NOTYPED: %ret = tail call <16 x i32> @llvm.genx.media.ld.v16i32(i32 0, i32 10, i32 0, i32 32, i32 24, i32 42)
  ret <16 x i32> %ret
}

define <16 x i32> @test.v16i32.padding() {
; COM: surface 25, width 20, offset x 24 offset y 42
  %ret = tail call <16 x i32> @llvm.genx.media.ld.v16i32(i32 0, i32 25, i32 0, i32 20, i32 24, i32 42)
  ret <16 x i32> %ret
; COM: L1, L3, other params
; CHECK: %ret = call <16 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i32.v2i8(<2 x i8> zeroinitializer, i32 25, i32 2, i32 5, i32 24, i32 42)
; NOTYPED: %ret = tail call <16 x i32> @llvm.genx.media.ld.v16i32(i32 0, i32 25, i32 0, i32 20, i32 24, i32 42)
}

define <8 x i32> @test.v8i32.grfsize() {
; COM: surface 10, width 16, offset x 24 offset y 42
  %ret = tail call <8 x i32> @llvm.genx.media.ld.v8i32(i32 0, i32 10, i32 0, i32 16, i32 24, i32 42)
  ret <8 x i32> %ret
; COM: L1, L3, other params
; CHECK: %ret = call <8 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v8i32.v2i8(<2 x i8> zeroinitializer, i32 10, i32 2, i32 4, i32 24, i32 42)
; NOTYPED: %ret = tail call <8 x i32> @llvm.genx.media.ld.v8i32(i32 0, i32 10, i32 0, i32 16, i32 24, i32 42)
}
