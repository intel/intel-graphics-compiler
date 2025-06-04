;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-custom-safe-opt -dce -S < %s | FileCheck %s

declare i32 @llvm.genx.GenISA.bfn.i32(i32, i32, i32, i8)

define <2 x i32> @test_bfn_0xD8_i32v2() {
; CHECK-LABEL: define <2 x i32> @test_bfn_0xD8_i32v2(
; CHECK-NEXT:    %1 = insertelement <2 x i32> undef, i32 -30246, i32 0
; CHECK-NEXT:    %2 = insertelement <2 x i32> %1, i32 840, i32 1
; CHECK-NEXT:    ret <2 x i32> %2
;
  %1 = call i32 @llvm.genx.GenISA.bfn.i32(i32 -32631, i32 -19240, i32 -30373, i8 -40)
  %2 = call i32 @llvm.genx.GenISA.bfn.i32(i32 -4381, i32 832, i32 -6389, i8 -40)
  %3 = insertelement <2 x i32> undef, i32 %1, i32 0
  %4 = insertelement <2 x i32> %3, i32 %2, i32 1
  ret <2 x i32> %4
}
