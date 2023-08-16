;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=-mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mattr=+mul_ddq -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=UseMulDDQ %s

declare i8 @llvm.genx.ssmul.sat.i8.i8(i8, i8)
declare i16 @llvm.genx.ssmul.sat.i16.i16(i16, i16)
declare i32 @llvm.genx.ssmul.sat.i32.i32(i32, i32)
declare i32 @llvm.genx.ssmul.i32.i32(i32, i32)

declare i16 @llvm.genx.sumul.sat.i16.i16(i16, i16)
declare i16 @llvm.genx.usmul.sat.i16.i16(i16, i16)

declare i8 @llvm.genx.uumul.sat.i8.i8(i8, i8)
declare i16 @llvm.genx.uumul.sat.i16.i16(i16, i16)
declare i32 @llvm.genx.uumul.sat.i32.i32(i32, i32)

; CHECK-LABEL: test1
define internal spir_func void @test1(i16 %op1, i16 %op2) {
; CHECK: %res1 = mul i32 %op1.sext, %op2.sext
; CHECK-NEXT: %res.sat = call i16 @llvm.genx.sstrunc.sat.i16.i32(i32 %res1)
  %res = call i16 @llvm.genx.ssmul.sat.i16.i16(i16 %op1, i16 %op2)
  ret void
}

; CHECK-LABEL: test2
define internal spir_func void @test2(i16 %op1, i16 %op2) {
; CHECK: %res1 = mul i32 %op1.zext, %op2.zext
; CHECK-NEXT: %res.sat = call i16 @llvm.genx.sutrunc.sat.i16.i32(i32 %res1)
  %res = call i16 @llvm.genx.sumul.sat.i16.i16(i16 %op1, i16 %op2)
  ret void
}

; CHECK-LABEL: test3
define internal spir_func void @test3(i16 %op1, i16 %op2) {
; CHECK: %res1 = mul i32 %op1.sext, %op2.sext
; CHECK-NEXT: %res.sat = call i16 @llvm.genx.ustrunc.sat.i16.i32(i32 %res1)
  %res = call i16 @llvm.genx.usmul.sat.i16.i16(i16 %op1, i16 %op2)
  ret void
}

; CHECK-LABEL: test4
define internal spir_func void @test4(i16 %op1, i16 %op2) {
; CHECK: %res1 = mul i32 %op1.zext, %op2.zext
; CHECK-NEXT: %res.sat = call i16 @llvm.genx.uutrunc.sat.i16.i32(i32 %res1)
  %res = call i16 @llvm.genx.uumul.sat.i16.i16(i16 %op1, i16 %op2)
  ret void
}

; CHECK-LABEL: testi8s
define internal spir_func void @testi8s(i8 %op1, i8 %op2) {
; CHECK: %res1 = mul i16 %op1.sext, %op2.sext
; CHECK-NEXT: %res.sat = call i8 @llvm.genx.sstrunc.sat.i8.i16(i16 %res1)
  %res = call i8 @llvm.genx.ssmul.sat.i8.i8(i8 %op1, i8 %op2)
  ret void
}

; CHECK-LABEL: testi8u
define internal spir_func void @testi8u(i8 %op1, i8 %op2) {
; CHECK: %res1 = mul i16 %op1.zext, %op2.zext
; CHECK-NEXT:  %res.sat = call i8 @llvm.genx.uutrunc.sat.i8.i16(i16 %res1)
  %res = call i8 @llvm.genx.uumul.sat.i8.i8(i8 %op1, i8 %op2)
  ret void
}

; CHECK-LABEL: testi32s
define internal spir_func void @testi32s(i32 %op1, i32 %op2) {
; CHECK: %op1.sext = sext i32 %op1 to i64
; CHECK: %op2.sext = sext i32 %op2 to i64
; CHECK: mul <1 x i32>
; CHECK: call <1 x i32> @llvm.genx.umulh.v1i32.v1i32
; CHECK-COUNT-2: mul <1 x i32>
; CHECK-NOT: mul <1 x i64>
; CHECK: %res.sat = call i32 @llvm.genx.sstrunc.sat.i32.i64(i64 %mul64recast)
; UseMulDDQ: %op1.sext = sext i32 %op1 to i64
; UseMulDDQ: %op2.sext = sext i32 %op2 to i64
; UseMulDDQ: call i64 @llvm.genx.uumul.i64.v1i32
; UseMulDDQ-COUNT-2: mul <1 x i32>
; UseMulDDQ-NOT: mul <1 x i64>
; UseMulDDQ: %res.sat = call i32 @llvm.genx.sstrunc.sat.i32.i64(i64 %mul64recast)
  %res = call i32 @llvm.genx.ssmul.sat.i32.i32(i32 %op1, i32 %op2)
  ret void
}

; CHECK-LABEL: testi32u
define internal spir_func void @testi32u(i32 %op1, i32 %op2) {
; CHECK: %op1.zext = zext i32 %op1 to i64
; CHECK: %op2.zext = zext i32 %op2 to i64
; CHECK: mul <1 x i32>
; CHECK: call <1 x i32> @llvm.genx.umulh.v1i32.v1i32
; CHECK-COUNT-2: mul <1 x i32>
; CHECK-NOT: mul <1 x i64>
; CHECK: %res.sat = call i32 @llvm.genx.uutrunc.sat.i32.i64(i64 %mul64recast)
; UseMulDDQ: %op1.zext = zext i32 %op1 to i64
; UseMulDDQ: %op2.zext = zext i32 %op2 to i64
; UseMulDDQ: call i64 @llvm.genx.uumul.i64.v1i32
; UseMulDDQ-COUNT-2: mul <1 x i32>
; UseMulDDQ-NOT: mul <1 x i64>
; UseMulDDQ: %res.sat = call i32 @llvm.genx.uutrunc.sat.i32.i64(i64 %mul64recast)
  %res = call i32 @llvm.genx.uumul.sat.i32.i32(i32 %op1, i32 %op2)
  ret void
}
