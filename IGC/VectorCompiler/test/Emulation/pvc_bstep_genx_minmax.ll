;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -mattr=+lightweight_i64_emulation -S < %s | FileCheck %s

; COM: these tests just check that there is no compilation/asserts failures

declare <2 x i64> @llvm.genx.umin.v2i64.v2i64(<2 x i64>, <2 x i64>)
declare <2 x i64> @llvm.genx.smin.v2i64.v2i64(<2 x i64>, <2 x i64>)

declare <2 x i64> @llvm.genx.umax.v2i64.v2i64(<2 x i64>, <2 x i64>)
declare <2 x i64> @llvm.genx.smax.v2i64.v2i64(<2 x i64>, <2 x i64>)

; CHECK: @test_genx_minU
; CHECK: [[IV1:%[^ ]+]] = bitcast <[[RT:2 x i64]]> %left to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ult <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <[[BV:2 x i1]]> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ult <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <[[BV]]> [[T2]], [[T3]]

; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <[[RT]]> %left to <[[CT]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[low_reg]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[Rlo:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]]
; CHECK-NEXT: [[Rhi:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Rlo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Rhi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>
define dllexport spir_kernel void @test_genx_minU(<2 x i64> %left, <2 x i64> %right) {
  %val = call <2 x i64> @llvm.genx.umin.v2i64.v2i64(<2 x i64> %left, <2 x i64> %right)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
; CHECK: @test_genx_minS
; CHECK: [[IV1:%[^ ]+]] = bitcast <[[RT:2 x i64]]> %left to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ult <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <[[BV:2 x i1]]> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp slt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <[[BV]]> [[T2]], [[T3]]

; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <[[RT]]> %left to <[[CT]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[low_reg]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[Rlo:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]]
; CHECK-NEXT: [[Rhi:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Rlo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Rhi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>
define dllexport spir_kernel void @test_genx_minS(<2 x i64> %left, <2 x i64> %right) {
  %val = call <2 x i64> @llvm.genx.smin.v2i64.v2i64(<2 x i64> %left, <2 x i64> %right)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
; CHECK: @test_genx_maxS
; CHECK: [[IV1:%[^ ]+]] = bitcast <[[RT:2 x i64]]> %left to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ugt <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <[[BV:2 x i1]]> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp sgt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <[[BV]]> [[T2]], [[T3]]

; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <[[RT]]> %left to <[[CT]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[low_reg]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[Rlo:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]]
; CHECK-NEXT: [[Rhi:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Rlo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Rhi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>
define dllexport spir_kernel void @test_genx_maxS(<2 x i64> %left, <2 x i64> %right) {
  %val = call <2 x i64> @llvm.genx.smax.v2i64.v2i64(<2 x i64> %left, <2 x i64> %right)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
; CHECK: @test_genx_maxU
; CHECK: [[IV1:%[^ ]+]] = bitcast <[[RT:2 x i64]]> %left to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[T0:%[^ ]+]] = icmp ugt <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[T1:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[T2:%[^ ]+]] = and <[[BV:2 x i1]]> [[T1]], [[T0]]
; CHECK-NEXT: [[T3:%[^ ]+]] = icmp ugt <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = or <[[BV]]> [[T2]], [[T3]]

; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <[[RT]]> %left to <[[CT]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[low_reg]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <[[RT]]> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]

; CHECK-NEXT: [[Rlo:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Lo_l]], <[[ET]]> [[Lo_r]]
; CHECK-NEXT: [[Rhi:%[^ ]+]] = select <[[BV]]> [[Res]], <[[ET]]> [[Hi_l]], <[[ET]]> [[Hi_r]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[Rlo]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[Rhi]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>
define dllexport spir_kernel void @test_genx_maxU(<2 x i64> %left, <2 x i64> %right) {
  %val = call <2 x i64> @llvm.genx.umax.v2i64.v2i64(<2 x i64> %left, <2 x i64> %right)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
