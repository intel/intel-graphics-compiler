;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; On platforms without native 32-bit integer div/rem (here XeHPC) the emulation
; must promote sub-i32 div/rem to i32 and use the precise "__rtz_" builtin:
; sign-extend for signed ops, zero-extend for unsigned ops, then truncate back.

; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

; CHECK-LABEL: @test_kernel
; CHECK-NOT: = srem <2 x i8>
; CHECK: %[[SA:[^ ]+]] = sext <2 x i8> %sa to <2 x i32>
; CHECK: %[[SB:[^ ]+]] = sext <2 x i8> %sb to <2 x i32>
; CHECK: %[[SR:[^ ]+]] = call <2 x i32> @__vc_builtin_srem_v2i32__rtz_(<2 x i32> %[[SA]], <2 x i32> %[[SB]])
; CHECK: = trunc <2 x i32> %[[SR]] to <2 x i8>
; CHECK-NOT: = udiv <2 x i16>
; CHECK: %[[UA:[^ ]+]] = zext <2 x i16> %ua to <2 x i32>
; CHECK: %[[UB:[^ ]+]] = zext <2 x i16> %ub to <2 x i32>
; CHECK: %[[UD:[^ ]+]] = call <2 x i32> @__vc_builtin_udiv_v2i32__rtz_(<2 x i32> %[[UA]], <2 x i32> %[[UB]])
; CHECK: = trunc <2 x i32> %[[UD]] to <2 x i16>

define dllexport spir_kernel void @test_kernel(<2 x i8> %sa, <2 x i8> %sb,
                                               <2 x i16> %ua, <2 x i16> %ub) {
  %srem = srem <2 x i8> %sa, %sb
  %udiv = udiv <2 x i16> %ua, %ub
  ret void
}

; COM: Presence of these __vc_builtin_* functions tricks the VC backend into
; COM: believing the emulation routines are available.
define <2 x i32> @__vc_builtin_srem_v2i32__rtz_(<2 x i32>, <2 x i32>) #0 {
  ret <2 x i32> zeroinitializer
}
define <2 x i32> @__vc_builtin_udiv_v2i32__rtz_(<2 x i32>, <2 x i32>) #0 {
  ret <2 x i32> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
