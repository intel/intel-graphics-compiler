;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.r0.v16i32()
declare <4 x i32> @llvm.genx.sr0.v4i32()
declare <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32, <4 x i32>)
declare <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32, <4 x i32>)

; CHECK-LABEL: test_r0
define <16 x i32> @test_r0() {
; CHECK: %r0 = call <16 x i32> @llvm.genx.r0.v16i32()
  %r0 = call <16 x i32> @llvm.genx.r0.v16i32()
  ret <16 x i32> %r0
}

; CHECK-LABEL: test_sr0
define <4 x i32> @test_sr0() {
; CHECK: %sr0 = call <4 x i32> @llvm.genx.sr0.v4i32()
  %sr0 = call <4 x i32> @llvm.genx.sr0.v4i32()
  ret <4 x i32> %sr0
}

; CHECK-LABEL: test_read_predefined
define <4 x i32> @test_read_predefined() {
; CHECK: %cr0 = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
  %cr0 = call <4 x i32> @llvm.genx.read.predef.reg.v4i32.v4i32(i32 14, <4 x i32> undef)
  ret <4 x i32> %cr0
}

; CHECK-LABEL: test_write_predefined
define <4 x i32> @test_write_predefined(<4 x i32> %cr0) {
; CHECK: %new = call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> %cr0)
  %new = call <4 x i32> @llvm.genx.write.predef.reg.v4i32.v4i32(i32 14, <4 x i32> %cr0)
  ret <4 x i32> %new
}
