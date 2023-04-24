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

; CHECK: @test_scalar_ptr2i32
; CHECK-NEXT: [[PTRCAST:%[^ ]+]] = bitcast %struct_Type* %op to <1 x %struct_Type*>
; CHECK-NEXT: [[I64:%[^ ]+]] = ptrtoint <1 x %struct_Type*> [[PTRCAST]] to <[[CT:1 x i64]]>
; CHECK-NEXT: [[CAST32:%[^ ]+]] = bitcast <[[CT]]> [[I64]] to <[[ET:2 x i32]]>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <[[SRT:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[ET]]> [[CAST32]], [[low_reg:i32 2, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <[[SRT]]> [[RES]] to i32
; CHECK-NEXT: ret i32 [[Casted]]

%struct_Type = type { i32, i64, i16, i8 }

define i32 @test_scalar_ptr2i32(%struct_Type* %op) {
  %i32 = ptrtoint %struct_Type* %op to i32
  ret i32 %i32
}
; CHECK: @test_scalar_ptr2i16
; CHECK-NEXT: [[PTRCAST:%[^ ]+]] = bitcast %struct_Type* %op to <1 x %struct_Type*>
; CHECK-NEXT: [[I64:%[^ ]+]] = ptrtoint <1 x %struct_Type*> [[PTRCAST]] to <[[CT:1 x i64]]>
; CHECK-NEXT: [[CAST16:%[^ ]+]] = bitcast <[[CT]]> [[I64]] to <[[ET:4 x i16]]>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <[[SRT:1 x i16]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[ET]]> [[CAST16]], [[low_reg:i32 4, i32 1, i32 4, i16 0,]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <[[SRT]]> [[RES]] to i16
; CHECK-NEXT: ret i16 [[Casted]]
define i16 @test_scalar_ptr2i16(%struct_Type* %op) {
  %i16 = ptrtoint %struct_Type* %op to i16
  ret i16 %i16
}
; CHECK: @test_scalar_ptr2i8
; CHECK-NEXT: [[PTRCAST:%[^ ]+]] = bitcast %struct_Type* %op to <1 x %struct_Type*>
; CHECK-NEXT: [[I64:%[^ ]+]] = ptrtoint <1 x %struct_Type*> [[PTRCAST]] to <[[CT:1 x i64]]>
; CHECK-NEXT: [[CAST8:%[^ ]+]] = bitcast <[[CT]]> [[I64]] to <[[ET:8 x i8]]>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <[[SRT:1 x i8]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[ET]]> [[CAST8]], [[low_reg:i32 8, i32 1, i32 8, i16 0,]]
; CHECK-NEXT: [[Casted:%[^ ]+]] = bitcast <[[SRT]]> [[RES]] to i8
; CHECK-NEXT: ret i8 [[Casted]]
define i8 @test_scalar_ptr2i8(%struct_Type* %op) {
  %i8 = ptrtoint %struct_Type* %op to i8
  ret i8 %i8
}
; CHECK: @test_vector_ptrv2i32
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint <2 x %struct_Type*> %lv to <2 x i64>
; CHECK-NEXT: [[CAST32:%[^ ]+]] = bitcast <2 x i64> [[PTRCAST_L]] to <[[CT:4 x i32]]>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[CAST32]], [[low_reg:i32 4, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: ret <[[ET]]> [[RES]]
define <2 x i32> @test_vector_ptrv2i32(<2 x %struct_Type*> %lv) {
  %i32v = ptrtoint <2 x %struct_Type*> %lv to <2 x i32>
  ret <2 x i32> %i32v
}
; CHECK: @test_vector_ptrv2i8
; CHECK-NEXT: [[PTRCAST_L:%[^ ]+]] = ptrtoint <2 x %struct_Type*> %lv to <2 x i64>
; CHECK-NEXT: [[CAST8V:%[^ ]+]] = bitcast <2 x i64> [[PTRCAST_L]] to <[[CT:16 x i8]]>
; CHECK-NEXT: [[RES:%[^ ]+]] = call <[[ET:2 x i8]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[CAST8V]], [[low_reg:i32 16, i32 2, i32 8, i16 0,]]
; CHECK-NEXT: ret <[[ET]]> [[RES]]
define <2 x i8> @test_vector_ptrv2i8(<2 x %struct_Type*> %lv) {
  %i8v = ptrtoint <2 x %struct_Type*> %lv to <2 x i8>
  ret <2 x i8> %i8v
}

; CHECK: @test_scalar_i8ptr
; CHECK-NEXT: [[BITCAST:%[^ ]+]] = bitcast i8 %lv to <1 x i8>
; CHECK-NEXT: [[ZEXT32:%[^ ]+]] = zext <1 x i8> [[BITCAST]] to <[[ET:1 x i32]]>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:2 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[ZEXT32]], [[WR_ARGS:i32 0, i32 1, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTVI64:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[IV64:1 x i64]]>
; CHECK-NEXT: [[CASTVPTRV:%[^ ]+]] = inttoptr <[[IV64]]> [[CASTVI64]] to <[[DSTV:1 x %struct_Type\*]]>
; CHECK-NEXT: [[CASTPTR:%[^ ]+]] = bitcast <[[DSTV]]> [[CASTVPTRV]] to [[DST:%struct_Type\*]]
; CHECK-NEXT: ret [[DST]] [[CASTPTR]]
define %struct_Type* @test_scalar_i8ptr(i8 %lv) {
  %ptr = inttoptr i8 %lv to %struct_Type*
  ret %struct_Type* %ptr
}
; CHECK: @test_vector_i8ptr
; CHECK-NEXT: [[ZEXT32:%[^ ]+]] = zext <2 x i8> %lv to <[[ET:2 x i32]]>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[ZEXT32]], [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTVI64:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[IV64:2 x i64]]>
; CHECK-NEXT: [[CASTVPTR:%[^ ]+]] = inttoptr <[[IV64]]> [[CASTVI64]] to <[[DST:2 x %struct_Type\*]]>
; CHECK-NEXT: ret <[[DST]]> [[CASTVPTR]]
define <2 x %struct_Type*> @test_vector_i8ptr(<2 x i8> %lv) {
  %ptr = inttoptr <2 x i8> %lv to <2 x %struct_Type*>
  ret <2 x %struct_Type*> %ptr
}
; CHECK: @test_vector_i32ptr
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %lv, [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTVI64:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <[[IV64:2 x i64]]>
; CHECK-NEXT: [[CASTVPTR:%[^ ]+]] = inttoptr <[[IV64]]> [[CASTVI64]] to <[[DST:2 x %struct_Type\*]]>
; CHECK-NEXT: ret <[[DST]]> [[CASTVPTR]]
define <2 x %struct_Type*> @test_vector_i32ptr(<2 x i32> %lv) {
  %ptr = inttoptr <2 x i32> %lv to <2 x %struct_Type*>
  ret <2 x %struct_Type*> %ptr
}

; CHECK: @test_vector_ptri_noop
; CHECK-NEXT: [[PTR:%[^ ]+]] = inttoptr <[[DST:2 x i64]]> %lv to <[[PT:2 x %struct_Type\*]]>
; CHECK-NEXT: [[INTV:%[^ ]+]] = ptrtoint <[[PT]]> [[PTR]] to <[[DST]]>
; CHECK-NEXT: ret <[[DST]]> [[INTV]]
define <2 x i64> @test_vector_ptri_noop(<2 x i64> %lv) {
  %ptr = inttoptr <2 x i64> %lv to <2 x %struct_Type*>
  %intv = ptrtoint <2 x %struct_Type*> %ptr to <2 x i64>
  ret <2 x i64> %intv
}
