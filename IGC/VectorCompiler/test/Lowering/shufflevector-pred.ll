;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <128 x i32> @llvm.genx.gather4.masked.scaled2.v128i32.v32i32.v32i1(i32, i16, i32, i32, <32 x i32>, <32 x i1>)
declare <128 x i32> @llvm.genx.wrregioni.v128i32.v4i32.i16.v4i1(<128 x i32>, <4 x i32>, i32, i32, i32, i16, i32, <4 x i1>)

; CHECK-LABEL: @test_not_splat
define <128 x i32> @test_not_splat() {

; CHECK: [[ADDR1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: [[LOAD1:%[^ ]+]] = call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v16i32.v16i1(i32 15, i16 0, i32 254, i32 0, <16 x i32> [[ADDR1]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK: [[CONST_PRED1:%[^ ]+]] = call <32 x i1> @llvm.genx.constantpred.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false>)
; CHECK: [[PRED1:%[^ ]+]] = shufflevector <32 x i1> [[CONST_PRED1]], <32 x i1> undef, <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v64i32.i16.v64i1(<64 x i32> undef, <64 x i32> [[LOAD1]], i32 0, i32 64, i32 1, i16 0, i32 undef, <64 x i1> [[PRED1]])

; CHECK: [[ADDR2:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK: [[LOAD2:%[^ ]+]] = call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v16i32.v16i1(i32 15, i16 0, i32 254, i32 0, <16 x i32> [[ADDR2]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false>)
; CHECK: [[CONST_PRED2:%[^ ]+]] = call <32 x i1> @llvm.genx.constantpred.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false>)
; CHECK: [[PRED2:%[^ ]+]] = shufflevector <32 x i1> [[CONST_PRED2]], <32 x i1> undef, <64 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23, i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v64i32.i16.v64i1(<64 x i32> undef, <64 x i32> [[LOAD2]], i32 0, i32 64, i32 1, i16 0, i32 undef, <64 x i1> [[PRED2]])

  %load = tail call <128 x i32> @llvm.genx.gather4.masked.scaled2.v128i32.v32i32.v32i1(i32 15, i16 0, i32 254, i32 0, <32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, <32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false>)
  %ret = tail call <128 x i32> @llvm.genx.wrregioni.v128i32.v4i32.i16.v4i1(<128 x i32> %load, <4 x i32> <i32 -1, i32 -1, i32 -1, i32 -1>, i32 0, i32 4, i32 32, i16 124, i32 0, <4 x i1> <i1 true, i1 true, i1 true, i1 true>)

  ret <128 x i32> %ret
}

; CHECK-LABEL: @test_splat
define <128 x i32> @test_splat() {
; CHECK: [[ADDR1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: [[LOAD1:%[^ ]+]] = call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v16i32.v16i1(i32 15, i16 0, i32 254, i32 0, <16 x i32> [[ADDR1]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v64i32.i16.i1(<64 x i32> undef, <64 x i32> [[LOAD1]], i32 0, i32 64, i32 1, i16 0, i32 undef, i1 true)

; CHECK: [[ADDR2:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK: [[LOAD2:%[^ ]+]] = call <64 x i32> @llvm.genx.gather4.masked.scaled2.v64i32.v16i32.v16i1(i32 15, i16 0, i32 254, i32 0, <16 x i32> [[ADDR2]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK: call <64 x i32> @llvm.genx.wrregioni.v64i32.v64i32.i16.i1(<64 x i32> undef, <64 x i32> [[LOAD2]], i32 0, i32 64, i32 1, i16 0, i32 undef, i1 true)

  %load = tail call <128 x i32> @llvm.genx.gather4.masked.scaled2.v128i32.v32i32.v32i1(i32 15, i16 0, i32 254, i32 0, <32 x i32> <i32 268435456, i32 268435472, i32 268435488, i32 268435504, i32 268435520, i32 268435536, i32 268435552, i32 268435568, i32 268435584, i32 268435600, i32 268435616, i32 268435632, i32 268435648, i32 268435664, i32 268435680, i32 268435696, i32 268435712, i32 268435728, i32 268435744, i32 268435760, i32 268435776, i32 268435792, i32 268435808, i32 268435824, i32 268435840, i32 268435856, i32 268435872, i32 268435888, i32 268435904, i32 268435920, i32 268435936, i32 268435952>, <32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  %ret = tail call <128 x i32> @llvm.genx.wrregioni.v128i32.v4i32.i16.v4i1(<128 x i32> %load, <4 x i32> <i32 -1, i32 -1, i32 -1, i32 -1>, i32 0, i32 4, i32 32, i16 124, i32 0, <4 x i1> <i1 true, i1 true, i1 true, i1 true>)

  ret <128 x i32> %ret
}
