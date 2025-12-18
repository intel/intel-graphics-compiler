;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Xe3P -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_madmatch(
; CHECK: <16 x i8> [[C0:%[A-Za-z0-9_.]+]], <16 x i16> [[S0:%[A-Za-z0-9_.]+]], <16 x i16> [[S1:%[A-Za-z0-9_.]+]], <16 x i32> [[I0:%[A-Za-z0-9_.]+]], i16 [[OFF:%[A-Za-z0-9_.]+]]
define spir_kernel void @test_madmatch(<16 x i8> %c0, <16 x i16> %s0, <16 x i16> %s1, <16 x i32> %i0, i16 %off) {
; CHECK-LABEL: case1:
; CHECK-NEXT: [[C1_OP0_RDREG:%[A-Za-z0-9_.]+]] = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> [[S0]], i32 0, i32 0, i32 1, i16 [[OFF]], i32 0, i1 true)
; CHECK: [[VAL0:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[C0]] to <16 x i16>
; CHECK-NEXT: [[VAL1:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[C0]] to <16 x i16>
; CHECK-NEXT: [[MUL:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmul.v16i32.v16i16(<16 x i16> [[VAL0]], <16 x i16> [[VAL1]])
; CHECK-NEXT: [[NEG:%[A-Za-z0-9_.]+]] = sub <16 x i32> zeroinitializer, [[MUL]]
; CHECK-NEXT: [[MAD:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.uumad.v16i32.v16i16(<16 x i16> [[C1_OP0_RDREG]], <16 x i16> [[S1]], <16 x i32> [[NEG]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[MAD]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case1:
  ; X * Y +/- Z, isSub, SSMUL
  %c1_op0_rdreg = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> %s0, i32 0, i32 0, i32 1, i16 %off, i32 0, i1 true)
  %c1_op00_zext = zext <16 x i16> %c1_op0_rdreg to <16 x i32>
  %c1_op01_zext = zext <16 x i16> %s1 to <16 x i32>
  %c1_op0_mul = call <16 x i32> @llvm.genx.ssmul.v16i32.v16i32(<16 x i32> %c1_op00_zext, <16 x i32> %c1_op01_zext)
  %c1_op1_zext = sext <16 x i8> %c0 to <16 x i32>
  %c1_op1_mul = mul <16 x i32> %c1_op1_zext, %c1_op1_zext
  %c1_op0_sub = sub <16 x i32> %c1_op0_mul, %c1_op1_mul
  %c1_op0_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c1_op0_sub, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case2

; CHECK-LABEL: case2:
; CHECK-NEXT: [[C2_OP0_RDREG:%[A-Za-z0-9_.]+]] = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> [[S0]], i32 0, i32 0, i32 1, i16 [[OFF]], i32 0, i1 true)
; CHECK-NEXT: [[VAL2:%[A-Za-z0-9_.]+]] = zext <16 x i8> [[C0]] to <16 x i16>
; CHECK-NEXT: [[VAL3:%[A-Za-z0-9_.]+]] = zext <16 x i8> [[C0]] to <16 x i16>
; CHECK-NEXT: [[MUL1:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.uumul.v16i32.v16i16(<16 x i16> [[VAL2]], <16 x i16> [[VAL3]])
; CHECK-NEXT: [[MAD2:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i16(<16 x i16> [[C2_OP0_RDREG]], <16 x i16> [[S1]], <16 x i32> [[MUL1]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[MAD2]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case2:
  ; X * Y +/- Z, isAdd
  %c2_op0_rdreg = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> %s0, i32 0, i32 0, i32 1, i16 %off, i32 0, i1 true)
  %c2_op00_sext = sext <16 x i16> %c2_op0_rdreg to <16 x i32>
  %c2_op01_sext = sext <16 x i16> %s1 to <16 x i32>
  %c2_op0_mul = mul <16 x i32> %c2_op00_sext, %c2_op01_sext
  %c2_op1_zext = zext <16 x i8> %c0 to <16 x i32>
  %c2_op1_mul = mul <16 x i32> %c2_op1_zext, %c2_op1_zext
  %c2_op0_add = add <16 x i32> %c2_op0_mul, %c2_op1_mul
  %c2_op0_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c2_op0_add, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case3

; CHECK-LABEL: case3:
; CHECK-NEXT: [[C3_OP1_rdreg:%[A-Za-z0-9_.]+]] = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> [[S0]], i32 0, i32 0, i32 1, i16 [[OFF]], i32 0, i1 true)
; CHECK-NEXT: [[NEG3:%[A-Za-z0-9_.]+]] = sub <16 x i16> zeroinitializer, [[S1]]
; CHECK-NEXT: [[MAD4:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.sumad.v16i32.v16i16(<16 x i16> [[C3_OP1_rdreg]], <16 x i16> [[NEG3]], <16 x i32> [[I0]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[MAD4]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case3:
  ; Z +/- X * Y, isSub
  %c3_op1_rdreg = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> %s0, i32 0, i32 0, i32 1, i16 %off, i32 0, i1 true)
  %c3_op10_sext = sext <16 x i16> %c3_op1_rdreg to <16 x i32>
  %c3_op11_zext = zext <16 x i16> %s1 to <16 x i32>
  %c3_op1_mul = mul <16 x i32> %c3_op10_sext, %c3_op11_zext
  %c3_op1_sub = sub <16 x i32> %i0, %c3_op1_mul
  %c3_op1_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c3_op1_sub, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case4

; CHECK-LABEL: case4:
; CHECK-NEXT: [[C4_OP1_RDREG:%[A-Za-z0-9_.]+]] = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> [[S0]], i32 0, i32 0, i32 1, i16 [[OFF]], i32 0, i1 true)
; CHECK: [[MAD5:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.usmad.v16i32.v16i16(<16 x i16> [[C4_OP1_RDREG]], <16 x i16> [[S1]], <16 x i32> [[I0]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[MAD5]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case4:
  ; Z +/- X * Y, isAdd, UUMUL
  %c4_op1_rdreg = tail call <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16> %s0, i32 0, i32 0, i32 1, i16 %off, i32 0, i1 true)
  %c4_op10_zext = zext <16 x i16> %c4_op1_rdreg to <16 x i32>
  %c4_op11_sext = sext <16 x i16> %s1 to <16 x i32>
  %c4_op1_mul = call <16 x i32> @llvm.genx.uumul.v16i32.v16i32(<16 x i32> %c4_op10_zext, <16 x i32> %c4_op11_sext)
  %c4_op1_add = add <16 x i32> %i0, %c4_op1_mul
  %c4_op1_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c4_op1_add, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

; CHECK-LABEL: @test_madmatch_non_profitable(
; CHECK: <16 x i32> [[I0:%[A-Za-z0-9_.]+]], <16 x i32> [[I1:%[A-Za-z0-9_.]+]]
define spir_kernel void @test_madmatch_non_profitable(<16 x i32> %i0, <16 x i32> %i1) {
; CHECK-LABEL: case1:
; CHECK-NEXT: [[C5_OP0_MUL:%[A-Za-z0-9_.]+]] = mul <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[C5_OP0_MUL]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case1:
  ; X * Y +/- Z, not Profitable
  %c5_op0_mul = mul <16 x i32> %i0, %i1
  %c5_op0_add = add <16 x i32> %c5_op0_mul, zeroinitializer
  %c5_op0_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c5_op0_add, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case2

; CHECK-LABEL: case2:
; CHECK-NEXT: [[C6_OP1_MUL:%[A-Za-z0-9_.]+]] = mul <16 x i32> [[I1]], [[I0]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[C6_OP1_MUL]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case2:
  ; Z +/- X * Y, not Profitable
  %c6_op1_mul = mul <16 x i32> %i1, %i0
  %c6_op0_add = add <16 x i32> zeroinitializer, %c6_op1_mul
  %c6_op0_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c6_op0_add, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

; CHECK-LABEL: @test_madmatch_broadcast(
; CHECK: i8 [[SCALAR:%[A-Za-z0-9_.]+]], <16 x i32> [[I0:%[A-Za-z0-9_.]+]]
define spir_kernel void @test_madmatch_broadcast(i8 %scalar, <16 x i32> %i0) {
; CHECK-LABEL: case1:
; CHECK: [[VAL4:%[A-Za-z0-9_.]+]] = sext i8 [[SCALAR]] to i16
; CHECK-NEXT: [[VAL5:%[A-Za-z0-9_.]+]] = bitcast i16 [[VAL4]] to <1 x i16>
; CHECK-NEXT: [[SPLAT:%[A-Za-z0-9_.]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16(<1 x i16> [[VAL5]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[MUL6:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmul.v16i32.v16i16(<16 x i16> [[SPLAT]], <16 x i16> <i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122>)
; CHECK-NEXT: [[VAL6:%[A-Za-z0-9_.]+]] = bitcast i8 [[SCALAR]] to <1 x i8>
; CHECK-NEXT: [[SPLAT7:%[A-Za-z0-9_.]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1i8.i16(<1 x i8> [[VAL6]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[VAL7:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[SPLAT7]] to <16 x i32>
; CHECK-NEXT: [[NEG8:%[A-Za-z0-9_.]+]] = sub <16 x i32> zeroinitializer, [[MUL6]]
; CHECK-NEXT: [[MAD9:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[VAL7]], <16 x i32> <i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122>, <16 x i32> [[NEG8]])
; CHECK-NEXT: call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> [[MAD9]], i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case1:
  ; broadcast Op0, isSub
  %c7_op0_sext = sext i8 %scalar to i32
  %c7_op0_mul = mul i32 %c7_op0_sext, 1122
  %c7_op0_bitcast = bitcast i32 %c7_op0_mul to <1 x i32>
  %c7_op0_rdreg = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %c7_op0_bitcast, i32 0, i32 0, i32 0, i16 0, i32 0)
  %c7_op0_sub = sub <16 x i32> %c7_op0_rdreg, %c7_op0_rdreg
  %c7_op0_wrreg = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> %c7_op0_sub, i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case2

; CHECK-LABEL: case2:
; CHECK: [[VAL8:%[A-Za-z0-9_.]+]] = sext i8 [[SCALAR]] to i16
; CHECK-NEXT: [[VAL9:%[A-Za-z0-9_.]+]] = bitcast i16 [[VAL8]] to <1 x i16>
; CHECK-NEXT: [[SPLAT10:%[A-Za-z0-9_.]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16(<1 x i16> [[VAL9]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[MUL11:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmul.v16i32.v16i16(<16 x i16> [[SPLAT10]], <16 x i16> <i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122, i16 1122>)
; CHECK-NEXT: [[VAL10:%[A-Za-z0-9_.]+]] = bitcast i8 [[SCALAR]] to <1 x i8>
; CHECK-NEXT: [[SPLAT12:%[A-Za-z0-9_.]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1i8.i16(<1 x i8> [[VAL10]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[VAL11:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[SPLAT12]] to <16 x i32>
; CHECK-NEXT: [[MAD13:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[VAL11]], <16 x i32> <i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122>, <16 x i32> [[MUL11]])
; CHECK-NEXT: call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> [[MAD13]], i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case2:
  ; broadcast Op0, isAdd
  %c8_op0_sext = sext i8 %scalar to i32
  %c8_op0_mul = mul i32 %c8_op0_sext, 1122
  %c8_op0_bitcast = bitcast i32 %c8_op0_mul to <1 x i32>
  %c8_op0_rdreg = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %c8_op0_bitcast, i32 0, i32 0, i32 0, i16 0, i32 0)
  %c8_op0_add = add <16 x i32> %c8_op0_rdreg, %c8_op0_rdreg
  %c8_op0_wrreg = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> %c8_op0_add, i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case3

; CHECK-LABEL: case3:
; CHECK: [[VAL12:%[A-Za-z0-9_.]+]] = bitcast i8 [[SCALAR]] to <1 x i8>
; CHECK-NEXT: [[SPLAT14:%[A-Za-z0-9_.]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1i8.i16(<1 x i8> [[VAL12]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[VAL13:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[SPLAT14]] to <16 x i32>
; CHECK-NEXT: [[MAD15:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[VAL13]], <16 x i32> <i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122, i32 -1122>, <16 x i32> [[I0]])
; CHECK-NEXT: call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> [[MAD15]], i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case3:
  ; broadcast Op1, isSub
  %c9_op1_sext = sext i8 %scalar to i32
  %c9_op1_mul = mul i32 %c9_op1_sext, 1122
  %c9_op1_bitcast = bitcast i32 %c9_op1_mul to <1 x i32>
  %c9_op1_rdreg = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %c9_op1_bitcast, i32 0, i32 0, i32 0, i16 0, i32 0)
  %c9_op1_sub = sub <16 x i32> %i0, %c9_op1_rdreg
  %c9_op1_wrreg = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> %c9_op1_sub, i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case4

; CHECK-LABEL: case4:
; CHECK: [[VAL14:%[A-Za-z0-9_.]+]] = bitcast i8 [[SCALAR]] to <1 x i8>
; CHECK-NEXT: [[SPLAT16:%[A-Za-z0-9_.]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v1i8.i16(<1 x i8> [[VAL14]], i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK-NEXT: [[VAL15:%[A-Za-z0-9_.]+]] = sext <16 x i8> [[SPLAT16]] to <16 x i32>
; CHECK-NEXT: [[MAD17:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[VAL15]], <16 x i32> <i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122, i32 1122>, <16 x i32> [[I0]])
; CHECK-NEXT: call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> [[MAD17]], i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case4:
  ; broadcast Op1, isAdd
  %c10_op1_sext = sext i8 %scalar to i32
  %c10_op1_mul = mul i32 %c10_op1_sext, 1122
  %c10_op1_bitcast = bitcast i32 %c10_op1_mul to <1 x i32>
  %c10_op1_rdreg = call <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32> %c10_op1_bitcast, i32 0, i32 0, i32 0, i16 0, i32 0)
  %c10_op1_add = add <16 x i32> %i0, %c10_op1_rdreg
  %c10_op1_wrreg = call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32> %c10_op1_add, i32 0, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

; CHECK-LABEL: @test_not_madmatch(
; CHECK: i8 [[SCALAR:%[A-Za-z0-9_.]+]], <16 x i8> [[C0:%[A-Za-z0-9_.]+]], <16 x i16> [[S0:%[A-Za-z0-9_.]+]], <16 x i16> [[S1:%[A-Za-z0-9_.]+]], <16 x i32> [[I0:%[A-Za-z0-9_.]+]], <16 x i32> [[I1:%[A-Za-z0-9_.]+]], <16 x i32> [[I2:%[A-Za-z0-9_.]+]], i16 [[OFF:%[A-Za-z0-9_.]+]]
define spir_kernel void @test_not_madmatch(i8 %scalar, <16 x i8> %c0, <16 x i16> %s0, <16 x i16> %s1, <16 x i32> %i0, <16 x i32> %i1, <16 x i32> %i2, i16 %off) {
; CHECK-LABEL: case1:
; CHECK-NEXT: [[C11_SUB:%[A-Za-z0-9_.]+]] = sub <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: [[C11_MUL:%[A-Za-z0-9_.]+]] = mul <16 x i32> [[C11_SUB]], [[I2]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[C11_MUL]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
case1:
  ; user not a call
  %c11_sub = sub <16 x i32> %i0, %i1
  %c11_mul = mul <16 x i32> %c11_sub, %i2
  %c11_wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %c11_mul, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  br label %case2

; CHECK-LABEL: case2:
; CHECK-NEXT: [[C12_SUB:%[A-Za-z0-9_.]+]] = sub <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.uutrunc.sat.v16i32.v16i32(<16 x i32> [[C12_SUB]])
case2:
  ; uutrunc
  %c12_sub = sub <16 x i32> %i0, %i1
  %c12_uutrunc = tail call <16 x i32> @llvm.genx.uutrunc.sat.v16i32.v16i32(<16 x i32> %c12_sub)
  br label %case3

; CHECK-LABEL: case3:
; CHECK-NEXT: [[C13_SUB:%[A-Za-z0-9_.]+]] = sub <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.sutrunc.sat.v16i32.v16i32(<16 x i32> [[C13_SUB]])
case3:
  ; sutrunc
  %c13_sub = sub <16 x i32> %i0, %i1
  %c13_sutrunc = tail call <16 x i32> @llvm.genx.sutrunc.sat.v16i32.v16i32(<16 x i32> %c13_sub)
  br label %case4

; CHECK-LABEL: case4:
; CHECK-NEXT: [[C14_SUB:%[A-Za-z0-9_.]+]] = sub <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.ustrunc.sat.v16i32.v16i32(<16 x i32> [[C14_SUB]])
case4:
  ; ustrunc
  %c14_sub = sub <16 x i32> %i0, %i1
  %c14_ustrunc = tail call <16 x i32> @llvm.genx.ustrunc.sat.v16i32.v16i32(<16 x i32> %c14_sub)
  br label %case5

; CHECK-LABEL: case5:
; CHECK-NEXT: [[C15_SUB:%[A-Za-z0-9_.]+]] = sub <16 x i32> [[I0]], [[I1]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.sstrunc.sat.v16i32.v16i32(<16 x i32> [[C15_SUB]])
case5:
  ; sstrunc
  %c15_sub = sub <16 x i32> %i0, %i1
  %c15_sstrunc = tail call <16 x i32> @llvm.genx.sstrunc.sat.v16i32.v16i32(<16 x i32> %c15_sub)

  ret void
}

; CHECK-LABEL: @test_madmatch_shift(
; CHECK: <16 x i32> [[I0:%[A-Za-z0-9_.]+]], <16 x i32> [[I1:%[A-Za-z0-9_.]+]]
define spir_kernel void @test_madmatch_shift(<16 x i32> %i0, <16 x i32> %i1) {
; CHECK-NEXT: [[NEG18:%[A-Za-z0-9_.]+]] = sub <16 x i32> zeroinitializer, [[I1]]
; CHECK-NEXT: [[MAD19:%[A-Za-z0-9_.]+]] = call <16 x i32> @llvm.genx.ssmad.v16i32.v16i32(<16 x i32> [[I0]], <16 x i32> <i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4, i32 4>, <16 x i32> [[NEG18]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> [[MAD19]], <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)
  %shl = shl <16 x i32> %i0, <i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %sub = sub <16 x i32> %shl, %i1
  %wrreg = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32> %sub, <16 x i32> zeroinitializer, i32 0, i32 0, i32 1, i16 0, i32 0, i1 true)

  ret void
}

declare <16 x i32> @llvm.genx.uutrunc.sat.v16i32.v16i32(<16 x i32>)
declare <16 x i32> @llvm.genx.sutrunc.sat.v16i32.v16i32(<16 x i32>)
declare <16 x i32> @llvm.genx.ustrunc.sat.v16i32.v16i32(<16 x i32>)
declare <16 x i32> @llvm.genx.sstrunc.sat.v16i32.v16i32(<16 x i32>)

declare <16 x i32> @llvm.genx.uumul.v16i32.v16i32(<16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.ssmul.v16i32.v16i32(<16 x i32>, <16 x i32>)

declare <16 x i32> @llvm.genx.rdregioni.v16i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)
declare <16 x i16> @llvm.genx.rdregioni.v16i16.v16i16.i16.i1(<16 x i16>, i32, i32, i32, i16, i32, i1)

declare <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.i32.i16.i1(<16 x i32>, i32, i32, i32, i32, i16, i32, i1)
declare <16 x i32> @llvm.genx.wrregioni.v16i32.v16i32.v16i32.i16.i1(<16 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1)
