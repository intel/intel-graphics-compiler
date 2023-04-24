;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s
declare <8 x i8> @llvm.genx.uuadd.sat.v8i8.v8i64(<8 x i64> , <8 x i64>)
define  <8 x i8> @test_uuadd_sat(<8 x i64> %op0, <8 x i64> %op1) {
  ; CHECK-LABEL: @test_uuadd_sat(
  ; CHECK-NEXT:  [[OP0:%.*]] = bitcast <8 x i64> %op0 to <16 x i32>
  ; CHECK-NEXT:  [[LO_OP0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[OP0]], i32 0, i32 8, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT:  [[HI_OP0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[OP0]], i32 0, i32 8, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT:  [[OP1:%.*]] = bitcast <8 x i64> %op1 to <16 x i32>
  ; CHECK-NEXT:  [[LO_OP1:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[OP1]], i32 0, i32 8, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT:  [[HI_OP1:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[OP1]], i32 0, i32 8, i32 2, i16 4, i32 undef)

  ; CHECK-NEXT:  [[ADDC_LO_SOA:%.*]] = call { <8 x i32>, <8 x i32> } @llvm.genx.addc.v8i32.v8i32(<8 x i32> [[LO_OP0]], <8 x i32> [[LO_OP1]])
  ; CHECK-NEXT:  [[ADDC_LO:%.*]] = extractvalue { <8 x i32>, <8 x i32> } [[ADDC_LO_SOA]], 1
  ; CHECK-NEXT:  [[CARRY:%.*]] = extractvalue { <8 x i32>, <8 x i32> } [[ADDC_LO_SOA]], 0

  ; CHECK-NEXT:  [[HI_OR:%.*]] = or <8 x i32> [[HI_OP0]], [[HI_OP1]]
  ; CHECK-NEXT:  [[CARRY_OR:%.*]] = or <8 x i32> [[HI_OR]], [[CARRY]]
  ; CHECK-NEXT:  [[SAT:%.*]] = icmp ne <8 x i32> [[CARRY_OR]], zeroinitializer

  ; CHECK-NEXT:  [[SELECT_SAT:%.*]] = select <8 x i1> [[SAT]], <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>, <8 x i32> [[ADDC_LO]]
  ; CHECK-NEXT:  [[TRUNC:%.*]] = call <8 x i8> @llvm.genx.uutrunc.sat.v8i8.v8i32(<8 x i32> [[SELECT_SAT]])
  ; CHECK-NEXT:  ret <8 x i8> [[TRUNC]]
  %1 = call <8 x i8> @llvm.genx.uuadd.sat.v8i8.v8i64(<8 x i64> %op0, <8 x i64> %op1)
  ret <8 x i8> %1
}
declare i8 @llvm.genx.uuadd.sat.i8.i64(i64 , i64)
define i8 @test_scalar_uuadd_sat(i64 %op0, i64 %op1) {
  ; CHECK-LABEL: @test_scalar_uuadd_sat(
  ; CHECK-NEXT:  [[OP0:%.*]] = bitcast i64 %op0 to <2 x i32>
  ; CHECK-NEXT:  [[LO_OP0:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[OP0]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT:  [[HI_OP0:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[OP0]], i32 0, i32 1, i32 2, i16 4, i32 undef)
  ; CHECK-NEXT:  [[OP1:%.*]] = bitcast i64 %op1 to <2 x i32>
  ; CHECK-NEXT:  [[LO_OP1:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[OP1]], i32 0, i32 1, i32 2, i16 0, i32 undef)
  ; CHECK-NEXT:  [[HI_OP1:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[OP1]], i32 0, i32 1, i32 2, i16 4, i32 undef)

  ; CHECK-NEXT:  [[ADDC_LO_SOA:%.*]] = call { <1 x i32>, <1 x i32> } @llvm.genx.addc.v1i32.v1i32(<1 x i32> [[LO_OP0]], <1 x i32> [[LO_OP1]])
  ; CHECK-NEXT:  [[ADDC_LO:%.*]] = extractvalue { <1 x i32>, <1 x i32> } [[ADDC_LO_SOA]], 1
  ; CHECK-NEXT:  [[CARRY:%.*]] = extractvalue { <1 x i32>, <1 x i32> } [[ADDC_LO_SOA]], 0

  ; CHECK-NEXT:  [[HI_OR:%.*]] = or <1 x i32> [[HI_OP0]], [[HI_OP1]]
  ; CHECK-NEXT:  [[CARRY_OR:%.*]] = or <1 x i32> [[HI_OR]], [[CARRY]]
  ; CHECK-NEXT:  [[SAT:%.*]] = icmp ne <1 x i32> [[CARRY_OR]], zeroinitializer

  ; CHECK-NEXT:  [[SELECT_SAT:%.*]] = select <1 x i1> [[SAT]], <1 x i32> <i32 -1>, <1 x i32> [[ADDC_LO]]
  ; CHECK-NEXT:  [[TRUNC:%.*]] = call i8 @llvm.genx.uutrunc.sat.i8.v1i32(<1 x i32> [[SELECT_SAT]])
  ; CHECK-NEXT:  ret i8 [[TRUNC]]
  %1 = call i8 @llvm.genx.uuadd.sat.i8.i64(i64 %op0, i64 %op1)
  ret i8 %1;
}
