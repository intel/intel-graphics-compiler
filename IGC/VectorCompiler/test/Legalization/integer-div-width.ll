;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: XeHPC platform allows SIMD32 operations
; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeHPC -mattr=-divrem32 -S < %s | FileCheck %s --check-prefix CHECK_MISSING

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeHPC -mattr=+divrem32 -S < %s | FileCheck %s --check-prefix CHECK_PRESENT

; CHECK_MISSING: @test_kernel
; CHECK_MISSING: = udiv <32 x i32>
; CHECK_MISSING: = sdiv <32 x i32>
; CHECK_MISSING: = urem <32 x i32>
; CHECK_MISSING: = srem <32 x i32>

; CHECK_PRESENT: @test_kernel
; CHECK_PRESENT: = udiv <16 x i32>
; CHECK_PRESENT: = udiv <16 x i32>

; CHECK_PRESENT: = sdiv <16 x i32>
; CHECK_PRESENT: = sdiv <16 x i32>

; CHECK_PRESENT: = urem <16 x i32>
; CHECK_PRESENT: = urem <16 x i32>

; CHECK_PRESENT: = srem <16 x i32>
; CHECK_PRESENT: = srem <16 x i32>

; CHECK_PRESENT: ret void

declare void @llvm.genx.oword.st.v32i32(i32, i32, <32 x i32>)
define dllexport spir_kernel void @test_kernel(<32 x i32> %l, <32 x i32> %r) {
  %udiv = udiv <32 x i32> %l, %r
  %sdiv = sdiv <32 x i32> %l, %r
  %urem = urem <32 x i32> %l, %r
  %srem = srem <32 x i32> %l, %r
  %s1 = add <32 x i32> %udiv, %sdiv
  %s2 = add <32 x i32> %s1, %udiv
  %s3 = add <32 x i32> %s2, %urem
  %s4 = add <32 x i32> %s3, %srem
  call void @llvm.genx.oword.st.v32i32(i32 0, i32 0, <32 x i32> %s4)
  ret void
}
