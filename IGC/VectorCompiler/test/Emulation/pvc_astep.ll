;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=XeHPC -mattr=-lightweight_i64_emulation -S < %s | FileCheck %s

; COM: Astep does not need emulation, so all operatoins should be preserved

; CHECK: @test_kernel
; CHECK: %add64 = add i64 %scalar_left, %scalar_right
; CHECK-NEXT: %sub64 = sub i64 %scalar_left, %scalar_right
; CHECK-NEXT: %xor64 = xor i64 %scalar_left, %scalar_right
; CHECK-NEXT: %or64 = or i64 %scalar_left, %scalar_right
; CHECK-NEXT: %and64 = and i64 %scalar_left, %scalar_right
; CHECK-NEXT: %not64 = xor <8 x i64> %left, <i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1>

define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %scalar_left  = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %left, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %scalar_right = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %right, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %add64 = add i64 %scalar_left, %scalar_right
  %sub64 = sub i64 %scalar_left, %scalar_right
  %xor64 = xor i64 %scalar_left, %scalar_right
  %or64 = or i64 %scalar_left, %scalar_right
  %and64 = and i64 %scalar_left, %scalar_right
  %not64 = xor <8 x i64> %left, <i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1, i64 -1>
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
