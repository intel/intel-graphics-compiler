;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; Function Attrs: nounwind readnone
declare <2 x i64> @llvm.genx.ssadd.sat.v2i64(<2 x i64>, <2 x i64>)
declare i64 @llvm.genx.ssadd.sat.i64(i64, i64)

declare <2 x i16> @llvm.genx.ssadd.sat.v2i16.v2i64(<2 x i64>, <2 x i64>)
declare i16 @llvm.genx.ssadd.sat.i16.i64(i64, i64)

; COM: these tests just check that there is no compilation/asserts failures
; CHECK-LABEL: @test_ssaddsat_vi64
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT:2 x i64]]> %lvop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT]]> %rvop to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[LoADDC:%[^ ]+]] = call [[BR:[^@]+]] @llvm.genx.addc.[[TT:v2i32.v2i32]](<[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]])
; CHECK-NEXT: [[LoADDC_ADD:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxAdd:1]]
; CHECK-NEXT: [[LoADDC_CARRY:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxCarry:0]]

; CHECK-NEXT: [[HiADDC_1:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]])
; CHECK-NEXT: [[HiADDC_1_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_1_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxCarry]]
; CHECK-NEXT: [[HiADDC_2:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[HiADDC_1_ADD]], <[[ET]]> [[LoADDC_CARRY]])
; CHECK-NEXT: [[HiADDC_2_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_2_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxCarry]]

; CHECK-NEXT: [[SIGN_SRC0:%[^ ]+]] = and <[[ET]]> [[Hi_l]], <[[MASK_BIT31:[^>]+]]>
; CHECK-NEXT: [[SIGN_SRC1:%[^ ]+]] = and <[[ET]]> [[Hi_r]], <[[MASK_BIT31]]>
; CHECK-NEXT: [[SIGN_RES:%[^ ]+]] = and <[[ET]]> [[HiADDC_2_ADD]], <[[MASK_BIT31]]>

; CHECK-NEXT: [[F_SIGN_OP_MATCH:%[^ ]+]] = icmp eq <[[ET]]> [[SIGN_SRC0]], [[SIGN_SRC1]]
; CHECK-NEXT: [[F_SIGN_RES_MISS:%[^ ]+]] = icmp ne <[[ET]]> [[SIGN_SRC0]], [[SIGN_RES]]
; CHECK-NEXT: [[F_OVERFLOW:%[^ ]+]] = and <[[BL:2 x i1]]> [[F_SIGN_OP_MATCH]], [[F_SIGN_RES_MISS]]

; CHECK-NEXT: [[LO1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[ONES:i32 -1, i32 -1]]>, <[[ET]]> [[LoADDC_ADD]]
; CHECK-NEXT: [[HI1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[P_MASK:[^>]+]]>, <[[ET]]> [[HiADDC_2_ADD]]

; CHECK-NEXT: [[F_IS_OP_NEGATIVE:%[^ ]+]] = icmp slt <[[ET]]> [[SIGN_SRC0]], [[ZERO:zeroinitializer]]
; CHECK-NEXT: [[F_NEG_OVERFLOW:%[^ ]+]] = and <[[BL]]> [[F_OVERFLOW]], [[F_IS_OP_NEGATIVE]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> [[ZERO]], <[[ET]]> [[LO1]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> <[[MASK_BIT31]]>, <[[ET]]> [[HI1]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>
; CHECK-NEXT: ret void

define dllexport spir_kernel void @test_ssaddsat_vi64(<2 x i64> %lvop, <2 x i64> %rvop) {
  %val = call <2 x i64> @llvm.genx.ssadd.sat.v2i64(<2 x i64> %lvop, <2 x i64> %rvop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}

; CHECK-LABEL: @test_ssaddsat_si64
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast [[RT:i64]] %lsop to <[[CT:2 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast [[RT]] %rsop to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[LoADDC:%[^ ]+]] = call [[BR:[^@]+]] @llvm.genx.addc.[[TT:v1i32.v1i32]](<[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]])
; CHECK-NEXT: [[LoADDC_ADD:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxAdd:1]]
; CHECK-NEXT: [[LoADDC_CARRY:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxCarry:0]]

; CHECK-NEXT: [[HiADDC_1:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]])
; CHECK-NEXT: [[HiADDC_1_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_1_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxCarry]]
; CHECK-NEXT: [[HiADDC_2:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[HiADDC_1_ADD]], <[[ET]]> [[LoADDC_CARRY]])
; CHECK-NEXT: [[HiADDC_2_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_2_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxCarry]]

; CHECK-NEXT: [[SIGN_SRC0:%[^ ]+]] = and <[[ET]]> [[Hi_l]], <[[MASK_BIT31:[^>]+]]>
; CHECK-NEXT: [[SIGN_SRC1:%[^ ]+]] = and <[[ET]]> [[Hi_r]], <[[MASK_BIT31]]>
; CHECK-NEXT: [[SIGN_RES:%[^ ]+]] = and <[[ET]]> [[HiADDC_2_ADD]], <[[MASK_BIT31]]>

; CHECK-NEXT: [[F_SIGN_OP_MATCH:%[^ ]+]] = icmp eq <[[ET]]> [[SIGN_SRC0]], [[SIGN_SRC1]]
; CHECK-NEXT: [[F_SIGN_RES_MISS:%[^ ]+]] = icmp ne <[[ET]]> [[SIGN_SRC0]], [[SIGN_RES]]
; CHECK-NEXT: [[F_OVERFLOW:%[^ ]+]] = and <[[BL:1 x i1]]> [[F_SIGN_OP_MATCH]], [[F_SIGN_RES_MISS]]

; CHECK-NEXT: [[LO1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[ONES:i32 -1]]>, <[[ET]]> [[LoADDC_ADD]]
; CHECK-NEXT: [[HI1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[P_MASK:[^>]+]]>, <[[ET]]> [[HiADDC_2_ADD]]

; CHECK-NEXT: [[F_IS_OP_NEGATIVE:%[^ ]+]] = icmp slt <[[ET]]> [[SIGN_SRC0]], [[ZERO:zeroinitializer]]
; CHECK-NEXT: [[F_NEG_OVERFLOW:%[^ ]+]] = and <[[BL]]> [[F_OVERFLOW]], [[F_IS_OP_NEGATIVE]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> [[ZERO]], <[[ET]]> [[LO1]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> <[[MASK_BIT31]]>, <[[ET]]> [[HI1]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <1 x i64>
; CHECK-NEXT: [[SRECAST:%[^ ]+]] = bitcast <1 x i64> [[RECAST]] to [[RT]]
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast [[RT]] [[SRECAST]] to <1 x i64>
; CHECK-NEXT: ret void

define dllexport spir_kernel void @test_ssaddsat_si64(i64 %lsop, i64 %rsop) {
  %val = call i64 @llvm.genx.ssadd.sat.i64(i64 %lsop, i64 %rsop)
  %vu = bitcast i64 %val to <1 x i64>
  ret void
}

; CHECK-LABEL: @test_ssaddsat_vi16
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT:2 x i64]]> %lvop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT]]> %rvop to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[LoADDC:%[^ ]+]] = call [[BR:[^@]+]] @llvm.genx.addc.[[TT:v2i32.v2i32]](<[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]])
; CHECK-NEXT: [[LoADDC_ADD:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxAdd:1]]
; CHECK-NEXT: [[LoADDC_CARRY:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxCarry:0]]

; CHECK-NEXT: [[HiADDC_1:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]])
; CHECK-NEXT: [[HiADDC_1_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_1_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxCarry]]
; CHECK-NEXT: [[HiADDC_2:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[HiADDC_1_ADD]], <[[ET]]> [[LoADDC_CARRY]])
; CHECK-NEXT: [[HiADDC_2_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_2_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxCarry]]

; CHECK-NEXT: [[SIGN_SRC0:%[^ ]+]] = and <[[ET]]> [[Hi_l]], <[[MASK_BIT31:[^>]+]]>
; CHECK-NEXT: [[SIGN_SRC1:%[^ ]+]] = and <[[ET]]> [[Hi_r]], <[[MASK_BIT31]]>
; CHECK-NEXT: [[SIGN_RES:%[^ ]+]] = and <[[ET]]> [[HiADDC_2_ADD]], <[[MASK_BIT31]]>

; CHECK-NEXT: [[F_SIGN_OP_MATCH:%[^ ]+]] = icmp eq <[[ET]]> [[SIGN_SRC0]], [[SIGN_SRC1]]
; CHECK-NEXT: [[F_SIGN_RES_MISS:%[^ ]+]] = icmp ne <[[ET]]> [[SIGN_SRC0]], [[SIGN_RES]]
; CHECK-NEXT: [[F_OVERFLOW:%[^ ]+]] = and <[[BL:2 x i1]]> [[F_SIGN_OP_MATCH]], [[F_SIGN_RES_MISS]]

; CHECK-NEXT: [[LO1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[ONES:i32 -1, i32 -1]]>, <[[ET]]> [[LoADDC_ADD]]
; CHECK-NEXT: [[HI1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[P_MASK:[^>]+]]>, <[[ET]]> [[HiADDC_2_ADD]]

; CHECK-NEXT: [[F_IS_OP_NEGATIVE:%[^ ]+]] = icmp slt <[[ET]]> [[SIGN_SRC0]], [[ZERO:zeroinitializer]]
; CHECK-NEXT: [[F_NEG_OVERFLOW:%[^ ]+]] = and <[[BL]]> [[F_OVERFLOW]], [[F_IS_OP_NEGATIVE]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> [[ZERO]], <[[ET]]> [[LO1]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> <[[MASK_BIT31]]>, <[[ET]]> [[HI1]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; COM: here emulation of i64->i16 sstrunc.sat, we don't have rigous checks for these
; CHECK-NEXT: [[CAST_PREPARE:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[CT]]>
; COM: ...
; CHECK: [[SATURATE_I32:%[^ ]+]] = call <2 x i16> @llvm.genx.sstrunc.sat.v2i16.v2i32(<2 x i32> {{[^)]+}})
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <2 x i16> [[SATURATE_I32]] to <2 x i16>
; CHECK-NEXT: ret void

define dllexport spir_kernel void @test_ssaddsat_vi16(<2 x i64> %lvop, <2 x i64> %rvop) {
  %val = call <2 x i16> @llvm.genx.ssadd.sat.v2i16.v2i64(<2 x i64> %lvop, <2 x i64> %rvop)
  %vu = bitcast <2 x i16> %val to <2 x i16>
  ret void
}

; CHECK-LABEL: @test_ssaddsat_si16
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast [[RT:i64]] %lsop to <[[CT:2 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast [[RT]] %rsop to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[LoADDC:%[^ ]+]] = call [[BR:[^@]+]] @llvm.genx.addc.[[TT:v1i32.v1i32]](<[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]])
; CHECK-NEXT: [[LoADDC_ADD:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxAdd:1]]
; CHECK-NEXT: [[LoADDC_CARRY:[^ ]+]] = extractvalue [[BR]] [[LoADDC]], [[IdxCarry:0]]

; CHECK-NEXT: [[HiADDC_1:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]])
; CHECK-NEXT: [[HiADDC_1_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_1_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_1]], [[IdxCarry]]
; CHECK-NEXT: [[HiADDC_2:%[^ ]+]] = call [[BR]] @llvm.genx.addc.[[TT]](<[[ET]]> [[HiADDC_1_ADD]], <[[ET]]> [[LoADDC_CARRY]])
; CHECK-NEXT: [[HiADDC_2_ADD:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxAdd]]
; CHECK-NEXT: [[HiADDC_2_CARRY:%[^ ]+]] = extractvalue [[BR]] [[HiADDC_2]], [[IdxCarry]]

; CHECK-NEXT: [[SIGN_SRC0:%[^ ]+]] = and <[[ET]]> [[Hi_l]], <[[MASK_BIT31:[^>]+]]>
; CHECK-NEXT: [[SIGN_SRC1:%[^ ]+]] = and <[[ET]]> [[Hi_r]], <[[MASK_BIT31]]>
; CHECK-NEXT: [[SIGN_RES:%[^ ]+]] = and <[[ET]]> [[HiADDC_2_ADD]], <[[MASK_BIT31]]>

; CHECK-NEXT: [[F_SIGN_OP_MATCH:%[^ ]+]] = icmp eq <[[ET]]> [[SIGN_SRC0]], [[SIGN_SRC1]]
; CHECK-NEXT: [[F_SIGN_RES_MISS:%[^ ]+]] = icmp ne <[[ET]]> [[SIGN_SRC0]], [[SIGN_RES]]
; CHECK-NEXT: [[F_OVERFLOW:%[^ ]+]] = and <[[BL:1 x i1]]> [[F_SIGN_OP_MATCH]], [[F_SIGN_RES_MISS]]

; CHECK-NEXT: [[LO1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[ONES:i32 -1]]>, <[[ET]]> [[LoADDC_ADD]]
; CHECK-NEXT: [[HI1:%[^ ]+]] = select <[[BL]]> [[F_OVERFLOW]], <[[ET]]> <[[P_MASK:[^>]+]]>, <[[ET]]> [[HiADDC_2_ADD]]

; CHECK-NEXT: [[F_IS_OP_NEGATIVE:%[^ ]+]] = icmp slt <[[ET]]> [[SIGN_SRC0]], [[ZERO:zeroinitializer]]
; CHECK-NEXT: [[F_NEG_OVERFLOW:%[^ ]+]] = and <[[BL]]> [[F_OVERFLOW]], [[F_IS_OP_NEGATIVE]]
; CHECK-NEXT: [[Lo:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> [[ZERO]], <[[ET]]> [[LO1]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = select <[[BL]]> [[F_NEG_OVERFLOW]], <[[ET]]> <[[MASK_BIT31]]>, <[[ET]]> [[HI1]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <1 x i64>
; CHECK-NEXT: [[SRECAST:%[^ ]+]] = bitcast <1 x i64> [[RECAST]] to [[RT]]
; CHECK-NEXT: [[CAST_PREPARE_SCALAR:%[^ ]+]] = bitcast [[RT]] [[SRECAST]] to <1 x i64>
; CHECK-NEXT: [[CAST_PREPARE_VECTOR:%[^ ]+]] = bitcast <1 x i64> [[CAST_PREPARE_SCALAR]] to <[[CT]]>
; COM: ...
; CHECK:  [[SATURATE_I32:%[^ ]+]] = call <1 x i16> @llvm.genx.sstrunc.sat.v1i16.v1i32(<1 x i32> {{[^)]+}})
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <1 x i16> [[SATURATE_I32]] to i16
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast i16 [[RECAST]] to <1 x i16>
; CHECK-NEXT: ret void
define dllexport spir_kernel void @test_ssaddsat_si16(i64 %lsop, i64 %rsop) {
  %val = call i16 @llvm.genx.ssadd.sat.i16.i64(i64 %lsop, i64 %rsop)
  %vu = bitcast i16 %val to <1 x i16>
  ret void
}
