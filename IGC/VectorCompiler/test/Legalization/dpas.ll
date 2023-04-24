;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that dpas legalization will rearrange argument of dpas
; DPAS must have contiguous regioning and in the example below
; its argument consists of every second element of the register
; This test checks that GenXLegalization will transform this argument
; to contiguous form. (rdregion -> wrregion here)

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK: @Kernel

; Function Attrs: noinline nounwind
define dso_local dllexport void @Kernel() local_unnamed_addr {
entry:

; CHECK: %[[NEWVAR:[a-zA-Z0-9_.]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> %{{[a-zA-Z0-9_.]*}}, <8 x i32> %{{[a-zA-Z0-9_.]*}}, i32 0, i32 8, i32 1, i16 224, i32 undef, i1 true)
; CHECK-NEXT: %call1.i.i.i = tail call <64 x float> @llvm.genx.dpas.v64f32.v64i32.v64i32(<64 x float> undef, <64 x i32> undef, <64 x i32> %rdr.cols.regioncollapsed.split56.join56, i32 134744586)


 %rdr.cols.regioncollapsed = tail call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> undef, i32 16, i32 8, i32 1, i16 0, i32 undef)
 %call1.i.i.i = tail call <64 x float> @llvm.genx.dpas.v64f32.v64i32.v64i32(<64 x float> undef, <64 x i32> undef, <64 x i32> %rdr.cols.regioncollapsed, i32 134744586)

 ret void
}

; Function Attrs: nounwind readnone
declare <64 x float> @llvm.genx.dpas.v64f32.v64i32.v64i32(<64 x float>, <64 x i32>, <64 x i32>, i32)

; Function Attrs: nounwind readnone
declare <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32>, i32, i32, i32, i16, i32)

