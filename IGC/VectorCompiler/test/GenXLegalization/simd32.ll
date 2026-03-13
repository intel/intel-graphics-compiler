;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,Xe3P
; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3PLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s --check-prefixes=CHECK,Xe3PLPG



declare <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64>, i32, i32, i32, i16, i32)
declare <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64>, <32 x i64>, i32, i32, i32, i16, i32, i1)

define <64 x i64> @test1(<64 x i64> %src) {
  ; Xe3P-LABEL: test1
  ; Xe3P-NEXT: %[[IN_SPLIT0:.+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %src, i32 16, i32 16, i32 1, i16 0, i32 undef)
  ; Xe3P-NEXT: %[[ADD_SPLIT0:.+]] = add <16 x i64> %[[IN_SPLIT0]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3P-NEXT: %[[RES_JOIN0:.+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> %src, <16 x i64> %[[ADD_SPLIT0]], i32 16, i32 16, i32 1, i16 256, i32 undef, i1 true)
  ; Xe3P-NEXT: %[[IN_SPLIT16:.+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v64i64.i16(<64 x i64> %[[RES_JOIN0]], i32 16, i32 16, i32 1, i16 128, i32 undef)
  ; Xe3P-NEXT: %[[ADD_SPLIT16:.+]] = add <16 x i64> %[[IN_SPLIT16]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3P-NEXT: %[[RES_JOIN16:.+]] = call <64 x i64> @llvm.genx.wrregioni.v64i64.v16i64.i16.i1(<64 x i64> %[[RES_JOIN0]], <16 x i64> %[[ADD_SPLIT16]], i32 16, i32 16, i32 1, i16 384, i32 undef, i1 true)
  ; Xe3P-NEXT: ret <64 x i64> %[[RES_JOIN16]]

  ; Xe3PLPG-LABEL: test1
  ; Xe3PLPG-NEXT: %[[IN:.+]] = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %src, i32 0, i32 32, i32 1, i16 0, i32 undef)
  ; Xe3PLPG-NEXT: %[[ADD:.+]] = add <32 x i64> %[[IN]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; Xe3PLPG-NEXT: %[[RES:.+]] = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64> %src, <32 x i64> %[[ADD]], i32 0, i32 32, i32 1, i16 256, i32 undef, i1 true)
  ; Xe3PLPG-NEXT: ret <64 x i64> %[[RES]]

  %in = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v64i64.i16(<64 x i64> %src, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %add = add <32 x i64> %in, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %res = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v32i64.i16.i1(<64 x i64> %src, <32 x i64> %add, i32 0, i32 32, i32 1, i16 256, i32 undef, i1 true)
  ret <64 x i64> %res
}

declare <32 x i64> @llvm.genx.rdregioni.v32i64.v72i64.i16(<72 x i64>, i32, i32, i32, i16, i32)
declare <72 x i64> @llvm.genx.wrregioni.v72i64.v32i64.i16.i1(<72 x i64>, <32 x i64>, i32, i32, i32, i16, i32, i1)

define <72 x i64> @test2(<72 x i64> %src) {
  ; CHECK-LABEL: test2
  ; CHECK-NEXT: %[[IN0:.+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v72i64.i16(<72 x i64> %src, i32 8, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK-NEXT: %[[ADD0:.+]] = add <8 x i64> %[[IN0]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; CHECK-NEXT: %[[RES0:.+]] = call <72 x i64> @llvm.genx.wrregioni.v72i64.v8i64.i16.i1(<72 x i64> %src, <8 x i64> %[[ADD0]], i32 8, i32 8, i32 1, i16 288, i32 undef, i1 true)
  ; CHECK-NEXT: %[[IN8:.+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v72i64.i16(<72 x i64> %[[RES0]], i32 8, i32 8, i32 1, i16 64, i32 undef)
  ; CHECK-NEXT: %[[ADD8:.+]] = add <8 x i64> %[[IN8]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; CHECK-NEXT: %[[RES8:.+]] = call <72 x i64> @llvm.genx.wrregioni.v72i64.v8i64.i16.i1(<72 x i64> %[[RES0]], <8 x i64> %[[ADD8]], i32 8, i32 8, i32 1, i16 352, i32 undef, i1 true)
  ; CHECK-NEXT: %[[IN16:.+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v72i64.i16(<72 x i64> %[[RES8]], i32 8, i32 8, i32 1, i16 128, i32 undef)
  ; CHECK-NEXT: %[[ADD16:.+]] = add <8 x i64> %[[IN16]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; CHECK-NEXT: %[[RES16:.+]] = call <72 x i64> @llvm.genx.wrregioni.v72i64.v8i64.i16.i1(<72 x i64> %[[RES8]], <8 x i64> %[[ADD16]], i32 8, i32 8, i32 1, i16 416, i32 undef, i1 true)
  ; CHECK-NEXT: %[[IN24:.+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v72i64.i16(<72 x i64> %[[RES16]], i32 8, i32 8, i32 1, i16 192, i32 undef)
  ; CHECK-NEXT: %[[ADD24:.+]] = add <8 x i64> %[[IN24]], <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  ; CHECK-NEXT: %[[RES:.+]] = call <72 x i64> @llvm.genx.wrregioni.v72i64.v8i64.i16.i1(<72 x i64> %[[RES16]], <8 x i64> %[[ADD24]], i32 8, i32 8, i32 1, i16 480, i32 undef, i1 true)
  ; CHECK-NEXT: ret <72 x i64> %[[RES]]

  %in = tail call <32 x i64> @llvm.genx.rdregioni.v32i64.v72i64.i16(<72 x i64> %src, i32 0, i32 32, i32 1, i16 0, i32 undef)
  %add = add <32 x i64> %in, <i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1>
  %res = tail call <72 x i64> @llvm.genx.wrregioni.v72i64.v32i64.i16.i1(<72 x i64> %src, <32 x i64> %add, i32 0, i32 32, i32 1, i16 288, i32 undef, i1 true)
  ret <72 x i64> %res
}
