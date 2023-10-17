;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Force -vc-ret-reg-size=100 to pass value on GRF.

; RUN: %opt %use_old_pass_manager% -GenXPrologEpilogInsertion \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -vc-ret-reg-size=100 -S < %s | FileCheck %s

; COM: Force -vc-ret-reg-size=0 to pass value on stack.

; RUN: %opt %use_old_pass_manager% -GenXPrologEpilogInsertion \
; RUN: -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -vc-ret-reg-size=0 -S < %s | FileCheck %s

; COM: In both cases the splat value must be generated due to vectorization of the scalar return value.

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_func i32 @test(i32 %arg) #0 {
entry:
; CHECK: %call_conv_vectorized_retval.splat
  ret i32 %arg
}

attributes #0 = { "CMStackCall" }
