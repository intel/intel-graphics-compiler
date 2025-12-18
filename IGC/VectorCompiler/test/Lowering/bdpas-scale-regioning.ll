;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm_12_or_greater
; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float>, <128 x i32>, <64 x i32>, <32 x i8>, <16 x i8>, i32, i32, i32, i32)

declare <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32)
declare <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8>, i32, i32, i32, i16, i32)

; CHECK-LABEL: @test1(
define <128 x float> @test1(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <64 x i8> %bscale, <64 x i8> %ascale) {
  %bscale.0 = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %bscale, i32 32, i32 16, i32 1, i16 0, i32 0)
  %bscale.16 = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %bscale, i32 32, i32 16, i32 1, i16 16, i32 0)

  %ascale.0 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %ascale, i32 32, i32 8, i32 1, i16 0, i32 0)
  %ascale.8 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %ascale, i32 32, i32 8, i32 1, i16 8, i32 0)
  %ascale.16 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %ascale, i32 32, i32 8, i32 1, i16 16, i32 0)
  %ascale.24 = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %ascale, i32 32, i32 8, i32 1, i16 24, i32 0)

; CHECK: [[BSCALE0:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ASCALE0:%.*]] = call <40 x i8> @llvm.genx.rdregioni.v40i8.v64i8.i16(<64 x i8> %ascale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ACC1:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE0]], <40 x i8> [[ASCALE0]], i32 15, i32 15, i32 8, i32 8)
  %1 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <32 x i8> %bscale.0, <16 x i8> %ascale.0, i32 15, i32 15, i32 8, i32 8)

; CHECK: [[BSCALE16:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 16, i32 undef)
; CHECK: [[ASCALE8:%.*]] = call <40 x i8> @llvm.genx.rdregioni.v40i8.v64i8.i16(<64 x i8> %ascale, i32 1, i32 1, i32 0, i16 8, i32 undef)
; CHECK: [[ACC2:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> [[ACC1]], <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE16]], <40 x i8> [[ASCALE8]], i32 15, i32 15, i32 8, i32 8)
  %2 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %1, <128 x i32> %b, <64 x i32> %a, <32 x i8> %bscale.16, <16 x i8> %ascale.8, i32 15, i32 15, i32 8, i32 8)

; CHECK: [[BSCALE0X:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ASCALE16:%.*]] = call <40 x i8> @llvm.genx.rdregioni.v40i8.v64i8.i16(<64 x i8> %ascale, i32 1, i32 1, i32 0, i16 16, i32 undef)
; CHECK: [[ACC3:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> [[ACC2]], <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE0X]], <40 x i8> [[ASCALE16]], i32 15, i32 15, i32 8, i32 8)
  %3 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %2, <128 x i32> %b, <64 x i32> %a, <32 x i8> %bscale.0, <16 x i8> %ascale.16, i32 15, i32 15, i32 8, i32 8)

; CHECK: [[BSCALE16X:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 16, i32 undef)
; CHECK: [[ASCALE24:%.*]] = call <40 x i8> @llvm.genx.rdregioni.v40i8.v64i8.i16(<64 x i8> %ascale, i32 1, i32 1, i32 0, i16 24, i32 undef)
; CHECK: [[ACC4:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> [[ACC3]], <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE16X]], <40 x i8> [[ASCALE24]], i32 15, i32 15, i32 8, i32 8)
  %4 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %3, <128 x i32> %b, <64 x i32> %a, <32 x i8> %bscale.16, <16 x i8> %ascale.24, i32 15, i32 15, i32 8, i32 8)

; CHECK: ret <128 x float> [[ACC4]]
  ret <128 x float> %4
}

; CHECK-LABEL: @test2(
define <128 x float> @test2(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <64 x i8> %bscale, <64 x i8> %ascale) {
  %xbscale = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %bscale, i32 2, i32 1, i32 0, i16 0, i32 0)
  %xascale = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> %ascale, i32 32, i32 8, i32 1, i16 1, i32 0)

; CHECK: [[BINS:%.*]] = call <48 x i8> @llvm.genx.wrregioni.v48i8.v32i8.i16.i1(<48 x i8> undef, <32 x i8> %xbscale, i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[AINS:%.*]] = call <40 x i8> @llvm.genx.wrregioni.v40i8.v16i8.i16.i1(<40 x i8> undef, <16 x i8> %xascale, i32 32, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[ACC:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BINS]], <40 x i8> [[AINS]], i32 15, i32 15, i32 8, i32 8)
; CHECK: ret <128 x float> [[ACC]]
  %res = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <32 x i8> %xbscale, <16 x i8> %xascale, i32 15, i32 15, i32 8, i32 8)
  ret <128 x float> %res
}

; CHECK-LABEL: @test3(
define <128 x float> @test3(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <64 x i8> %bscale) {
  %xbscale = call <32 x i8> @llvm.genx.rdregioni.v32i8.v64i8.i16(<64 x i8> %bscale, i32 32, i32 16, i32 1, i16 0, i32 0)

; CHECK: [[BSCALE1:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ACC1:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v16i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE1]], <16 x i8> undef, i32 15, i32 15, i32 8, i32 8)
  %1 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %acc, <128 x i32> %b, <64 x i32> %a, <32 x i8> %xbscale, <16 x i8> undef, i32 15, i32 15, i32 8, i32 8)

; CHECK: [[BSCALE2:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ACC2:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v16i8(<128 x float> [[ACC1]], <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE2]], <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 15, i32 15, i32 8, i32 8)
  %2 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %1, <128 x i32> %b, <64 x i32> %a, <32 x i8> %xbscale, <16 x i8> <i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127, i8 127>, i32 15, i32 15, i32 8, i32 8)

; CHECK: [[BSCALE3:%.*]] = call <48 x i8> @llvm.genx.rdregioni.v48i8.v64i8.i16(<64 x i8> %bscale, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: [[ASCALE3:%.*]] = call <40 x i8> @llvm.genx.wrregioni.v40i8.v16i8.i16.i1(<40 x i8> undef, <16 x i8> zeroinitializer, i32 32, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[ACC3:%.*]] = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v48i8.v40i8(<128 x float> [[ACC2]], <128 x i32> %b, <64 x i32> %a, <48 x i8> [[BSCALE3]], <40 x i8> [[ASCALE3]], i32 15, i32 15, i32 8, i32 8)
  %3 = call <128 x float> @llvm.genx.bdpas.v128f32.v128f32.v128i32.v64i32.v32i8.v16i8(<128 x float> %2, <128 x i32> %b, <64 x i32> %a, <32 x i8> %xbscale, <16 x i8> zeroinitializer, i32 15, i32 15, i32 8, i32 8)

; CHECK: ret <128 x float> [[ACC3]]
  ret <128 x float> %3
}
