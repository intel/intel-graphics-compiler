;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 \
; RUN: -vc-builtins-bif-path=%VC_BIF_XeLPG% -mcpu=XeLPG \
; RUN: -mtriple=spir64-unknown-unknown -S < %s 2>&1 | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 \
; RUN: -vc-builtins-bif-path=%VC_BIF_XeHPC% -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown -S < %s 2>&1 | FileCheck %s \
; RUN:  --check-prefix=CHECK-NOEMU

; Function Attrs: nofree nosync nounwind readnone
declare <32 x float> @llvm.genx.ieee.div.v32f32(<32 x float>, <32 x float>)

define dllexport spir_kernel void @test_kernel(<32 x float> %l, <32 x float> %r) {
  ; CHECK: = fdiv <32 x float> %l, %r
  ; CHECK-NOEMU: = fdiv <32 x float> %l, %r
  %1 = fdiv <32 x float> %l, %r
  ; CHECK: = call <32 x float> @__vc_builtin_fdiv_v32f32(<32 x float> %l, <32 x float> %r)
  ; CHECK-NOEMU: = call <32 x float> @llvm.genx.ieee.div.v32f32
  %2 = call <32 x float> @llvm.genx.ieee.div.v32f32(<32 x float> %l, <32 x float> %r)
  ret void
}
