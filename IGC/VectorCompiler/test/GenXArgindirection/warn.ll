;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; The only purpose of this test is to test warning when arg indirection can not happen
; It is convoluted enough for this purpose

; RUN: opt %use_old_pass_manager% -load /users/kvladimi/gfx/gfx-driver/dump64/igc/libVCBackendPlugin.so -GenXModule -GenXLiveRangesWrapper -GenXArgIndirectionWrapper -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s >& %t
; RUN: FileCheck %s < %t

; CHECK: warning: GenXArgIndirection failed for: < Argument 1 in foo>: Use of argument cannot be indirected

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32>, i32, i32, i32, i16, i32)

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32)

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32>, i32, i32, i32, i16, i32)

declare <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

declare <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @i() local_unnamed_addr #0 {
  %call = tail call spir_func <128 x i32> @buz(<128 x i32> undef)
  ret void
}

define internal spir_func <128 x i32> @buz(<128 x i32> %arg) unnamed_addr {
  %rdr.split0 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %arg, i32 16, i32 16, i32 1, i16 0, i32 undef)
  %rdr.split0.join0 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> undef, <16 x i32> %rdr.split0, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %rdr.split16 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %arg, i32 16, i32 16, i32 1, i16 64, i32 undef)
  %rdr.split16.join16 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %rdr.split0.join0, <16 x i32> %rdr.split16, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %rdr.split32 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %arg, i32 16, i32 16, i32 1, i16 128, i32 undef)
  %rdr.split32.join32 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %rdr.split16.join16, <16 x i32> %rdr.split32, i32 0, i32 16, i32 1, i16 128, i32 undef, i1 true)
  %rdr.split48 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> %arg, i32 16, i32 16, i32 1, i16 192, i32 undef)
  %rdr.split48.join48 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %rdr.split32.join32, <16 x i32> %rdr.split48, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true)
  %call = tail call spir_func <64 x i32> @bar(<64 x i32> %rdr.split48.join48)
  ret <128 x i32> undef
}

define internal spir_func <64 x i32> @bar(<64 x i32> %arg) unnamed_addr {
  %rdr.split0 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 0, i32 undef)
  %rdr.split0.join0 = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> %rdr.split0, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %rdr.split16 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %arg, i32 16, i32 16, i32 1, i16 64, i32 undef)
  %rdr.split16.join16 = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> %rdr.split0.join0, <16 x i32> %rdr.split16, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %call = tail call spir_func <32 x i32> @foo(<32 x i32> %rdr.split16.join16)
  ret <64 x i32> undef
}

define internal spir_func <32 x i32> @foo(<32 x i32> %arg) unnamed_addr {
  %rdr = tail call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %arg, i32 0, i32 16, i32 1, i16 0, i32 undef)
  ret <32 x i32> undef
}

attributes #0 = { noinline nounwind "CMGenxMain" "RequiresImplArgsBuffer" "VC.Stack.Amount"="80" "oclrt"="1" }
