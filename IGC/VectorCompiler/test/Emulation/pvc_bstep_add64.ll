;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -mattr=+lightweight_i64_emulation -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; COM: ===============================
; COM:             TEST #1
; COM: ===============================
; COM: add64 transforms as:
; COM: 1. operands are splitted to lo/hi parts
; COM: 2. [add_lo, carry] = genx_addc(src0.l0, src1.lo)
; COM: 3. add_hi = genx_add3(carry, src0.hi, src1.hi)
; COM: 4. add64  = combine(add_lo,add_hi)

; CHECK: @test_kernel
; CHECK: %add64 = add <8 x i64> %left, %right
; CHECK-NEXT: tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %add64)

define dllexport spir_kernel void @test_kernel(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %add64 = add <8 x i64> %left, %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %2, i32 0, <8 x i64> %add64)
  ret void
}

; COM: ===============================
; COM:            TEST #2
; COM: ===============================
; COM: check that if we have a scalar operation - it shall be recasted to vector
; COM: and then back to scalar

; CHECK: @scalar_kernel
; CHECK: %add64 = add i64 %scalar_left, %scalar_right

define dllexport spir_kernel void @scalar_kernel(i32 %0, i32 %1, i32 %2) {
  %left = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %0, i32 0)
  %right = tail call <8 x i64> @llvm.genx.oword.ld.v8i64(i32 0, i32 %1, i32 0)
  %scalar_left  = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %left, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %scalar_right = tail call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %right, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %add64 = add i64 %scalar_left, %scalar_right
  ret void
}

declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)
declare <8 x i32> @llvm.genx.add3.v8i32.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
declare { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32>, <8 x i32>)
declare <8 x i64> @llvm.genx.oword.ld.v8i64(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
