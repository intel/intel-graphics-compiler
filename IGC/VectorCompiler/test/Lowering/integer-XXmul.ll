;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN:      -mtriple=spir64-unknown-unknown -mattr=+mul_ddq -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK_WITH_MUL_DDQ

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN:      -mtriple=spir64-unknown-unknown -mattr=-mul_ddq -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK_NO_MUL_DDQ

declare i8 @llvm.genx.ssmul.i8.i8(i8, i8)
declare i16 @llvm.genx.ssmul.i16.i16(i16, i16)
declare i32 @llvm.genx.ssmul.i32.i32(i32, i32)
declare i64 @llvm.genx.ssmul.i64.i32(i32, i32)
declare <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32>, <2 x i32>)

declare i16 @llvm.genx.sumul.i16.i16(i16, i16)
declare i16 @llvm.genx.usmul.i16.i16(i16, i16)

declare i8 @llvm.genx.uumul.i8.i8(i8, i8)
declare i16 @llvm.genx.uumul.i16.i16(i16, i16)
declare i64 @llvm.genx.uumul.i64.i32(i32, i32)
declare <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32>, <2 x i32>)

declare i64 @llvm.genx.ssmul.i64.i8(i8, i8)
declare i64 @llvm.genx.ssmul.i64.i16(i16, i16)
declare i64 @llvm.genx.uumul.i64.i8(i8, i8)
declare i64 @llvm.genx.uumul.i64.i16(i16, i16)

; CHECK_WITH_MUL_DDQ: testi32s
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.ssmul.i64.i32(i32 %op1, i32 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi32s
; CHECK_NO_MUL_DDQ-NEXT: [[MUL:[^ ]+]] = mul i32 %op1, %op2
; CHECK_NO_MUL_DDQ-NEXT: [[MULH:[^ ]+]] = call i32 @llvm.genx.smulh.i32.i32(i32 %op1, i32 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_LO:[^ ]+]] = bitcast i32 [[MUL]] to <1 x i32>
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_HI:[^ ]+]] = bitcast i32 [[MULH]] to <1 x i32>
; CHECK_NO_MUL_DDQ-NEXT: [[PJOIN:[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i32> undef, <1 x i32> [[CAST_LO]], i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[JOINED:[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i32> [[PJOIN]], <1 x i32> [[CAST_HI]], i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_RES:[^ ]+]]  = bitcast <2 x i32> [[JOINED]] to <1 x i64>
; CHECK_NO_MUL_DDQ-NEXT: [[RECAST:[^ ]+]] = bitcast <1 x i64> [[CAST_RES]] to i64
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi32s(i32 %op1, i32 %op2) {
  %res = call i64 @llvm.genx.ssmul.i64.i32(i32 %op1, i32 %op2)
  ret void
}

; CHECK_WITH_MUL_DDQ: testi32sv
; CHECK_WITH_MUL_DDQ-NEXT: call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi32sv
; CHECK_NO_MUL_DDQ-NEXT: [[MUL:[^ ]+]] = mul <2 x i32> %op1, %op2
; CHECK_NO_MUL_DDQ-NEXT: [[MULH:[^ ]+]] = call <2 x i32> @llvm.genx.smulh.v2i32.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[PJOIN:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<4 x i32> undef, <2 x i32> [[MUL]], i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[JOINED:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<4 x i32> [[PJOIN]], <2 x i32> [[MULH]], i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_RES:[^ ]+]]  = bitcast <4 x i32> [[JOINED]] to <2 x i64>
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi32sv(<2 x i32> %op1, <2 x i32> %op2) {
  %res = call <2 x i64> @llvm.genx.ssmul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
  ret void
}

; CHECK_WITH_MUL_DDQ: testi32u
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.uumul.i64.i32(i32 %op1, i32 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi32u
; CHECK_NO_MUL_DDQ-NEXT: [[MUL:[^ ]+]] = mul i32 %op1, %op2
; CHECK_NO_MUL_DDQ-NEXT: [[MULH:[^ ]+]] = call i32 @llvm.genx.umulh.i32.i32(i32 %op1, i32 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_LO:[^ ]+]] = bitcast i32 [[MUL]] to <1 x i32>
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_HI:[^ ]+]] = bitcast i32 [[MULH]] to <1 x i32>
; CHECK_NO_MUL_DDQ-NEXT: [[PJOIN:[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i32> undef, <1 x i32> [[CAST_LO]], i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[JOINED:[^ ]+]] = call <2 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<2 x i32> [[PJOIN]], <1 x i32> [[CAST_HI]], i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_RES:[^ ]+]]  = bitcast <2 x i32> [[JOINED]] to <1 x i64>
; CHECK_NO_MUL_DDQ-NEXT: [[RECAST:[^ ]+]] = bitcast <1 x i64> [[CAST_RES]] to i64
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi32u(i32 %op1, i32 %op2) {
  %res = call i64 @llvm.genx.uumul.i64.i32(i32 %op1, i32 %op2)
  ret void
}

; CHECK_WITH_MUL_DDQ: testi32uv
; CHECK_WITH_MUL_DDQ-NEXT: call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi32uv
; CHECK_NO_MUL_DDQ-NEXT: [[MUL:[^ ]+]] = mul <2 x i32> %op1, %op2
; CHECK_NO_MUL_DDQ-NEXT: [[MULH:[^ ]+]] = call <2 x i32> @llvm.genx.umulh.v2i32.v2i32(<2 x i32> %op1, <2 x i32> %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[PJOIN:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<4 x i32> undef, <2 x i32> [[MUL]], i32 0, i32 2, i32 2, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[JOINED:[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.{{[^(]+}}(<4 x i32> [[PJOIN]], <2 x i32> [[MULH]], i32 0, i32 2, i32 2, i16 4, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-NEXT: [[CAST_RES:[^ ]+]]  = bitcast <4 x i32> [[JOINED]] to <2 x i64>
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi32uv(<2 x i32> %op1, <2 x i32> %op2) {
  %res = call <2 x i64> @llvm.genx.uumul.v2i64.v2i32(<2 x i32> %op1, <2 x i32> %op2)
  ret void
}

; CHECK_WITH_MUL_DDQ: testi8u
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.ssmul.i64.i8(i8 %op1, i8 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.uumul.i64.i8(i8 %op1, i8 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi8u
; CHECK_NO_MUL_DDQ-NEXT: [[SMUL:[^ ]+]] = call i16 @llvm.genx.ssmul.i16.i8(i8 %op1, i8 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[SEXT:[^ ]+]] = sext i16 [[SMUL]] to i64
; CHECK_NO_MUL_DDQ-NEXT: [[UMUL:[^ ]+]] = call i16 @llvm.genx.uumul.i16.i8(i8 %op1, i8 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[ZEXT:[^ ]+]] = zext i16 [[UMUL]] to i64
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi8u(i8 %op1, i8 %op2) {
  %res_s = call i64 @llvm.genx.ssmul.i64.i8(i8 %op1, i8 %op2)
  %res_u = call i64 @llvm.genx.uumul.i64.i8(i8 %op1, i8 %op2)
  ret void
}

; CHECK_WITH_MUL_DDQ: testi16u
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.ssmul.i64.i16(i16 %op1, i16 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: call i64 @llvm.genx.uumul.i64.i16(i16 %op1, i16 %op2)
; CHECK_WITH_MUL_DDQ-NEXT: ret void

; CHECK_NO_MUL_DDQ: testi16u
; CHECK_NO_MUL_DDQ-NEXT: [[SMUL:[^ ]+]] = call i32 @llvm.genx.ssmul.i32.i16(i16 %op1, i16 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[SEXT:[^ ]+]] = sext i32 [[SMUL]] to i64
; CHECK_NO_MUL_DDQ-NEXT: [[UMUL:[^ ]+]] = call i32 @llvm.genx.uumul.i32.i16(i16 %op1, i16 %op2)
; CHECK_NO_MUL_DDQ-NEXT: [[ZEXT:[^ ]+]] = zext i32 [[UMUL]] to i64
; CHECK_NO_MUL_DDQ-NEXT: ret void
define internal spir_func void @testi16u(i16 %op1, i16 %op2) {
  %res_s = call i64 @llvm.genx.ssmul.i64.i16(i16 %op1, i16 %op2)
  %res_u = call i64 @llvm.genx.uumul.i64.i16(i16 %op1, i16 %op2)
  ret void
}

