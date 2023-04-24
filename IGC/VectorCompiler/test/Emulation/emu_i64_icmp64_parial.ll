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

declare <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1>, <8 x i1>, i32)

; CHECK: @test_icmp_eq_partial
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Sel1:%[^ ]+]] = select <8 x i1> [[Slo]], <8 x i32> <[[ONES:i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1]]>, <8 x i32> zeroinitializer
; CHECK-NEXT: [[Sel2:%[^ ]+]] = select <8 x i1> [[Shi]], <8 x i32> <[[ONES]]>, <8 x i32> zeroinitializer
; CHECK-NEXT: [[Res:%[^ ]+]]  = and <8 x i32> [[Sel1]], [[Sel2]]
; CHECK-NEXT: [[PRED:%[^ ]+]]  = icmp eq <8 x i32> [[Res]], <[[ONES]]>
; COM: just check that the result is used
; CHECK: %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> [[PRED]], i32 8)

define dllexport spir_kernel void @test_icmp_eq_partial(<8 x i64> %left, <8 x i64> %right) {
  %p1   = icmp eq <8 x i64> %left, %right
  %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> %p1, i32 8)
  ret void
}

; CHECK: @test_icmp_ne_partial
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %right to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp ne <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp ne <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Sel1:%[^ ]+]] = select <8 x i1> [[Slo]], <8 x i32> <[[ONES:i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1]]>, <8 x i32> zeroinitializer
; CHECK-NEXT: [[Sel2:%[^ ]+]] = select <8 x i1> [[Shi]], <8 x i32> <[[ONES]]>, <8 x i32> zeroinitializer
; CHECK-NEXT: [[Res:%[^ ]+]]  = or <8 x i32> [[Sel1]], [[Sel2]]
; CHECK-NEXT: [[PRED:%[^ ]+]]  = icmp eq <8 x i32> [[Res]], <[[ONES]]>
; COM: just check that the result is used
; CHECK: %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> [[PRED]], i32 8)

define dllexport spir_kernel void @test_icmp_ne_partial(<8 x i64> %left, <8 x i64> %right) {
  %p1   = icmp ne <8 x i64> %left, %right
  %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> %p1, i32 8)
  ret void
}

; CHECK: @test_icmp_eq_partial_constant
; CHECK: [[IV1:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> %left to <[[CT:16 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^.]+.LoSplit[0-9]*]] = call <[[ET:8 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 8, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 8, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^.]+.iv32cast[0-9]*]] = bitcast <8 x i64> <i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8> to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^.]+.LoSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^.]+.HiSplit[0-9]*]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Sel1:%[^ ]+]] = select <8 x i1> [[Slo]], <[[ET]]> <[[ONES:i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1]]>, <[[ET]]> zeroinitializer
; CHECK-NEXT: [[Sel2:%[^ ]+]] = select <8 x i1> [[Shi]], <[[ET]]> <[[ONES]]>, <[[ET]]> zeroinitializer
; CHECK-NEXT: [[Res:%[^ ]+]]  = and <[[ET]]> [[Sel1]], [[Sel2]]
; CHECK-NEXT: [[PRED:%[^ ]+]]  = icmp eq <[[ET]]> [[Res]], <[[ONES]]>
; COM: just check that the result is used
; CHECK: %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> [[PRED]], i32 8)

define dllexport spir_kernel void @test_icmp_eq_partial_constant(<8 x i64> %left) {
  %p1   = icmp eq <8 x i64> %left, <i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8>
  %part = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> undef, <8 x i1> %p1, i32 8)
  ret void
}

