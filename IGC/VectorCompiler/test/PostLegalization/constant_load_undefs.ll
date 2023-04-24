
;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeLP -mtriple=spir64 -S < %s | FileCheck %s

declare <32 x i8> @llvm.genx.media.ld.v32i8(i32, i32, i32, i32, i32, i32)
declare <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1)
declare void @llvm.genx.media.st.v64i8(i32, i32, i32, i32, i32, i32, <64 x i8>)

; CHECK-LABEL: constant_load_undefs
define dllexport spir_kernel void @constant_load_undefs(i32 %0, i32 %1) {
  %3 = tail call <32 x i8> @llvm.genx.media.ld.v32i8(i32 0, i32 %0, i32 0, i32 32, i32 0, i32 0)
  %.cast = bitcast <32 x i8> %3 to <8 x i32>
; CHECK: [[WRR:%.+]] = call <64 x i8> @llvm.genx.wrregioni.v64i8.v32i8.i16.i1(<64 x i8> <i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 17, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef, i8 undef>, <32 x i8> %3, i32 0, i32 32, i32 1, i16 32, i32 undef, i1 true)
  %4 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> <i32 286331153, i32 286331153, i32 286331153, i32 286331153, i32 286331153, i32 286331153, i32 286331153, i32 286331153, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>, <8 x i32> %.cast, i32 0, i32 8, i32 1, i16 32, i32 undef, i1 true)
  %.cast41 = bitcast <16 x i32> %4 to <64 x i8>
  tail call void @llvm.genx.media.st.v64i8(i32 0, i32 %1, i32 0, i32 32, i32 0, i32 0, <64 x i8> %.cast41)
  ret void
}
