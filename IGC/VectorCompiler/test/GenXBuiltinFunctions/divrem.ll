;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -S < %s 2>&1 | FileCheck %s

; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; CHECK: @test_kernel
; CHECK: = udiv <2 x i32>
; CHECK: = sdiv <2 x i32>
; CHECK: = urem <2 x i32>
; CHECK: = srem <2 x i32>
; CHECK: = sext <2 x i32> %l to <2 x i64>
; CHECK: = sext <2 x i32> %r to <2 x i64>
; CHECK: = call <2 x i64> @__vc_builtin_udiv_v2i64
; CHECK: = call <2 x i64> @__vc_builtin_sdiv_v2i64
; CHECK: = call <2 x i64> @__vc_builtin_urem_v2i64
; CHECK: = call <2 x i64> @__vc_builtin_srem_v2i64
; CHECK: ret void

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

; COM: The presence of these __vc_builtin_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have built-in routines
define <2 x i64> @__vc_builtin_sdiv_v2i64(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__vc_builtin_srem_v2i64(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__vc_builtin_udiv_v2i64(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}
define <2 x i64> @__vc_builtin_urem_v2i64(<2 x i64>, <2 x i64>) #0 {
  ret <2 x i64> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
