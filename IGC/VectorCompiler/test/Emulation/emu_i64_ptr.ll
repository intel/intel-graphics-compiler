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

; CHECK: @test_scalar_icmp_ptr_eq
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint %struct_Type* %left to i64
; CHECK-NEXT: [[PTRCAST_R:%[^ ]+]] = ptrtoint %struct_Type* %right to i64
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast i64 [[PTRCAST_L]] to <[[CT:2 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast i64 [[PTRCAST_R]] to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = and <1 x i1> [[Slo]], [[Shi]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <1 x i1> [[Res]] to i1
%struct_Type = type { i32, i64, i16, i8 }

define i1 @test_scalar_icmp_ptr_eq(%struct_Type* %left, %struct_Type* %right) {
  %cmp_res = icmp eq %struct_Type* %left, %right
  ret i1 %cmp_res
}

; CHECK: @test_scalar_icmp_ptr_eq_null
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint %struct_Type* %left to i64
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast i64 [[PTRCAST_L]] to <[[CT:2 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]

; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], zeroinitializer
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], zeroinitializer
; CHECK-NEXT: [[Res:%[^ ]+]] = and <1 x i1> [[Slo]], [[Shi]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <1 x i1> [[Res]] to i1
define i1 @test_scalar_icmp_ptr_eq_null(%struct_Type* %left) {
  %cmp_res = icmp eq %struct_Type* %left, null
  ret i1 %cmp_res
}

; CHECK: @test_scalar_icmp_ptr_eqv
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint <2 x %struct_Type*> %lv to <2 x i64>
; CHECK-NEXT: [[PTRCAST_R:%[^ ]+]] = ptrtoint <2 x %struct_Type*> %rv to <2 x i64>
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <2 x i64> [[PTRCAST_L]] to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[IV2:%[^ ]+]] = bitcast <2 x i64> [[PTRCAST_R]] to <[[CT]]>
; CHECK-NEXT: [[Lo_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[low_reg]]
; CHECK-NEXT: [[Hi_r:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV2]], [[high_reg]]
; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], [[Lo_r]]
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], [[Hi_r]]
; CHECK-NEXT: [[Res:%[^ ]+]] = and <2 x i1> [[Slo]], [[Shi]]
define <2 x i1> @test_scalar_icmp_ptr_eqv(<2 x %struct_Type*> %lv, <2 x %struct_Type*> %rv) {
  %cmp_res = icmp eq <2 x %struct_Type*> %lv, %rv
  ret <2 x i1> %cmp_res
}

; CHECK: @test_scalar_icmp_ptr_eqv_null
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint <2 x %struct_Type*> %lv to <2 x i64>
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <2 x i64> [[PTRCAST_L]] to <[[CT:4 x i32]]>
; CHECK-NEXT: [[Lo_l:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[Hi_l:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]

; CHECK-NEXT: [[Slo:%[^ ]+]] = icmp eq <[[ET]]> [[Lo_l]], zeroinitializer
; CHECK-NEXT: [[Shi:%[^ ]+]] = icmp eq <[[ET]]> [[Hi_l]], zeroinitializer
; CHECK-NEXT: [[Res:%[^ ]+]] = and <2 x i1> [[Slo]], [[Shi]]
define <2 x i1> @test_scalar_icmp_ptr_eqv_null(<2 x %struct_Type*> %lv) {
  %cmp_res = icmp eq <2 x %struct_Type*> %lv, zeroinitializer
  ret <2 x i1> %cmp_res
}
