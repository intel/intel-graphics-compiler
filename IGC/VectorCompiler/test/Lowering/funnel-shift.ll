;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target triple = "spir64-unknown-unknown"

declare <8 x i16> @llvm.fshl.v8i16(<8 x i16>, <8 x i16>, <8 x i16>)
declare <8 x i16> @llvm.fshr.v8i16(<8 x i16>, <8 x i16>, <8 x i16>)
declare <2 x i32> @llvm.fshl.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i32> @llvm.fshr.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
declare <2 x i64> @llvm.fshl.v2i64(<2 x i64>, <2 x i64>, <2 x i64>)

; CHECK-LABEL: test_rol
define internal spir_func void @test_rol(<8 x i16> %op1, <8 x i16> %op2) {
; CHECK: %rotate = call <8 x i16> @llvm.genx.rol.v8i16.v8i16(<8 x i16> %op1, <8 x i16> %op2)
  %res = call <8 x i16> @llvm.fshl.v8i16(<8 x i16> %op1, <8 x i16> %op1, <8 x i16> %op2)
  ret void
}

; CHECK-LABEL: test_ror
define internal spir_func void @test_ror(<8 x i16> %op1, <8 x i16> %op2) {
; CHECK: %rotate = call <8 x i16> @llvm.genx.ror.v8i16.v8i16(<8 x i16> %op1, <8 x i16> %op2)
  %res = call <8 x i16> @llvm.fshr.v8i16(<8 x i16> %op1, <8 x i16> %op1, <8 x i16> %op2)
  ret void
}

; CHECK-LABEL: test_fshl1
define internal spir_func void @test_fshl1(<2 x i32> %op1, <2 x i32> %op2) {
; CHECK-DAG: %fstpart = shl <2 x i32> %op1, <i32 15, i32 17>
; CHECK-DAG: %sndpart = lshr <2 x i32> %op2, <i32 17, i32 15>
; CHECK-DAG: %funnelshift = or <2 x i32> %fstpart, %sndpart
  %res = call <2 x i32> @llvm.fshl.v2i32(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> <i32 15, i32 49>)
  ret void
}

; CHECK-LABEL: test_fshl2
define internal spir_func void @test_fshl2(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> %op3) {
; CHECK-DAG: %shiftamnt = and <2 x i32> %op3, <i32 31, i32 31>
; CHECK-DAG: %complementshiftamnt = sub <2 x i32> <i32 32, i32 32>, %shiftamnt
; CHECK-DAG: %fstpart = shl <2 x i32> %op1, %shiftamnt
; CHECK-DAG: %sndpart = lshr <2 x i32> %op2, %complementshiftamnt
; CHECK-DAG: %funnelshift = or <2 x i32> %fstpart, %sndpart
  %res = call <2 x i32> @llvm.fshl.v2i32(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> %op3)
  ret void
}

; CHECK-LABEL: test_fshr1
define internal spir_func void @test_fshr1(<2 x i32> %op1, <2 x i32> %op2) {
; CHECK-DAG: %fstpart = shl <2 x i32> %op1, <i32 17, i32 15>
; CHECK-DAG: %sndpart = lshr <2 x i32> %op2, <i32 15, i32 17>
; CHECK-DAG: %funnelshift = or <2 x i32> %fstpart, %sndpart
  %res = call <2 x i32> @llvm.fshr.v2i32(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> <i32 47, i32 17>)
  ret void
}

; CHECK-LABEL: test_fshr2
define internal spir_func void @test_fshr2(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> %op3) {
; CHECK-DAG: %shiftamnt = and <2 x i32> %op3, <i32 31, i32 31>
; CHECK-DAG: %complementshiftamnt = sub <2 x i32> <i32 32, i32 32>, %shiftamnt
; CHECK-DAG: %fstpart = shl <2 x i32> %op1, %complementshiftamnt
; CHECK-DAG: %sndpart = lshr <2 x i32> %op2, %shiftamnt
; CHECK-DAG: %funnelshift = or <2 x i32> %fstpart, %sndpart
  %res = call <2 x i32> @llvm.fshr.v2i32(<2 x i32> %op1, <2 x i32> %op2, <2 x i32> %op3)
  ret void
}

; CHECK-LABEL: test_long
define internal spir_func void @test_long(<2 x i64> %op1, <2 x i64> %op2) {
; CHECK-DAG: %shiftamnt = and <2 x i64> %op2, <i64 63, i64 63>
; CHECK-DAG: %complementshiftamnt = sub <2 x i64> <i64 64, i64 64>, %shiftamnt
; CHECK-DAG: %fstpart = shl <2 x i64> %op1, %shiftamnt
; CHECK-DAG: %sndpart = lshr <2 x i64> %op1, %complementshiftamnt
; CHECK-DAG: %funnelshift = or <2 x i64> %fstpart, %sndpart
  %res = call <2 x i64> @llvm.fshl.v2i64(<2 x i64> %op1, <2 x i64> %op1, <2 x i64> %op2)
  ret void
}

