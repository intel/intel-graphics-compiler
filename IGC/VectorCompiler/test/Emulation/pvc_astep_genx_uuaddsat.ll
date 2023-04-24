;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; Function Attrs: nounwind readnone
declare <2 x i64> @llvm.genx.uuadd.sat.v2i64(<2 x i64>, <2 x i64>)

; COM: these tests just check that there is no compilation/asserts failures
; CHECK: @test_uuaddsat
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT]]> %vop to <[[CT]]>
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

; CHECK-NEXT: [[SAT:%[^ ]+]] = or <[[ET]]> [[HiADDC_1_CARRY]], [[HiADDC_2_CARRY]]
; CHECK-NEXT: [[CMP_SAT:%[^ ]+]] = icmp ne <[[ET]]> [[SAT]], zeroinitializer
; CHECK-NEXT: [[Lo:%[^ ]+]] = select <2 x i1> [[CMP_SAT]], <2 x i32> <i32 -1, i32 -1>, <[[ET]]> [[LoADDC_ADD]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = select <2 x i1> [[CMP_SAT]], <2 x i32> <i32 -1, i32 -1>, <[[ET]]> [[HiADDC_2_ADD]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>

define dllexport spir_kernel void @test_uuaddsat(i64 %sop, <2 x i64> %vop) {
  %val = call <2 x i64> @llvm.genx.uuadd.sat.v2i64(<2 x i64> %vop, <2 x i64> %vop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
