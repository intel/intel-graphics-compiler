;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXFuncBaling -print-baling-info=true -march=genx64 -mtriple=spir64-unknown-unknown -disable-output -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXBaling

@AccOutSD = external dso_local global <16 x i32>
@AccOutUD = external dso_local global <16 x i32>
@ZUB = external dso_local global <16 x i16>
@ZSB = external dso_local global <16 x i16>

; CHECK: kernel_SD_UB_UB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16
define spir_kernel void @kernel_SD_UB_UB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_UB_UB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16
define spir_kernel void @kernel_SD_UB_UB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_UB_SB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16
define spir_kernel void @kernel_SD_UB_SB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_UB_SB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16
define spir_kernel void @kernel_SD_UB_SB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_SB_UB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16
define spir_kernel void @kernel_SD_SB_UB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_SB_UB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16
define spir_kernel void @kernel_SD_SB_UB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_SB_SB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16
define spir_kernel void @kernel_SD_SB_SB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_SD_SB_SB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16
define spir_kernel void @kernel_SD_SB_SB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_UB_UB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16
define spir_kernel void @kernel_UD_UB_UB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_UB_UB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16
define spir_kernel void @kernel_UD_UB_UB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_UB_SB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16
define spir_kernel void @kernel_UD_UB_SB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_UB_SB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16
define spir_kernel void @kernel_UD_UB_SB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_SB_UB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16
define spir_kernel void @kernel_UD_SB_UB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_SB_UB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16
define spir_kernel void @kernel_UD_SB_UB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_SB_SB_UB
; CHECK: %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16
define spir_kernel void @kernel_UD_SB_SB_UB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = zext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

; CHECK: kernel_UD_SB_SB_SB
; CHECK: %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16
define spir_kernel void @kernel_UD_SB_SB_SB(<16 x i16> %0) {
  %2 = load volatile <16 x i16>, <16 x i16>* null, align 32
  %3 = sext <16 x i16> %0 to <16 x i32>
  %mad = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> zeroinitializer, <16 x i16> zeroinitializer, <16 x i32> %3)
  store volatile <16 x i32> %mad, <16 x i32>* null, align 64
  ret void
}

declare <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16>, <16 x i16>, <16 x i32>)
declare <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16>, <16 x i16>, <16 x i32>)
declare <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16>, <16 x i16>, <16 x i32>)
declare <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16>, <16 x i16>, <16 x i32>)
