;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPostLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64 -S < %s | FileCheck %s

declare <8 x i64> @llvm.genx.rdregioni.v8i64.v64i64.i16(<64 x i64>, i32, i32, i32, i16, i32)
declare <64 x i64> @llvm.genx.wrregioni.v64i64.v8i64.i16.i1(<64 x i64>, <8 x i64>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: USG_TILT
define dllexport spir_kernel void @USG_TILT(<64 x i64> %input) {
  %1 = tail call <8 x i64> @llvm.genx.rdregioni.v8i64.v64i64.i16(<64 x i64> %input, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %2 = tail call <8 x i64> @llvm.genx.rdregioni.v8i64.v64i64.i16(<64 x i64> %input, i32 0, i32 8, i32 1, i16 0, i32 undef)
  %3 = tail call <8 x i64> @llvm.genx.rdregioni.v8i64.v64i64.i16(<64 x i64> %input, i32 0, i32 8, i32 1, i16 64, i32 undef)
  %4 = tail call <8 x i64> @llvm.genx.rdregioni.v8i64.v64i64.i16(<64 x i64> %input, i32 0, i32 8, i32 1, i16 64, i32 undef)
  %5 = lshr <8 x i64> %3, <i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8>
  %.cast = bitcast <8 x i64> %5 to <16 x i32>
; CHECK: [[CAST1:%.*]] = bitcast <64 x i64> %input to <128 x i32>
; CHECK-NEXT: call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> [[CAST1]], i32 0, i32 16, i32 1, i16 0, i32 undef)
  %.cast75 = bitcast <8 x i64> %1 to <16 x i32>
  %int_emu. = xor <16 x i32> %.cast, %.cast75
  %.cast76 = bitcast <16 x i32> %int_emu. to <8 x i64>
  %.cast76.cast = bitcast <8 x i64> %.cast76 to <16 x i32>
  %int_emu.77 = and <16 x i32> %.cast76.cast, <i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935, i32 16711935>
  %.cast78 = bitcast <16 x i32> %int_emu.77 to <8 x i64>
  %.cast78.cast = bitcast <8 x i64> %.cast78 to <16 x i32>
; CHECK: [[CAST2:%.*]] = bitcast <64 x i64> %input to <128 x i32>
; CHECK-NEXT: call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> [[CAST2]], i32 0, i32 16, i32 1, i16 0, i32 undef)
  %.cast79 = bitcast <8 x i64> %2 to <16 x i32>
  %int_emu.80 = xor <16 x i32> %.cast78.cast, %.cast79
  %.cast81 = bitcast <16 x i32> %int_emu.80 to <8 x i64>
  %6 = shl nuw <8 x i64> %.cast78, <i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8, i64 8>
  %.cast82 = bitcast <8 x i64> %6 to <16 x i32>
; CHECK: [[CAST3:%.*]] = bitcast <64 x i64> %input to <128 x i32>
; CHECK-NEXT: call <16 x i32> @llvm.genx.rdregioni.v16i32.v128i32.i16(<128 x i32> [[CAST3]], i32 0, i32 16, i32 1, i16 64, i32 undef)
  %.cast83 = bitcast <8 x i64> %4 to <16 x i32>
  %int_emu.84 = xor <16 x i32> %.cast82, %.cast83
  %.cast85 = bitcast <16 x i32> %int_emu.84 to <8 x i64>
; CHECK: %input.cast = bitcast <64 x i64> %input to <128 x i32>
; CHECK-NEXT: [[WRR:%.*]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v16i32.i16.i1(<128 x i32> %input.cast, <16 x i32> %int_emu.84, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK-NEXT: bitcast <128 x i32> [[WRR]] to <64 x i64>
  %7 = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v8i64.i16.i1(<64 x i64> %input, <8 x i64> %.cast85, i32 0, i32 8, i32 1, i16 64, i32 undef, i1 true)
  %8 = tail call <64 x i64> @llvm.genx.wrregioni.v64i64.v8i64.i16.i1(<64 x i64> %7, <8 x i64> %.cast81, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ret void
}

declare <8 x float> @llvm.genx.rdregionf.v8f32.v64f32.i16(<64 x float>, i32, i32, i32, i16, i32)
declare <64 x float> @llvm.genx.wrregionf.v64f32.v8f32.i16.i1(<64 x float>, <8 x float>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: fp_test
define <64 x float> @fp_test(<64 x float> %input) {
  %1 = tail call <8 x float> @llvm.genx.rdregionf.v8f32.v64f32.i16(<64 x float> %input, i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK: [[CAST11:%.*]] = bitcast <64 x float> %input to <64 x i32>
; CHECK-NEXT: call <8 x i32> @llvm.genx.rdregioni.v8i32.v64i32.i16(<64 x i32> [[CAST11]], i32 0, i32 8, i32 1, i16 0, i32 undef)
  %cast1 = bitcast <8 x float> %1 to <8 x i32>
  %2 = shl nuw <8 x i32> %cast1, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %cast2 = bitcast <8 x i32> %2 to <8 x float>
; CHECK: %input.cast = bitcast <64 x float> %input to <64 x i32>
; CHECK-NEXT: call <64 x i32> @llvm.genx.wrregioni.v64i32.v8i32.i16.i1(<64 x i32> %input.cast, <8 x i32> %3, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %3 = tail call <64 x float> @llvm.genx.wrregionf.v64f32.v8f32.i16.i1(<64 x float> %input, <8 x float> %cast2, i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ret <64 x float> %3
}

declare <2048 x i8> @llvm.genx.read.predef.reg.v2048i8.v2048i8(i32, <2048 x i8>)
declare <2048 x i8> @llvm.genx.wrregioni.v2048i8.v208i8.i16.i1(<2048 x i8>, <208 x i8>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: @ARG_reg_test
define <2048 x i8> @ARG_reg_test(<52 x i32> %input) {
  %cast = bitcast <52 x i32> %input to <208 x i8>
; CHECK: call <2048 x i8> @llvm.genx.read.predef.reg.v2048i8.v2048i8(i32 8, <2048 x i8> undef)
; CHECK-NOT: bitcast <2048 x i8> %arg to <512 x i32>
; CHECK-NEXT: call <2048 x i8> @llvm.genx.wrregioni.v2048i8.v208i8.i16.i1(<2048 x i8> %arg, <208 x i8> %cast, i32 0, i32 208, i32 1, i16 0, i32 undef, i1 true)
  %arg = call <2048 x i8> @llvm.genx.read.predef.reg.v2048i8.v2048i8(i32 8, <2048 x i8> undef)
  %wrr = call <2048 x i8> @llvm.genx.wrregioni.v2048i8.v208i8.i16.i1(<2048 x i8> %arg, <208 x i8> %cast, i32 0, i32 208, i32 1, i16 0, i32 undef, i1 true)
  ret <2048 x i8> %wrr
}

declare <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.genx.media.st.v192i8(i32, i32, i32, i32, i32, i32, <192 x i8>)

; CHECK-LABEL: @scalar_bitcast
define void @scalar_bitcast(<2 x i32> %in, <192 x i8> %val, i32 %media.st.arg1, i32 %media.st.arg2) {
; CHECK: %rdres = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %in, i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK-NEXT: %castres = bitcast <1 x i32> %rdres to i32
; CHECK-NEXT: call void @llvm.genx.media.st.v192i8(i32 0, i32 %castres, i32 0, i32 24, i32 %media.st.arg1, i32 %media.st.arg2, <192 x i8> %val)
  %rdres = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %in, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %castres = bitcast <1 x i32> %rdres to i32
  call void @llvm.genx.media.st.v192i8(i32 0, i32 %castres, i32 0, i32 24, i32 %media.st.arg1, i32 %media.st.arg2, <192 x i8> %val)
  ret void
}
