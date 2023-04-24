;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;RUN: opt %use_old_pass_manager% -GenXReduceIntSize -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

@b = internal global <4 x i8> undef, align 4

define dllexport spir_kernel void @test_kernel1()  {
  ;COM:    ===--vector operands test--===
  ;CHECK:  xor <4 x i32> %conv, <i32 -69136, i32 -69136, i32 -69136, i32 -69136>
  ;CHECK:  xor <4 x i16> %conv.reduceintsize, <i16 1, i16 1, i16 1, i16 1>
  ;CHECK:  xor <4 x i32> %conv, <i32 131071, i32 131071, i32 131071, i32 131071>

  %g = call <4 x i8> @llvm.genx.vload.v4i8.p0v4i8(<4 x i8>* nonnull @b)
  %conv = sext <4 x i8> %g to <4 x i32>
  xor <4 x i32> %conv, <i32 -69136, i32 -69136, i32 -69136, i32 -69136>
  xor <4 x i32> %conv, <i32 1, i32 1, i32 1, i32 1>
  xor <4 x i32> %conv, <i32 131071, i32 131071, i32 131071, i32 131071>

  ret void
}

declare <4 x i8> @llvm.genx.vload.v4i8.p0v4i8(<4 x i8>*)
