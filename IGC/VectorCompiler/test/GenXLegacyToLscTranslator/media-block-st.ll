;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegacyToLscTranslator -march=genx64 -mcpu=Xe2 \
; RUN: -mattr=+translate_legacy_message -mtriple=spir64-unknown-unknown  -S < %s | \
; RUN: FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPG \
; RUN: -mattr=+translate_legacy_message -mtriple=spir64-unknown-unknown  -S < %s | \
; RUN: FileCheck --check-prefix=NOTYPED %s

; COM: media.st -> llvm.genx.lsc.store2d.typed.bti

declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>)
declare void @llvm.genx.media.st.v16i32(i32, i32, i32, i32, i32, i32, <16 x i32>)

define void @test.v16i32(i32 %off, <16 x i32> %arg) {
  call void @llvm.genx.media.st.v16i32(i32 0, i32 3, i32 0, i32 32, i32 24, i32 42, <16 x i32> %arg)
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i32(<2 x i8> zeroinitializer, i32 3, i32 2, i32 8, i32 24, i32 42, <16 x i32> %arg)
; NOTYPED: call void @llvm.genx.media.st.v16i32(i32 0, i32 3, i32 0, i32 32, i32 24, i32 42, <16 x i32> %arg)
  ret void
}

define void @test.v16i32.padding(i32 %off, <16 x i32> %arg) {
  call void @llvm.genx.media.st.v16i32(i32 0, i32 3, i32 0, i32 20, i32 24, i32 42, <16 x i32> %arg)
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i32(<2 x i8> zeroinitializer, i32 3, i32 2, i32 5, i32 24, i32 42, <16 x i32> %arg)
; NOTYPED: call void @llvm.genx.media.st.v16i32(i32 0, i32 3, i32 0, i32 20, i32 24, i32 42, <16 x i32> %arg)
  ret void
}

define void @test.v8i32.grfsize(i32 %off, <8 x i32> %arg) {
  call void @llvm.genx.media.st.v8i32(i32 0, i32 3, i32 0, i32 16, i32 24, i32 42, <8 x i32> %arg)
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v8i32(<2 x i8> zeroinitializer, i32 3, i32 2, i32 4, i32 24, i32 42, <8 x i32> %arg)
; NOTYPED: call void @llvm.genx.media.st.v8i32(i32 0, i32 3, i32 0, i32 16, i32 24, i32 42, <8 x i32> %arg)
  ret void
}
