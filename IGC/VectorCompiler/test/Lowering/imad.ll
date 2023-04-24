;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN:      -mtriple=spir64-unknown-unknown -mattr=-mul_ddq -S < %s | \
; RUN: FileCheck %s --check-prefix=CHECK_NO_MUL_DDQ

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 \
; RUN:      -mtriple=spir64-unknown-unknown -mattr=+mul_ddq -S < %s | \
; RUN: FileCheck %s  --check-prefix=CHECK_WITH_MUL_DDQ

declare { <16 x i32>, <16 x i32> } @llvm.genx.uimad.v16i32(<16 x i32>, <16 x i32>, <16 x i32>)
; CHECK-LABEL @test_madw_Sz16
define { <16 x i32>, <16 x i32> } @test_uimad_Sz16(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK_NO_MUL_DDQ-DAG: [[OP0_SPLIT0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op0, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[OP1_SPLIT0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op1, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[OP2_SPLIT0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op2, i32 8, i32 8, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[MADW_SPLIT0:%.*]] = call <16 x i32> @llvm.genx.umadw.v16i32.v8i32(<8 x i32> [[OP0_SPLIT0]], <8 x i32> [[OP1_SPLIT0]], <8 x i32> [[OP2_SPLIT0]])

; CHECK_NO_MUL_DDQ-DAG: [[HI_SPLIT0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[MADW_SPLIT0]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[LO_SPLIT0:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[MADW_SPLIT0]], i32 8, i32 8, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG: [[JOIN_HI_SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[HI_SPLIT0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-DAG: [[JOIN_LO_SPLIT0:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> [[LO_SPLIT0]], i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)

; CHECK_NO_MUL_DDQ-DAG: [[OP0_SPLIT8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op0, i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[OP1_SPLIT8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op1, i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[OP2_SPLIT8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %op2, i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[MADW_SPLIT8:%.*]] = call <16 x i32> @llvm.genx.umadw.v16i32.v8i32(<8 x i32> [[OP0_SPLIT8]], <8 x i32> [[OP1_SPLIT8]], <8 x i32> [[OP2_SPLIT8]])

; CHECK_NO_MUL_DDQ-DAG: [[HI_SPLIT8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[MADW_SPLIT8]], i32 8, i32 8, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG: [[LO_SPLIT8:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[MADW_SPLIT8]], i32 8, i32 8, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG: [[RES_HI:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[JOIN_HI_SPLIT0]], <8 x i32> [[HI_SPLIT8]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-DAG: [[RES_LO:%.*]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> [[JOIN_LO_SPLIT0]], <8 x i32> [[LO_SPLIT8]], i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)

; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { <16 x i32>, <16 x i32> } undef, <16 x i32> [[RES_HI]], 0
; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { <16 x i32>, <16 x i32> } [[INSERT_VAL_HI]], <16 x i32> [[RES_LO]], 1

; CHECK_NO_MUL_DDQ-DAG: ret { <16 x i32>, <16 x i32> } [[INSERT_VAL_LO]]

  %1 = call { <16 x i32>, <16 x i32> } @llvm.genx.uimad.v16i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2)
  %2 = extractvalue { <16 x i32>, <16 x i32> } %1, 0
  %3 = extractvalue { <16 x i32>, <16 x i32> } %1, 1
  %4 = insertvalue { <16 x i32>, <16 x i32> } undef, <16 x i32> %2, 0
  %5 = insertvalue { <16 x i32>, <16 x i32> } %4, <16 x i32> %3, 1
  ret { <16 x i32>, <16 x i32> } %5
}

declare { <7 x i32>, <7 x i32> } @llvm.genx.uimad.v7i32(<7 x i32>, <7 x i32>, <7 x i32>)
; CHECK-LABEL @test_madw_Sz7
define { <7 x i32>, <7 x i32> } @test_uimad_Sz7(<7 x i32> %op0, <7 x i32> %op1, <7 x i32> %op2) {
; CHECK_NO_MUL_DDQ-DAG:  [[OP0_SPLIT0:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %op0, i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP1_SPLIT0:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %op1, i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP2_SPLIT0:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v7i32.i16(<7 x i32> %op2, i32 4, i32 4, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[MADW_SPLIT0:%.*]] = call <16 x i32> @llvm.genx.umadw.v16i32.v4i32(<4 x i32> [[OP0_SPLIT0]], <4 x i32> [[OP1_SPLIT0]], <4 x i32> [[OP2_SPLIT0]])

; CHECK_NO_MUL_DDQ-DAG:  [[HI_SPLIT0:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> [[MADW_SPLIT0]], i32 4, i32 4, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[LO_SPLIT0:%.*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v16i32.i16(<16 x i32> [[MADW_SPLIT0]], i32 4, i32 4, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG:  [[JOIN_HI_SPLIT0:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v4i32.i16.i1(<7 x i32> undef, <4 x i32> [[HI_SPLIT0]], i32 4, i32 4, i32 1, i16 0, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-DAG:  [[JOIN_LO_SPLIT0:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v4i32.i16.i1(<7 x i32> undef, <4 x i32> [[LO_SPLIT0]], i32 4, i32 4, i32 1, i16 0, i32 undef, i1 true)

; CHECK_NO_MUL_DDQ-DAG:  [[OP0_SPLIT4:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %op0, i32 2, i32 2, i32 1, i16 16, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP1_SPLIT4:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %op1, i32 2, i32 2, i32 1, i16 16, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP2_SPLIT4:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v7i32.i16(<7 x i32> %op2, i32 2, i32 2, i32 1, i16 16, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[MADW_SPLIT4:%.*]] = call <16 x i32> @llvm.genx.umadw.v16i32.v2i32(<2 x i32> [[OP0_SPLIT4:%.*]], <2 x i32> [[OP1_SPLIT4:%.*]], <2 x i32> [[OP2_SPLIT4]])

; CHECK_NO_MUL_DDQ-DAG:  [[HI_SPLIT4:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[MADW_SPLIT4]], i32 2, i32 2, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[LO_SPLIT4:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[MADW_SPLIT4]], i32 2, i32 2, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG:  [[JOIN_HI_SPLIT4:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v2i32.i16.i1(<7 x i32> [[JOIN_HI_SPLIT0]], <2 x i32> [[HI_SPLIT4]], i32 2, i32 2, i32 1, i16 16, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-DAG:  [[JOIN_LO_SPLIT4:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v2i32.i16.i1(<7 x i32> [[JOIN_LO_SPLIT0]], <2 x i32> [[LO_SPLIT4]], i32 2, i32 2, i32 1, i16 16, i32 undef, i1 true)

; CHECK_NO_MUL_DDQ-DAG:  [[OP0_SPLIT6:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %op0, i32 1, i32 1, i32 1, i16 24, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP1_SPLIT6:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %op1, i32 1, i32 1, i32 1, i16 24, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[OP2_SPLIT6:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v7i32.i16(<7 x i32> %op2, i32 1, i32 1, i32 1, i16 24, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[MADW_SPLIT6:%.*]] = call <16 x i32> @llvm.genx.umadw.v16i32.v1i32(<1 x i32> [[OP0_SPLIT6]], <1 x i32> [[OP1_SPLIT6]], <1 x i32> [[OP2_SPLIT6]])

; CHECK_NO_MUL_DDQ-DAG:  [[HI_SPLIT6:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> [[MADW_SPLIT6]], i32 1, i32 1, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[LO_SPLIT6:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> [[MADW_SPLIT6]], i32 1, i32 1, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG:  [[RES_HI:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[JOIN_HI_SPLIT4]], <1 x i32> [[HI_SPLIT6]], i32 1, i32 1, i32 1, i16 24, i32 undef, i1 true)
; CHECK_NO_MUL_DDQ-DAG:  [[RES_LO:%.*]] = call <7 x i32> @llvm.genx.wrregioni.v7i32.v1i32.i16.i1(<7 x i32> [[JOIN_LO_SPLIT4]], <1 x i32> [[LO_SPLIT6:%.*]], i32 1, i32 1, i32 1, i16 24, i32 undef, i1 true)

; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { <7 x i32>, <7 x i32> } undef, <7 x i32> [[RES_HI]], 0
; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { <7 x i32>, <7 x i32> } [[INSERT_VAL_HI]], <7 x i32> [[RES_LO]], 1

; CHECK_NO_MUL_DDQ-DAG: ret { <7 x i32>, <7 x i32> } [[INSERT_VAL_LO]]

  %1 = call { <7 x i32>, <7 x i32> } @llvm.genx.uimad.v7i32(<7 x i32> %op0, <7 x i32> %op1, <7 x i32> %op2)
  %2 = extractvalue { <7 x i32>, <7 x i32> } %1, 0
  %3 = extractvalue { <7 x i32>, <7 x i32> } %1, 1
  %4 = insertvalue { <7 x i32>, <7 x i32> } undef, <7 x i32> %2, 0
  %5 = insertvalue { <7 x i32>, <7 x i32> } %4, <7 x i32> %3, 1
  ret { <7 x i32>, <7 x i32> } %5
}

declare { i32, i32 } @llvm.genx.simad.i32(i32, i32, i32)
; CHECK-LABEL @test_madw_Sz1
define { i32, i32 } @test_simad_Sz1(i32 %op0, i32 %op1, i32 %op2) {
; CHECK_NO_MUL_DDQ-DAG:  [[BITCAST_OP0:%.*]] = bitcast i32 %op0 to <1 x i32>
; CHECK_NO_MUL_DDQ-DAG:  [[BITCAST_OP1:%.*]] = bitcast i32 %op1 to <1 x i32>
; CHECK_NO_MUL_DDQ-DAG:  [[BITCAST_OP2:%.*]] = bitcast i32 %op2 to <1 x i32>

; CHECK_NO_MUL_DDQ-DAG:  [[SMADW:%.*]] = call <16 x i32> @llvm.genx.smadw.v16i32.v1i32(<1 x i32> [[BITCAST_OP0]], <1 x i32> [[BITCAST_OP1]], <1 x i32> [[BITCAST_OP2]])

; CHECK_NO_MUL_DDQ-DAG:  [[HI:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> [[SMADW]], i32 1, i32 1, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[LO:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v16i32.i16(<16 x i32> [[SMADW]], i32 1, i32 1, i32 1, i16 0, i32 undef)

; CHECK_NO_MUL_DDQ-DAG:  [[BITCAST_HI:%.*]] = bitcast <1 x i32> [[HI]] to i32
; CHECK_NO_MUL_DDQ-DAG:  [[BITCAST_LO:%.*]] = bitcast <1 x i32> [[LO]] to i32

; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { i32, i32 } undef, i32 [[BITCAST_HI]], 0
; CHECK_NO_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { i32, i32 } [[INSERT_VAL_HI]], i32 [[BITCAST_LO]], 1

; CHECK_NO_MUL_DDQ-DAG: ret { i32, i32 } [[INSERT_VAL_LO]]

  %1 = call { i32, i32 } @llvm.genx.simad.i32(i32 %op0, i32 %op1, i32 %op2)
  ret { i32, i32 } %1
}

declare void @lo_user(<2 x i32>)
declare void @hi_user(<2 x i32>)

declare { <2 x i32>, <2 x i32> } @llvm.genx.simad.v2i32(<2 x i32>, <2 x i32>, <2 x i32>)
; CHECK-LABEL @test_different_imad_users
define { <2 x i32>, <2 x i32> } @test_different_imad_users(<2 x i32> %op0, <2 x i32> %op1, <2 x i32> %op2) {
; CHECK_NO_MUL_DDQ-DAG:  [[SMADW:%.*]] = call <16 x i32> @llvm.genx.smadw.v16i32.v2i32(<2 x i32> %op0, <2 x i32> %op1, <2 x i32> %op2)
; CHECK_NO_MUL_DDQ-DAG:  [[HI:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[SMADW]], i32 2, i32 2, i32 1, i16 32, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[LO:%.*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v16i32.i16(<16 x i32> [[SMADW]], i32 2, i32 2, i32 1, i16 0, i32 undef)
; CHECK_NO_MUL_DDQ-DAG:  [[INSERT_HI:%.*]] = insertvalue { <2 x i32>, <2 x i32> } undef, <2 x i32> [[HI]], 0
; CHECK_NO_MUL_DDQ-DAG:  [[INSERT_LO:%.*]] = insertvalue { <2 x i32>, <2 x i32> } [[INSERT_HI]], <2 x i32> [[LO]], 1
; CHECK_NO_MUL_DDQ-DAG:  call void @lo_user(<2 x i32> [[LO]])
; CHECK_NO_MUL_DDQ-DAG:  call void @hi_user(<2 x i32> [[HI]])
; CHECK_NO_MUL_DDQ-DAG:  ret { <2 x i32>, <2 x i32> } [[INSERT_LO]]

  %1 = call { <2 x i32>, <2 x i32> } @llvm.genx.simad.v2i32(<2 x i32> %op0, <2 x i32> %op1,  <2 x i32> %op2)
  %lo = extractvalue { <2 x i32>, <2 x i32> } %1, 1
  %hi = extractvalue { <2 x i32>, <2 x i32> } %1, 0
  call void @lo_user(<2 x i32> %lo)
  call void @hi_user(<2 x i32> %hi)
  ret { <2 x i32>, <2 x i32> } %1
}

; CHECK-LABEL @test_convert_uimad_to_mul64_and_add_Sz16
define { <16 x i32>, <16 x i32> } @test_convert_uimad_to_mul64_and_add_Sz16(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2) {
; CHECK_WITH_MUL_DDQ-DAG:  [[MUL64:%.*]] = call <16 x i64> @llvm.genx.uumul.v16i64.v16i32(<16 x i32> %op0, <16 x i32> %op1)
; CHECK_WITH_MUL_DDQ-DAG:  [[ZEXT:%.*]]  = zext <16 x i32> %op2 to <16 x i64>
; CHECK_WITH_MUL_DDQ-DAG:  [[ADD64:%.*]] = add <16 x i64> [[MUL64]], [[ZEXT:%.*]]

; CHECK_WITH_MUL_DDQ-DAG:  [[RES_BITCAST:%.*]] = bitcast <16 x i64> [[ADD64]] to <32 x i32>
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_HI:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[RES_BITCAST]], i32 0, i32 16, i32 2, i16 4, i32 undef)
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_LO:%.*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> [[RES_BITCAST]], i32 0, i32 16, i32 2, i16 0, i32 undef)

; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { <16 x i32>, <16 x i32> } undef, <16 x i32> [[RES_HI]], 0
; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { <16 x i32>, <16 x i32> } [[INSERT_VAL_HI]], <16 x i32> [[RES_LO]], 1

; CHECK_WITH_MUL_DDQ-DAG: ret { <16 x i32>, <16 x i32> } [[INSERT_VAL_LO:%.*]]

  %1 = call { <16 x i32>, <16 x i32> } @llvm.genx.uimad.v16i32(<16 x i32> %op0, <16 x i32> %op1, <16 x i32> %op2)
  %lo = extractvalue { <16 x i32>, <16 x i32> } %1, 1
  %hi = extractvalue { <16 x i32>, <16 x i32> } %1, 0
  %2 = insertvalue { <16 x i32>, <16 x i32> } undef, <16 x i32> %hi, 0
  %3 = insertvalue { <16 x i32>, <16 x i32> } %2, <16 x i32> %lo, 1
  ret { <16 x i32>, <16 x i32> } %3
}

declare { <8 x i32>, <8 x i32> } @llvm.genx.simad.v8i32(<8 x i32>, <8 x i32>, <8 x i32>)
; CHECK-LABEL @test_convert_simad_to_mul64_and_add_Sz8
define { <8 x i32>, <8 x i32> } @test_convert_simad_to_mul64_and_add_Sz8(<8 x i32> %op0, <8 x i32> %op1) {
; CHECK_WITH_MUL_DDQ-DAG:  [[MUL64:%.*]] = call <8 x i64> @llvm.genx.ssmul.v8i64.v8i32(<8 x i32> %op0, <8 x i32> %op1)
; CHECK_WITH_MUL_DDQ-DAG:  [[SEXT:%.*]]  = sext <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1> to <8 x i64>
; CHECK_WITH_MUL_DDQ-DAG:  [[ADD64:%.*]] = add <8 x i64> [[MUL64]], [[SEXT]]

; CHECK_WITH_MUL_DDQ-DAG:  [[RES_BITCAST:%.*]] = bitcast <8 x i64> [[ADD64]] to <16 x i32>
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_HI:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[RES_BITCAST]], i32 0, i32 8, i32 2, i16 4, i32 undef)
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_LO:%.*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> [[RES_BITCAST]], i32 0, i32 8, i32 2, i16 0, i32 undef)

; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { <8 x i32>, <8 x i32> } undef, <8 x i32> [[RES_HI]], 0
; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { <8 x i32>, <8 x i32> } [[INSERT_VAL_HI]], <8 x i32> [[RES_LO]], 1

; CHECK_WITH_MUL_DDQ-DAG: ret { <8 x i32>, <8 x i32> } [[INSERT_VAL_LO:%.*]]

  %1 = call { <8 x i32>, <8 x i32> } @llvm.genx.simad.v8i32(<8 x i32> %op0, <8 x i32> %op1, <8 x i32> <i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1>)
  %lo = extractvalue { <8 x i32>, <8 x i32> } %1, 1
  %hi = extractvalue { <8 x i32>, <8 x i32> } %1, 0
  %2 = insertvalue { <8 x i32>, <8 x i32> } undef, <8 x i32> %hi, 0
  %3 = insertvalue { <8 x i32>, <8 x i32> } %2, <8 x i32> %lo, 1
  ret { <8 x i32>, <8 x i32> } %3
}

declare { i32, i32 } @llvm.genx.simad.vi32(i32, i32, i32)
; CHECK-LABEL @test_convert_simad_to_mul64_i32
define { i32, i32 } @test_convert_simad_to_mul64_i32( i32 %op0, i32 %op1 ) {
; CHECK_WITH_MUL_DDQ-DAG:  [[BITCAST_OP0:%.*]] = bitcast i32 %op0 to <1 x i32>
; CHECK_WITH_MUL_DDQ-DAG:  [[BITCAST_OP1:%.*]] = bitcast i32 %op1 to <1 x i32>

; CHECK_WITH_MUL_DDQ-DAG:  [[MUL64:%.*]] = call <1 x i64> @llvm.genx.ssmul.v1i64.v1i32(<1 x i32> [[BITCAST_OP0]], <1 x i32> [[BITCAST_OP1]])

; CHECK_WITH_MUL_DDQ-DAG:  [[RES_BITCAST:%.*]] = bitcast <1 x i64> [[MUL64:%.*]] to <2 x i32>
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_HI:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[RES_BITCAST]], i32 0, i32 1, i32 2, i16 4, i32 undef)
; CHECK_WITH_MUL_DDQ-DAG:  [[RES_LO:%.*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> [[RES_BITCAST]], i32 0, i32 1, i32 2, i16 0, i32 undef)

; CHECK_WITH_MUL_DDQ-DAG:  [[BITCAST_HI:%.*]] = bitcast <1 x i32> [[RES_HI]] to i32
; CHECK_WITH_MUL_DDQ-DAG:  [[BITCAST_LO:%.*]] = bitcast <1 x i32> [[RES_LO]] to i32

; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_HI:%.*]] = insertvalue { i32, i32 } undef, i32 [[BITCAST_HI]], 0
; CHECK_WITH_MUL_DDQ-DAG: [[INSERT_VAL_LO:%.*]] = insertvalue { i32, i32 } [[INSERT_VAL_HI]], i32 [[BITCAST_LO]], 1

; CHECK_WITH_MUL_DDQ-DAG: ret { i32, i32 } [[INSERT_VAL_LO:%.*]]

  %1 = call { i32, i32 } @llvm.genx.simad.vi32(i32 %op0, i32 %op1, i32 zeroinitializer)
  %lo = extractvalue { i32, i32 } %1, 1
  %hi = extractvalue { i32, i32 } %1, 0
  %2 = insertvalue { i32, i32 } undef, i32 %hi, 0
  %3 = insertvalue { i32, i32 } %2, i32 %lo, 1
  ret { i32, i32 } %3
}
