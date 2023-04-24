;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global.int = internal global i32 0, align 4

; Function Attrs: noinline nounwind
define internal spir_func void @foo(<8 x i32>* %foo.int.vec.ref.ptr, float %foo.value, <4 x float>* %foo.flt.vec.ref.ptr, i32 signext %foo.int.val) #0 {
; CHECK: @foo(<8 x i32> %[[FOO_INT_VEC_IN:[^ ]+]], float %foo.value, <4 x float> %[[FOO_FLT_VEC_IN:[^ ]+]], i32 signext %foo.int.val, i32 %global.int.in) #0 {
  %global.int.load = load i32, i32* @global.int, align 4
  ret void
}

; Function Attrs: noinline nounwind
define dllexport void @kernel(float %kernel.flt.val, i32 %kernel.int.val) {
  %kernel.int.vec.ref.ptr = alloca <8 x i32>, align 32
  %kernel.flt.vec.ref.ptr = alloca <4 x float>, align 16

  call spir_func void @foo(<8 x i32>* nonnull byval(<8 x i32>) %kernel.int.vec.ref.ptr, float %kernel.flt.val, <4 x float>* nonnull byval(<4 x float>) %kernel.flt.vec.ref.ptr, i32 signext inreg %kernel.int.val) #1
; COM: pass changes the order of attributes, not on purpose
; CHECK: @foo(<8 x i32> %kernel.int.vec.ref.ptr.val, float %kernel.flt.val, <4 x float> %kernel.flt.vec.ref.ptr.val, i32 inreg signext %kernel.int.val, i32 %global.int.val)

  ret void
}

attributes #0 = { noinline }
; CHECK: attributes #0 = { noinline }
attributes #1 = { readonly }
; CHECK: attributes #1 = { readonly }

!genx.kernels = !{!0}
!0 = !{void (float, i32)* @kernel}
