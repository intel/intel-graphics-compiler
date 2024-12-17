;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Xe3 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
target triple = "genx64-unknown-unknown"

declare <64 x i8> @llvm.genx.hf8.cvt.v64i8.v64f16(<64 x half>)
declare <64 x half> @llvm.genx.hf8.cvt.v64f16.v64i8(<64 x i8>)

define <64 x half> @test_in(<64 x i8> %src) {
  ; CHECK-DAG: %[[SPLIT0:.+]] = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %src, i32 32, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK-DAG: %[[RES0:.+]] = call <32 x half> @llvm.genx.hf8.cvt.v32f16.v32i8(<32 x i8> %[[SPLIT0]])
  ; CHECK-DAG: %[[SPLIT32:.+]] = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %src, i32 32, i32 32, i32 1, i16 32, i32 undef)
  ; CHECK-DAG: %[[RES32:.+]] = call <32 x half> @llvm.genx.hf8.cvt.v32f16.v32i8(<32 x i8> %[[SPLIT32]])
  %res = call <64 x half> @llvm.genx.hf8.cvt.v64f16.v64i8(<64 x i8> %src)
  ret <64 x half> %res
}

define <64 x i8> @test_out(<64 x half> %src) {
  ; CHECK-DAG: %[[XSPLIT0:.+]] = call <32 x half> @llvm.genx.rdregionf.v32f16.v64f16.i16(<64 x half> %src, i32 32, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK-DAG: %[[XRES0:.+]] = call <32 x i8> @llvm.genx.hf8.cvt.v32i8.v32f16(<32 x half> %[[XSPLIT0]])
  ; CHECK-DAG: %[[XSPLIT32:.+]] = call <32 x half> @llvm.genx.rdregionf.v32f16.v64f16.i16(<64 x half> %src, i32 32, i32 32, i32 1, i16 64, i32 undef)
  ; CHECK-DAG: %[[XRES32:.+]] = call <32 x i8> @llvm.genx.hf8.cvt.v32i8.v32f16(<32 x half> %[[XSPLIT32]])
  %res = call <64 x i8> @llvm.genx.hf8.cvt.v64i8.v64f16(<64 x half> %src)
  ret <64 x i8> %res
}
