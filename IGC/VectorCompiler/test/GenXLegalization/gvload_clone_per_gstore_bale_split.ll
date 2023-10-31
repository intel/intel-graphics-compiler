;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
;
; ------------------------------------------------
; GenXLegalization
; ------------------------------------------------

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@g_global1 = external global <64 x i16> #0
@g_global2 = external global <64 x i16> #0

define internal spir_func void @test_gvload_clone_per_split1() {
  %gvload1 = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
  %add = add <64 x i16> %gvload1, zeroinitializer
; CHECK: %add.gvload_use_split_clone = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; CHECK: %add.gvload_use_split_clone.split32 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %add.gvload_use_split_clone, i32 32, i32 32, i32 1, i16 64, i32 undef)
; CHECK: %add.split32 = add <32 x i16> %add.gvload_use_split_clone.split32, zeroinitializer
  store volatile <64 x i16> %add, <64 x i16>* @g_global1, align 128
  ret void
}

define internal spir_func void @test_gvload_clone_per_split2() {
  %gvload1 = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
  %gvload2 = load volatile <64 x i16>, <64 x i16>* @g_global2, align 128
  %add = add <64 x i16> %gvload1, %gvload2
; CHECK: %add.gvload_use_split_clone = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; CHECK-NEXT: %add.gvload_use_split_clone.split32 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %add.gvload_use_split_clone, i32 32, i32 32, i32 1, i16 64, i32 undef)
; CHECK-NEXT: %gvload2.split32 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %gvload2, i32 32, i32 32, i32 1, i16 64, i32 undef)
; CHECK-NEXT: %add.split32 = add <32 x i16> %add.gvload_use_split_clone.split32, %gvload2.split32
  store volatile <64 x i16> %add, <64 x i16>* @g_global1, align 128
  ret void
}

; COM: define internal spir_func void @test_gvload_clone_per_split2() #1 {
; COM:   %gvload1 = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; COM:   %gvload2 = load volatile <64 x i16>, <64 x i16>* @g_global2, align 128
; COM:   %.gload = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; COM:   %gvload1.split0 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %gvload1, i32 32, i32 32, i32 1, i16 0, i32 undef)
; COM:   %gvload2.split0 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %gvload2, i32 32, i32 32, i32 1, i16 0, i32 undef)
; COM:   %add.split0 = add <32 x i16> %gvload1.split0, %gvload2.split0
; COM:   %.wrr.gstore.join0 = call <64 x i16> @llvm.genx.wrregioni.v64i16.v32i16.i16.i1(<64 x i16> %.gload, <32 x i16> %add.split0, i32 32, i32 32, i32 1, i16 0, i32 undef, i1 true)
; COM:   store volatile <64 x i16> %.wrr.gstore.join0, <64 x i16>* @g_global1, align 128
; COM:   %add.gvload_use_split_clone = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; COM:   %add.gvload_use_split_clone.split32 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %add.gvload_use_split_clone, i32 32, i32 32, i32 1, i16 64, i32 undef)
; COM:   %gvload2.split32 = call <32 x i16> @llvm.genx.rdregioni.v32i16.v64i16.i16(<64 x i16> %gvload2, i32 32, i32 32, i32 1, i16 64, i32 undef)
; COM:   %add.split32 = add <32 x i16> %add.gvload_use_split_clone.split32, %gvload2.split32
; COM:   %.gload1 = load volatile <64 x i16>, <64 x i16>* @g_global1, align 128
; COM:   %.wrr.gstore.join32 = call <64 x i16> @llvm.genx.wrregioni.v64i16.v32i16.i16.i1(<64 x i16> %.gload1, <32 x i16> %add.split32, i32 32, i32 32, i32 1, i16 64, i32 undef, i1 true)
; COM:   store volatile <64 x i16> %.wrr.gstore.join32, <64 x i16>* @g_global1, align 128
; COM:   ret void
; COM: }

attributes #0 = { "genx_volatile" }
