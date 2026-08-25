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
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,GRF32
; RUN: %opt %use_old_pass_manager% -GenXBuiltinFunctions -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,GRF64

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

; GRF32-LABEL: @test_simd32
; GRF32-NOT: = urem <32 x i8>
; GRF32-NOT: call <32 x i32> @__vc_builtin_urem_v32i32__rtz_
; GRF32: %[[CALL0:[^ ]+]] = call <16 x i32> @__vc_builtin_urem_v16i32__rtz_
; GRF32: %[[TRUNC0:[^ ]+]] = trunc <16 x i32> %[[CALL0]] to <16 x i8>
; GRF32: %[[JOIN0:[^ ]+]] = call <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8> poison, <16 x i8> %[[TRUNC0]], i32 16, i32 16, i32 1, i16 0, i32 undef, i1 true)
; GRF32: %[[CALL16:[^ ]+]] = call <16 x i32> @__vc_builtin_urem_v16i32__rtz_
; GRF32: %[[TRUNC16:[^ ]+]] = trunc <16 x i32> %[[CALL16]] to <16 x i8>
; GRF32: %[[JOIN16:[^ ]+]] = call <32 x i8> @llvm.genx.wrregioni.v32i8.v16i8.i16.i1(<32 x i8> %[[JOIN0]], <16 x i8> %[[TRUNC16]], i32 16, i32 16, i32 1, i16 16, i32 undef, i1 true)
; GRF32-NOT: call <32 x i32> @__vc_builtin_urem_v32i32__rtz_
; GRF32: ret <32 x i8> %[[JOIN16]]

; GRF64-LABEL: @test_simd32
; GRF64-NOT: = urem <32 x i8>
; GRF64-NOT: call <16 x i32> @__vc_builtin_urem_v16i32__rtz_
; GRF64: %[[CALL32:[^ ]+]] = call <32 x i32> @__vc_builtin_urem_v32i32__rtz_
; GRF64: %[[TRUNC32:[^ ]+]] = trunc <32 x i32> %[[CALL32]] to <32 x i8>
; GRF64-NOT: call <16 x i32> @__vc_builtin_urem_v16i32__rtz_
; GRF64: ret <32 x i8> %[[TRUNC32]]

; GRF32-LABEL: @test_simd64_sdiv
; GRF32-NOT: = sdiv <64 x i8>
; GRF32: sext <16 x i8>
; GRF32-COUNT-4: call <16 x i32> @__vc_builtin_sdiv_v16i32__rtz_
; GRF32: ret <64 x i8>

; GRF64-LABEL: @test_simd64_sdiv
; GRF64-NOT: = sdiv <64 x i8>
; GRF64: sext <32 x i8>
; GRF64-COUNT-2: call <32 x i32> @__vc_builtin_sdiv_v32i32__rtz_
; GRF64: ret <64 x i8>

; GRF32-LABEL: @test_simd64_srem
; GRF32-NOT: = srem <64 x i8>
; GRF32: sext <16 x i8>
; GRF32-COUNT-4: call <16 x i32> @__vc_builtin_srem_v16i32__rtz_
; GRF32: ret <64 x i8>

; GRF64-LABEL: @test_simd64_srem
; GRF64-NOT: = srem <64 x i8>
; GRF64: sext <32 x i8>
; GRF64-COUNT-2: call <32 x i32> @__vc_builtin_srem_v32i32__rtz_
; GRF64: ret <64 x i8>

; GRF32-LABEL: @test_simd64_udiv
; GRF32-NOT: = udiv <64 x i8>
; GRF32: zext <16 x i8>
; GRF32-COUNT-4: call <16 x i32> @__vc_builtin_udiv_v16i32__rtz_
; GRF32: ret <64 x i8>

; GRF64-LABEL: @test_simd64_udiv
; GRF64-NOT: = udiv <64 x i8>
; GRF64: zext <32 x i8>
; GRF64-COUNT-2: call <32 x i32> @__vc_builtin_udiv_v32i32__rtz_
; GRF64: ret <64 x i8>

; GRF32-LABEL: @test_simd64_urem
; GRF32-NOT: = urem <64 x i8>
; GRF32: zext <16 x i8>
; GRF32-COUNT-4: call <16 x i32> @__vc_builtin_urem_v16i32__rtz_
; GRF32: ret <64 x i8>

; GRF64-LABEL: @test_simd64_urem
; GRF64-NOT: = urem <64 x i8>
; GRF64: zext <32 x i8>
; GRF64-COUNT-2: call <32 x i32> @__vc_builtin_urem_v32i32__rtz_
; GRF64: ret <64 x i8>

define dllexport spir_kernel void @test_kernel(<2 x i8> %sa, <2 x i8> %sb,
                                               <2 x i16> %ua, <2 x i16> %ub) {
  %srem = srem <2 x i8> %sa, %sb
  %udiv = udiv <2 x i16> %ua, %ub
  ret void
}

define <32 x i8> @test_simd32(<32 x i8> %a, <32 x i8> %b) {
  %urem = urem <32 x i8> %a, %b
  ret <32 x i8> %urem
}

define <64 x i8> @test_simd64_sdiv(<64 x i8> %a, <64 x i8> %b) {
  %sdiv = sdiv <64 x i8> %a, %b
  ret <64 x i8> %sdiv
}

define <64 x i8> @test_simd64_srem(<64 x i8> %a, <64 x i8> %b) {
  %srem = srem <64 x i8> %a, %b
  ret <64 x i8> %srem
}

define <64 x i8> @test_simd64_udiv(<64 x i8> %a, <64 x i8> %b) {
  %udiv = udiv <64 x i8> %a, %b
  ret <64 x i8> %udiv
}

define <64 x i8> @test_simd64_urem(<64 x i8> %a, <64 x i8> %b) {
  %urem = urem <64 x i8> %a, %b
  ret <64 x i8> %urem
}

; COM: Presence of these __vc_builtin_* functions tricks the VC backend into
; COM: believing the emulation routines are available.
define <2 x i32> @__vc_builtin_srem_v2i32__rtz_(<2 x i32>, <2 x i32>) #0 {
  ret <2 x i32> zeroinitializer
}
define <2 x i32> @__vc_builtin_udiv_v2i32__rtz_(<2 x i32>, <2 x i32>) #0 {
  ret <2 x i32> zeroinitializer
}
define <16 x i32> @__vc_builtin_sdiv_v16i32__rtz_(<16 x i32>, <16 x i32>) #0 {
  ret <16 x i32> zeroinitializer
}
define <32 x i32> @__vc_builtin_sdiv_v32i32__rtz_(<32 x i32>, <32 x i32>) #0 {
  ret <32 x i32> zeroinitializer
}
define <16 x i32> @__vc_builtin_srem_v16i32__rtz_(<16 x i32>, <16 x i32>) #0 {
  ret <16 x i32> zeroinitializer
}
define <32 x i32> @__vc_builtin_srem_v32i32__rtz_(<32 x i32>, <32 x i32>) #0 {
  ret <32 x i32> zeroinitializer
}
define <16 x i32> @__vc_builtin_udiv_v16i32__rtz_(<16 x i32>, <16 x i32>) #0 {
  ret <16 x i32> zeroinitializer
}
define <32 x i32> @__vc_builtin_udiv_v32i32__rtz_(<32 x i32>, <32 x i32>) #0 {
  ret <32 x i32> zeroinitializer
}
define <16 x i32> @__vc_builtin_urem_v16i32__rtz_(<16 x i32>, <16 x i32>) #0 {
  ret <16 x i32> zeroinitializer
}
define <32 x i32> @__vc_builtin_urem_v32i32__rtz_(<32 x i32>, <32 x i32>) #0 {
  ret <32 x i32> zeroinitializer
}

attributes #0 = { "VC.Builtin" }
