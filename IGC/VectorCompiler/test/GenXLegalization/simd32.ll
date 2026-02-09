;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=Xe3P
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefix=Xe3PLPG



declare <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64>, i32, i32, i32, i16, i32)
declare <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64>, <32 x i64>, i32, i32, i32, i16, i32, i1)

define <64 x i64> @test(<64 x i64> %src) {
  ; Xe3P-LABEL: test
  ; Xe3P-NEXT: %[[IN_SPLIT0:.+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %src, i32 16, i32 16, i32 1, i16 0, i32 undef)
  ; Xe3P-NEXT: %[[ADD_SPLIT0:.+]] = add <16 x i64> %[[IN_SPLIT0]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3P-NEXT: %[[RES_JOIN0:.+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> %src, <16 x i64> %[[ADD_SPLIT0]], i32 16, i32 16, i32 1, i16 256, i32 undef, i1 true)
  ; Xe3P-NEXT: %[[IN_SPLIT16:.+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %[[RES_JOIN0]], i32 16, i32 16, i32 1, i16 128, i32 undef)
  ; Xe3P-NEXT: %[[ADD_SPLIT16:.+]] = add <16 x i64> %[[IN_SPLIT16]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3P-NEXT: %[[RES_JOIN16:.+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> %[[RES_JOIN0]], <16 x i64> %[[ADD_SPLIT16]], i32 16, i32 16, i32 1, i16 384, i32 undef, i1 true)
  ; Xe3P-NEXT: ret <64 x i64> %[[RES_JOIN16]]

  ; Xe3PLPG-LABEL: test
  ; Xe3PLPG-NEXT: %[[IN:.+]] = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %src, i32 0, i32 32, i32 1, i16 0, i32 undef)
  ; Xe3PLPG-NEXT: %[[ADD:.+]] = add <32 x i64> %[[IN]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3PLPG-NEXT: %[[RES:.+]] = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64> %src, <32 x i64> %[[ADD]], i32 0, i32 32, i32 1, i16 256, i32 undef, i1 true)
  ; Xe3PLPG-NEXT: ret <64 x i64> %[[RES]]

  %in = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %src, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %add = add <32 x i64> %in, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %res = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64> %src, <32 x i64> %add, i32 0, i32 32, i32 1, i16 256, i32 undef, i1 true)
  ret <64 x i64> %res
}
