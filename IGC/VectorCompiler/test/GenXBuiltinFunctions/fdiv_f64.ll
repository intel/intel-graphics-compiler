;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXBuiltinFunctions -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeLPG -S < %s 2>&1 | FileCheck %s

; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

declare <2 x double> @llvm.genx.ieee.div.v2f64(<2 x double> %l, <2 x double> %r)

define dllexport spir_kernel void @test_kernel(<2 x double> %l, <2 x double> %r) {
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %1 = fdiv <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %2 = fdiv nnan <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %3 = fdiv ninf <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %4 = fdiv nnan ninf <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %5 = fdiv nsz <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %6 = fdiv nnan nsz <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %7 = fdiv ninf nsz <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %8 = fdiv nnan ninf nsz <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_fast_v2f64(<2 x double> %l, <2 x double> %r)
  %9 = fdiv arcp <2 x double> %l, %r
  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_fast_v2f64(<2 x double> %l, <2 x double> %r)
  %10 = fdiv fast <2 x double> %l, %r

  ; CHECK: = call <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r)
  %11 = call <2 x double> @llvm.genx.ieee.div.v2f64(<2 x double> %l, <2 x double> %r)

  ret void
}

; COM: The presence of these __vc_builtin_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have built-in routines
define <2 x double> @__vc_builtin_fdiv_v2f64(<2 x double> %l, <2 x double> %r) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__vc_builtin_fdiv_fast_v2f64(<2 x double> %l, <2 x double> %r) #0 {
  ret <2 x double> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
