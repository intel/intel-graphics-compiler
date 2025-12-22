;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXEmulate -march=genx64 -mtriple=spir64-unknown-unknown \
; RUN: -mcpu=XeHPC -mattr=+lightweight_i64_emulation -S < %s | FileCheck %s

; COM: "CT" stands for "casted type"
; COM: "ET" valid type (the type by which we emulate an operation)

; Function Attrs: nounwind readnone
declare <2 x i64> @llvm.genx.absi.v2i64(<2 x i64>)

; COM: these tests just check that there is no compilation/asserts failures
; CHECK: @test_genx_absi
; CHECK: [[IV1:%[^ ]+]] = bitcast <[[RT:2 x i64]]> %vop to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[Sub_IV2:%[^ ]+]] = bitcast <[[RT]]> %vop to <[[CT]]>
; CHECK-NEXT: [[Sub_Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[Sub_IV2]], [[low_reg]]
; CHECK-NEXT: [[Sub_Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[Sub_IV2]], [[high_reg]]

; CHECK-NEXT: [[SUBB:%[^ ]+]] = call { <[[ET]]>, <[[ET]]> } @llvm.genx.subb.{{[^(]+}}(<[[ET]]> zeroinitializer, <[[ET]]> [[Sub_Lo_r]])
; CHECK-NEXT: [[SUBB_SUB:%[^ ]+]] = extractvalue { <[[ET]]>, <[[ET]]> } [[SUBB]], 1
; CHECK-NEXT: [[SUBB_BORROW:%[^ ]+]] = extractvalue { <[[ET]]>, <[[ET]]> } [[SUBB]], 0
; CHECK-NEXT: [[BORROW_NEGATE:%[^ ]+]] = sub <[[ET]]> zeroinitializer, [[SUBB_BORROW]]
; CHECK-NEXT: [[Sub_Hi:%[^ ]+]] = sub <[[ET]]> [[BORROW_NEGATE]], [[Sub_Hi_r]]

; CHECK: [[CMP:%[^ ]+]] = icmp slt <[[ET]]> [[Hi_l]], zeroinitializer
; CHECK-NEXT: [[SEL_LO:%[^ ]+]] = select <2 x i1> [[CMP]], <[[ET]]> [[SUBB_SUB]], <[[ET]]> [[Lo_l]]
; CHECK-NEXT: [[SEL_HI:%[^ ]+]] = select <2 x i1> [[CMP]], <[[ET]]> [[Sub_Hi]], <[[ET]]> [[Hi_l]]

; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[SEL_LO]], [[low_reg]]
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[SEL_HI]], [[high_reg]]
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[RT]]>
; CHECK-NEXT: [[USER:%[^ ]+]] = bitcast <[[RT]]> [[RECAST]] to <[[RT]]>

define dllexport spir_kernel void @test_genx_absi(i64 %sop, <2 x i64> %vop) {
  %val = call <2 x i64> @llvm.genx.absi.v2i64(<2 x i64> %vop)
  %vu = bitcast <2 x i64> %val to <2 x i64>
  ret void
}
