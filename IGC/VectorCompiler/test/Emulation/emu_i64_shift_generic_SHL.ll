;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unkonwn-unknown \
; RUN: -mcpu=Gen9 -mattr=+emulate_i64 -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; COM: NOTE: Since general cases for shifts are relatively complex - there is
; COM: little to befit to cover the exact expansion. The functionality should
; COM: tested by a proper EtoE tests. However, an "optimized" routes must
; COM: be covered

; COM: ===============================
; COM:      TEST:  shl
; COM: ===============================

; CHECK: @test_shl_generic
; CHECK: [[IV1:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+.LoSplit[0-9]*]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+.iv32cast[0-9]*]] = bitcast <[[RT]]> %sha to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[SHA:%[^ ]+]] = and <[[ET]]> [[Lo_r]], <i32 63, i32 63>
; CHECK-NEXT: [[SH32:%[^ ]+]] = sub <[[ET]]> <i32 32, i32 32>, [[SHA]]
; CHECK-NEXT: [[CND_MASK1:%[^ ]+]] = icmp uge <[[ET]]> [[SHA]], <i32 32, i32 32>
; CHECK-NEXT: [[CND_MASK0:%[^ ]+]] = icmp eq <[[ET]]> [[SHA]], zeroinitializer
; CHECK-NEXT: [[MASK1:%[^ ]+]] = select <2 x i1> %1, <[[ET]]> zeroinitializer, <[[ET]]> <i32 -1, i32 -1>
; CHECK-NEXT: [[MASK0:%[^ ]+]] = select <2 x i1> %2, <[[ET]]> zeroinitializer, <[[ET]]> <i32 -1, i32 -1>

; CHECK-NEXT: [[Lo_:%[^ ]+]] = shl <[[ET]]> [[Lo_l]], [[SHA]]
; CHECK-NEXT: [[Lo:%[^ ]+]]  = and <[[ET]]> [[Lo_]], [[MASK1]]
; CHECK-NEXT: [[NSH32:%[^ ]+]] = sub <[[ET]]> zeroinitializer, [[SH32]]
; CHECK-NEXT: [[Hl1_:%[^ ]+]] = shl <[[ET]]> [[Lo_l]], [[NSH32]]
; CHECK-NEXT: [[NMASK1:%[^ ]+]] = xor <[[ET]]> [[MASK1]], <i32 -1, i32 -1>
; CHECK-NEXT: [[Hl1:%[^ ]+]] = and <[[ET]]> [[Hl1_]], [[NMASK1]]
; CHECK-NEXT: [[TMP:%[^ ]+]] = and <[[ET]]> [[Lo_l]], [[MASK0]]
; CHECK-NEXT: [[Hl2_:%[^ ]+]] = lshr <[[ET]]> [[TMP]], [[SH32]]
; CHECK-NEXT: [[Hl2:%[^ ]+]] = and  <[[ET]]> [[Hl2_]], [[MASK1]]
; CHECK-NEXT: [[Hh_:%[^ ]+]] = shl <[[ET]]> [[Hi_l]], [[SHA]]
; CHECK-NEXT: [[Hh:%[^ ]+]] = and <[[ET]]> [[Hh_]], [[MASK1]]
; CHECK-NEXT: [[Hi_:%[^ ]+]] = or  <[[ET]]> [[Hl1]], [[Hl2]]
; CHECK-NEXT: [[Hi:%[^ ]+]] = or  <[[ET]]> [[Hh]], [[Hi_]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Lo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Hi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>

define dllexport spir_kernel void @test_shl_generic(i64 %sop1, i64 %sop2,
                                                    <2 x i64> %vop, <2 x i64> %sha) {
  %v1 = shl <2 x i64> %vop, %sha
  %uv = bitcast <2 x i64> %v1 to <2 x i64>
  ret void
}
