;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"

; COM: ===============================
; COM:         TEST #1, #2, #3
; COM: ===============================
; COM: [or, xor, and] transform as (except for not/xor -1):
; COM: 1. operands are casted to valid type
; COM: 2. new [or, xor, and] with casted operands created
; COM: 3. new cast for old type created

; COM: |>========== OR ===========<|
; CHECK: @test_or
; CHECK: [[IV1:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[IV2:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>

; CHECK-NEXT: [[Res:[^ ]+]] = or <[[CT]]> [[IV1]], [[IV2]]

; CHECK-NEXT: [[ResCast: [^ ]+]] = bitcast <[[CT]]> [[Res]] to <8 x i64>

; COM: |>========= XOR ==========<|
; CHECK: @test_xor
; CHECK: [[IV1:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[IV2:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>

; CHECK-NEXT: [[Res:[^ ]+]] = xor <[[CT]]> [[IV1]], [[IV2]]

; CHECK-NEXT: [[ResCast: [^ ]+]] = bitcast <[[CT]]> [[Res]] to <8 x i64>

; COM: |>========= AND ==========<|
; CHECK: @test_and
; CHECK: [[IV1:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[IV2:%[^ ]+.cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>

; CHECK-NEXT: [[Res:[^ ]+]] = and <[[CT]]> [[IV1]], [[IV2]]

; CHECK-NEXT: [[ResCast: [^ ]+]] = bitcast <[[CT]]> [[Res]] to <8 x i64>

; COM: |>========= AND SCALAR I64 ==========<|
; CHECK: @test_and_i64
; CHECK: [[IV1:%[^ ]+.cast[0-9]*]] = bitcast i64 %left to <[[CT:2 x i32]]>
; CHECK-NEXT: [[IV2:%[^ ]+.cast[0-9]*]] = bitcast i64 %right to <[[CT]]>

; CHECK-NEXT: [[Res:[^ ]+]] = and <[[CT]]> [[IV1]], [[IV2]]

; CHECK-NEXT: [[ResCast: [^ ]+]] = bitcast <[[CT]]> [[Res]] to i64

define dllexport spir_kernel void @test_or(<8 x i64> %left, <8 x i64> %right, i32 %out) {
  %or64 = or <8 x i64> %left, %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %out, i32 0, <8 x i64> %or64)
  ret void
}
define dllexport spir_kernel void @test_xor(<8 x i64> %left, <8 x i64> %right, i32 %out) {
  %xor64 = xor <8 x i64> %left, %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %out, i32 0, <8 x i64> %xor64)
  ret void
}
define dllexport spir_kernel void @test_and(<8 x i64> %left, <8 x i64> %right, i32 %out) {
  %and64 = and <8 x i64> %left, %right
  tail call void @llvm.genx.oword.st.v8i64(i32 %out, i32 0, <8 x i64> %and64)
  ret void
}
define dllexport spir_kernel void @test_and_i64(i64 %left, i64 %right, i32 %out) {
  %and64 = and i64 %left, %right
  %and.bc = bitcast i64 %and64 to <1 x i64>
  tail call void @llvm.genx.oword.st.v1i64(i32 %out, i32 0, <1 x i64> %and.bc)
  ret void
}

declare void @llvm.genx.oword.st.v8i64(i32, i32, <8 x i64>)
declare void @llvm.genx.oword.st.v1i64(i32, i32, <1 x i64>)
