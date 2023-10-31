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

declare <2 x double> @llvm.sqrt.v2f64(<2 x double>)
declare <2 x double> @llvm.genx.sqrt.v2f64(<2 x double>)
declare <2 x double> @llvm.genx.ieee.sqrt.v2f64(<2 x double>)

define dllexport spir_kernel void @test_kernel(<2 x double> %x) {
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %1 = call <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %2 = call nnan <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %3 = call ninf <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %4 = call nnan ninf <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %5 = call nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %6 = call nnan nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %7 = call ninf nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %8 = call nnan ninf nsz <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_fast_v2f64
  %9 = call afn <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_fast_v2f64
  %10 = call fast <2 x double> @llvm.sqrt.v2f64(<2 x double> %x)

  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_fast_v2f64
  %11 = call <2 x double> @llvm.genx.sqrt.v2f64(<2 x double> %x)
  ; CHECK: = call <2 x double> @__vc_builtin_fsqrt_v2f64
  %12 = call <2 x double> @llvm.genx.ieee.sqrt.v2f64(<2 x double> %x)

  ret void
}

; COM: The presence of these __vc_builtin_* funcitions is a HACK to trick VC
; COM: backend into thinking that we have built-in routines
define <2 x double> @__vc_builtin_fsqrt_v2f64(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}
define <2 x double> @__vc_builtin_fsqrt_fast_v2f64(<2 x double> %x) #0 {
  ret <2 x double> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
