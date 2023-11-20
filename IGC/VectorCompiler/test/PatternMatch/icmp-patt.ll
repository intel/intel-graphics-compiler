;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @test_icmp
; CHECK: (<16 x i32> [[VEC:%[A-Za-z0-9_.]+]], <16 x i1> [[FLAG1:%[A-Za-z0-9_.]+]], <16 x i1> [[FLAG2:%[A-Za-z0-9_.]+]], <16 x i1> [[FLAG3:%[A-Za-z0-9_.]+]], <2 x i32> [[INS:%[A-Za-z0-9_.]+]])

; CHECK-LABEL: case1:
; CHECK-NEXT: [[BITCAST:%[A-Za-z0-9_.]+]] = bitcast <16 x i32> [[VEC]] to <32 x i16>
; CHECK-NEXT: [[RDREG:%[A-Za-z0-9_.]+]] = call <16 x i16> @llvm.genx.rdregioni.v16i16.v32i16.i16(<32 x i16> [[BITCAST]], i32 32, i32 16, i32 2, i16 0, i32 undef)
; CHECK-NEXT: [[CMP_UGE:%[A-Za-z0-9_.]+]] = icmp uge <16 x i16> [[RDREG]], <i16 11111, i16 22222, i16 -32203, i16 -21092, i16 -9981, i16 -104, i16 12345, i16 23456, i16 -30969, i16 0, i16 -11215, i16 15243, i16 32415, i16 21211, i16 23232, i16 -21204>
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.v16i1(<16 x i32> [[VEC]], <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, <16 x i1> [[CMP_UGE]])

; CHECK-LABEL: case2:
; CHECK-NEXT: [[XOR:%[A-Za-z0-9_.]+]] = xor <16 x i1> [[FLAG1]], <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
; CHECK-NEXT: [[OR:%[A-Za-z0-9_.]+]] = or <16 x i1> [[FLAG2]], [[XOR]]
; CHECK-NEXT: [[AND:%[A-Za-z0-9_.]+]] = and <16 x i1> [[OR]], [[FLAG3]]
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.v16i1(<16 x i32> [[VEC]], <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, <16 x i1> [[AND]])

; CHECK-LABEL: case3:
; CHECK-NEXT: [[CMP_SGT:%[A-Za-z0-9_.]+]] = icmp sgt <2 x i32> [[INS]], zeroinitializer
; CHECK-NEXT: [[SEL_SGT:%[A-Za-z0-9_.]+]] = select <2 x i1> [[CMP_SGT]], <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v2i32.i16.v2i1(<16 x i32> zeroinitializer, <2 x i32> [[SEL_SGT]], i32 2, i32 2, i32 1, i16 0, i32 0, <2 x i1> <i1 true, i1 false>)
; CHECK-NEXT: [[CMP_SLE:%[A-Za-z0-9_.]+]] = icmp sle <2 x i32> [[INS]], zeroinitializer
; CHECK-NEXT: [[ALL:%[A-Za-z0-9_.]+]] = call i1 @llvm.genx.all.v2i1(<2 x i1> [[CMP_SLE]])
; CHECK-NEXT: tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.i1(<16 x i32> [[VEC]], <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 [[ALL]])

; CHECK-LABEL: case4:
; CHECK-NEXT: [[CMP_SGT2:%[A-Za-z0-9_.]+]] = icmp sgt <2 x i32> [[INS]], zeroinitializer
; CHECK-NEXT: [[SEL_SGT2:%[A-Za-z0-9_.]+]] = select <2 x i1> [[CMP_SGT2]], <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer
; CHECK-NEXT: [[WRREG_ZERO2:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v2i32.i16.v2i1(<16 x i32> [[VEC]], <2 x i32> [[SEL_SGT2]], i32 2, i32 2, i32 1, i16 0, i32 0, <2 x i1> <i1 true, i1 false>)
; CHECK-NEXT: [[AND_C2:%[A-Za-z0-9_.]+]] = and <16 x i32> [[WRREG_ZERO2]], <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
; CHECK-NEXT: [[CMP_SGT3:%[A-Za-z0-9_.]+]] = icmp sle <16 x i32> [[AND_C2]], zeroinitializer
; CHECK-NEXT: [[ALL2:%[A-Za-z0-9_.]+]] = call i1 @llvm.genx.all.v16i1(<16 x i1> [[CMP_SGT3]])
; CHECK-NEXT: [[WRREG4:%[A-Za-z0-9_.]+]] = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.i1(<16 x i32> [[VEC]], <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 [[ALL2]])

define spir_kernel void @test_icmp(<16 x i32> %vec, <16 x i1> %flag1, <16 x i1> %flag2, <16 x i1> %flag3, <2 x i32> %ins) {
case1:
  ; (V0 & 65535), C2 ==> icmp (trunc V0 to i16), C2
  %trunc = and <16 x i32> %vec, <i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535, i32 65535>
  %cmp_uge = icmp uge <16 x i32> %trunc, <i32 11111, i32 22222, i32 33333, i32 44444, i32 55555, i32 65432, i32 12345, i32 23456, i32 34567, i32 0, i32 54321, i32 15243, i32 32415, i32 21211, i32 23232, i32 44332>
  %wrreg1 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.v16i1(<16 x i32> %vec, <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, <16 x i1> %cmp_uge)
  br label %case2

case2:
  ; (icmp.ne V0, 0) where V0 is promoted from i1
  %sel1 = select <16 x i1> %flag1, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <16 x i32> zeroinitializer
  %sel2 = select <16 x i1> %flag2, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <16 x i32> zeroinitializer
  %sel3 = select <16 x i1> %flag3, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <16 x i32> zeroinitializer
  %not = xor <16 x i32> %sel1, <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>
  %or = or <16 x i32> %sel2, %not
  %and = and <16 x i32> %or, %sel3
  %cmp_ne1 = icmp ne <16 x i32> %and, zeroinitializer
  %wrreg2 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.v16i1(<16 x i32> %vec, <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, <16 x i1> %cmp_ne1)
  br label %case3

case3:
  ; transform the evaluation of flag == 0 into (~flag).all()
  %cmp_sgt = icmp sgt <2 x i32> %ins, zeroinitializer
  %sel_sgt = select <2 x i1> %cmp_sgt, <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer
  %wrreg_zero = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v2i32.i16.v2i1(<16 x i32> zeroinitializer, <2 x i32> %sel_sgt, i32 2, i32 2, i32 1, i16 0, i32 0, <2 x i1> <i1 true, i1 false>)
  %and_c1 = and <16 x i32> %wrreg_zero, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %cmp_ne2 = icmp ne <16 x i32> %and_c1, zeroinitializer
  %bitcast = bitcast <16 x i1> %cmp_ne2 to i16
  %cmp_eq = icmp eq i16 %bitcast, zeroinitializer
  %wrreg3 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.i1(<16 x i32> %vec, <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 %cmp_eq)
  br label %case4

case4:
  ; transform the evaluation of flag == 0 into (~flag).all(), simplifyCmp false
  %cmp_sgt2 = icmp sgt <2 x i32> %ins, zeroinitializer
  %sel_sgt2 = select <2 x i1> %cmp_sgt2, <2 x i32> <i32 1, i32 1>, <2 x i32> zeroinitializer
  %wrreg_zero2 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v2i32.i16.v2i1(<16 x i32> %vec, <2 x i32> %sel_sgt2, i32 2, i32 2, i32 1, i16 0, i32 0, <2 x i1> <i1 true, i1 false>)
  %and_c2 = and <16 x i32> %wrreg_zero2, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %cmp_sgt3 = icmp sgt <16 x i32> %and_c2, zeroinitializer
  %bitcast2 = bitcast <16 x i1> %cmp_sgt3 to i16
  %cmp_eq2 = icmp eq i16 %bitcast2, zeroinitializer
  %wrreg4 = tail call <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.i1(<16 x i32> %vec, <1 x i32> zeroinitializer, i32 0, i32 0, i32 0, i16 0, i32 0, i1 %cmp_eq2)

  ret void
}

declare <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare <16 x i32> @llvm.genx.wrregionf.v16i32.v2i32.i16.v2i1(<16 x i32>, <2 x i32>, i32, i32, i32, i16, i32, <2 x i1>)
declare <16 x i32> @llvm.genx.wrregionf.v16i32.v1i32.i16.v16i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, <16 x i1>)