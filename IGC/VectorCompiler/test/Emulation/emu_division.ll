;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -S < %s | FileCheck %s

; CHECK: @test_kernel
; CHECK-NEXT: = udiv <2 x i32>
; CHECK-NEXT: = sdiv <2 x i32>
; CHECK-NEXT: = urem <2 x i32>
; CHECK-NEXT: = srem <2 x i32>
; CHECK-NEXT: = sext <2 x i32> %l to <2 x i64>
; CHECK-NEXT: = sext <2 x i32> %r to <2 x i64>
; CHECK-NEXT: = call <2 x i64> @__cm_intrinsic_impl_udiv
; CHECK-NEXT: = call <2 x i64> @__cm_intrinsic_impl_sdiv
; CHECK-NEXT: = call <2 x i64> @__cm_intrinsic_impl_urem
; CHECK-NEXT: = call <2 x i64> @__cm_intrinsic_impl_srem
; CHECK-NEXT: ret void

define dllexport spir_kernel void @test_kernel(<2 x i32> %l, <2 x i32> %r) {
  %udiv = udiv <2 x i32> %l, %r
  %sdiv = sdiv <2 x i32> %l, %r
  %urem = urem <2 x i32> %l, %r
  %srem = srem <2 x i32> %l, %r

  %l64 = sext <2 x i32> %l to <2 x i64>
  %r64 = sext <2 x i32> %r to <2 x i64>

  %udiv64 = udiv <2 x i64> %l64, %r64
  %sdiv64 = sdiv <2 x i64> %l64, %r64
  %urem64 = urem <2 x i64> %l64, %r64
  %srem64 = srem <2 x i64> %l64, %r64

  ret void
}

; COM: The presence of these __cm_intrinsic_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have emulation routines
define <2 x i64> @__cm_intrinsic_impl_sdiv(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__cm_intrinsic_impl_srem(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__cm_intrinsic_impl_udiv(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__cm_intrinsic_impl_urem(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}

attributes #0 = { "VC.Emulation.Routine" }
