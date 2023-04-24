;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s


; optimize
define <32 x i32> @test_match_v32i32(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)  {
  ;CHECK: [[V1:%[^ ]+]] = call <32 x i32> @llvm.genx.add3.v32i32.v32i32(<32 x i32> %op0, <32 x i32> %op1, <32 x i32> %op2)

  %1 = add <32 x i32> %op0, %op1
  %2 = add <32 x i32> %1, %op2

  ret <32 x i32> %2
}

; scalar value optimize
define i32 @test_match_i32(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[SIMPLE_RES:%[^ ]+]] = call i32 @llvm.genx.add3.i32.i32(i32 %op0, i32 %op1, i32 %op2)

  %1 = add i32 %op0, %op1
  %2 = add i32 %1, %op2

  ret i32 %2
}
; scalar value 2 const not opt
define i32 @test_match_i32cc(i32 %op0)  {
  ;CHECK: [[TMP1:%[^ ]+]] = add i32 %op0, 7
  ;CHECK-NEXT: [[TMP2:%[^ ]+]] = add i32 [[TMP1]], 10

  %1 = add i32 %op0, 7
  %2 = add i32 %1, 10

  ret i32 %2
}
; scalar value 1 const  opt
define i32 @test_match_i32c(i32 %op0, i32 %op1)  {
  ;CHECK: [[NEG_RES:%[^ ]+]] = call i32 @llvm.genx.add3.i32.i32(i32 %op0, i32 %op1, i32 10)

  %1 = add i32 %op0, %op1
  %2 = add i32 %1, 10

  ret i32 %2
}
; float type - not opt
define <32 x float> @test_v32float(<32 x float> %op0, <32 x float> %op1, <32 x float> %op2)  {
  ;CHECK: %1 = fadd <32 x float> %op0, %op1


  %1 = fadd <32 x float> %op0, %op1
  %2 = fadd <32 x float> %1, %op2

  ret <32 x float> %2
}
; scalar value negative
; place negative values before, after places add3
define i32 @test_match_i32_neg(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: [[NEG1:%[^ ]+]] = sub i32 0, %op1
  ;CHECK-NEXT: [[NEG2:%[^ ]+]] = sub i32 0, %op2
  ;CHECK-NEXT: [[NEG_RES:%[^ ]+]] = call i32 @llvm.genx.add3.i32.i32(i32 %op0, i32 [[NEG1]], i32 [[NEG2]])

  %1 = sub i32 %op0, %op1
  %2 = sub i32 %1, %op2

  ret i32 %2
}

declare <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
; scalar value splat
; place rdregion before, after places add3 with vector type
define <4 x i32> @test_match_i32_splat(i32 %op0, i32 %op1, <4 x i32> %op2)  {
  ;CHECK: [[BC1:%[^ ]+]] = bitcast i32 %op0 to <1 x i32>
  ;CHECK-NEXT: [[RD1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[BC1]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[BC2:%[^ ]+]] = bitcast i32 %op1 to <1 x i32>
  ;CHECK-NEXT: [[RD2:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[BC2]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[RES:%[^ ]+]] = call <4 x i32> @llvm.genx.add3.v4i32.v4i32(<4 x i32> [[RD1]], <4 x i32> [[RD2]], <4 x i32> %op2)
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %1 = add i32 %op0, %op1
  %bc = bitcast i32 %1 to <1 x i32>
  %rd = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %bc, i32 0, i32 4, i32 0, i16 0, i32 undef)
  %2 = add <4 x i32>  %rd, %op2

  ret <4 x i32> %2
}
; scalar value splat
; place rdregion before, after places add3 with vector type
define <4 x i32> @test_match_i32_splat2(i32 %op0, i32 %op1, <4 x i32> %op2)  {
  ;CHECK: [[BC1:%[^ ]+]] = bitcast i32 %op0 to <1 x i32>
  ;CHECK-NEXT: [[RD1:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[BC1]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[BC2:%[^ ]+]] = bitcast i32 %op1 to <1 x i32>
  ;CHECK-NEXT: [[RD2:%[^ ]+]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> [[BC2]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[RES:%[^ ]+]] = call <4 x i32> @llvm.genx.add3.v4i32.v4i32(<4 x i32> %op2, <4 x i32> [[RD1]], <4 x i32> [[RD2]])
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %1 = add i32 %op0, %op1
  %bc = bitcast i32 %1 to <1 x i32>
  %rd = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %bc, i32 0, i32 4, i32 0, i16 0, i32 undef)
  %2 = add <4 x i32> %op2, %rd

  ret <4 x i32> %2
}
; scalar value splat
; place rdregion before, after places add3 with vector type
declare <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16>, i32, i32, i32, i16, i32)
define <4 x i32> @test_match_zext_match(i16 %op0, i16 %op1, <4 x i32> %op2)  {
  ;CHECK: [[BC1:%[^ ]+]] = bitcast i16 %op0 to <1 x i16>
  ;CHECK-NEXT: [[RD1:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> [[BC1]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[EX1:%[^ ]+]] = zext <4 x i16> [[RD1]] to <4 x i32>
  ;CHECK-NEXT: [[BC2:%[^ ]+]] = bitcast i16 %op1 to <1 x i16>
  ;CHECK-NEXT: [[RD2:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> [[BC2]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[EX2:%[^ ]+]] = zext <4 x i16> [[RD2]] to <4 x i32>
  ;CHECK-NEXT: [[RES:%[^ ]+]] = call <4 x i32> @llvm.genx.add3.v4i32.v4i32(<4 x i32> %op2, <4 x i32> [[EX1]], <4 x i32> [[EX2]])
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %op0_c = zext i16 %op0 to i32
  %op1_c = zext i16 %op1 to i32

  %1 = add i32 %op0_c, %op1_c
  %bc = bitcast i32 %1 to <1 x i32>
  %rd = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %bc, i32 0, i32 4, i32 0, i16 0, i32 undef)
  %2 = add <4 x i32> %op2, %rd

  ret <4 x i32> %2
}
; scalar value splat
; place rdregion before, after places add3 with vector type
define <4 x i32> @test_match_sext_match(i16 %op0, i16 %op1, <4 x i32> %op2)  {
  ;CHECK: [[BC1:%[^ ]+]] = bitcast i16 %op0 to <1 x i16>
  ;CHECK-NEXT: [[RD1:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> [[BC1]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[SX1:%[^ ]+]] = sext <4 x i16> [[RD1]] to <4 x i32>
  ;CHECK-NEXT: [[BC2:%[^ ]+]] = bitcast i16 %op1 to <1 x i16>
  ;CHECK-NEXT: [[RD2:%[^ ]+]] = call <4 x i16> @llvm.genx.rdregioni.v4i16.v1i16.i16(<1 x i16> [[BC2]], i32 0, i32 1, i32 0, i16 0, i32 undef)
  ;CHECK-NEXT: [[SX2:%[^ ]+]] = sext <4 x i16> [[RD2]] to <4 x i32>
  ;CHECK-NEXT: [[RES:%[^ ]+]] = call <4 x i32> @llvm.genx.add3.v4i32.v4i32(<4 x i32> [[SX1]], <4 x i32> [[SX2]], <4 x i32> %op2)
  ;CHECK-NEXT: ret <4 x i32> [[RES]]

  %op0_c = sext i16 %op0 to i32
  %op1_c = sext i16 %op1 to i32

  %1 = add i32 %op0_c, %op1_c
  %bc = bitcast i32 %1 to <1 x i32>
  %rd = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %bc, i32 0, i32 4, i32 0, i16 0, i32 undef)
  %2 = add <4 x i32> %rd, %op2

  ret <4 x i32> %2
}

declare i32 @llvm.genx.ssadd.sat.i32.i32(i32, i32)
; No optimization
define i32 @test_match_i32ssadd(i32 %op0, i32 %op1, i32 %op2)  {
  ;CHECK: %1 = add i32 %op0, %op1
  ;CHECK-NEXT: %2 = call i32 @llvm.genx.ssadd.sat.i32.i32(i32 %1, i32 %op2)

  %1 = add i32 %op0, %op1
  %2 = call i32 @llvm.genx.ssadd.sat.i32.i32(i32 %1, i32 %op2)
  ret i32 %2
}
