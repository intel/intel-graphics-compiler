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

; COM: ===============================
; COM:             TEST #1
; COM: ===============================
; COM: zext <X x i32> to <X x i64> transforms as:
; COM: 1. low = op
; COM: 2. combine(low, 0)

; CHECK: @test_zext1
; CHECK: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET:2 x i32]]> %op, [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: %{{[^ ]+}} = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4

define dllexport spir_kernel void @test_zext1(i32 %0, <2 x i32> %op) {
  %zext = zext <2 x i32> %op to <2 x i64>
  ret void
}

; COM: ===============================
; COM:             TEST #2
; COM: ===============================
; COM: zext <X x i8> to <X x i64> transforms as:
; COM: 1. low = zext op
; COM: 2. combine(low, 0)

; CHECK: @test_zext2
; CHECK: [[EXT32:%[^ ]+]] = zext <2 x i8> %op to <[[ET:2 x i32]]>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[EXT32]], [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: %{{[^ ]+}} = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4

define dllexport spir_kernel void @test_zext2(i32 %0, <2 x i8> %op) {
  %zext = zext <2 x i8> %op to <2 x i64>
  ret void
}

; COM: ===============================
; COM:             TEST #3
; COM: ===============================
; COM: zext i8 to i64 transforms as:
; COM: 1. vop cast op to vector
; COM: 2. low = zext vop
; COM: 2. combine(low, 0)

; CHECK: @test_zext3
; CHECK: [[CASTED:%[^ ]+]] = bitcast i8 %op to <1 x i8>
; CHECK-NEXT: [[EXT32:%[^ ]+]] = zext <1 x i8> [[CASTED]] to <[[ET:1 x i32]]>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:2 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[EXT32]], [[WR_ARGS:i32 0, i32 1, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> zeroinitializer, [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTVI64:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <1 x i64>
; CHECK-NEXT: [[RECASTI64:%[^ ]+]] =   bitcast <1 x i64> [[CASTVI64]] to i64

define dllexport spir_kernel void @test_zext3(i32 %0, i8 %op) {
  %zext = zext i8 %op to i64
  ret void
}

; COM: ===============================
; COM:             TEST #4
; COM: ===============================
; COM: sext <X x i32> to <X x i64> transforms as:
; COM: 1. sign_hi = ashr low, 31
; COM: 2. combine(low, sign_hi)

; CHECK: @test_sext1
; CHECK: [[SIGN_PART:[^ ]+]] = ashr <[[ET:2 x i32]]> %op, <i32 31, i32 31>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> %op, [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: %{{[^ ]+}} = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[SIGN_PART]], [[WR_ARGS]], i16 4

define dllexport spir_kernel void @test_sext1(i32 %0, <2 x i32> %op) {
  %sext = sext <2 x i32> %op to <2 x i64>
  ret void
}

; COM: ===============================
; COM:             TEST #5
; COM: ===============================
; COM: sext <X x i8> to <X x i64> transforms as:
; COM: 1. low = sext op to i32
; COM: 2. sign_hi = ashr low, 31
; COM: 3. combine(low, sign_hi)

; CHECK: @test_sext2
; CHECK: [[EXT32:%[^ ]+]] = sext <2 x i8> %op to <[[ET:2 x i32]]>
; CHECK-NEXT: [[SIGN_PART:[^ ]+]] = ashr <[[ET:2 x i32]]> [[EXT32]], <i32 31, i32 31>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:4 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[EXT32]], [[WR_ARGS:i32 0, i32 2, i32 2]], i16 0
; CHECK-NEXT: %{{[^ ]+}} = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[SIGN_PART]], [[WR_ARGS]], i16 4

define dllexport spir_kernel void @test_sext2(i32 %0, <2 x i8> %op) {
  %sext = sext <2 x i8> %op to <2 x i64>
  ret void
}

; COM: ===============================
; COM:             TEST #6
; COM: ===============================
; COM: sext i8 to i64 transforms as:
; COM: 1. vop cast op to vector
; COM: 2. low = sext vop
; COM: 3. sign_hi = ashr low, 31
; COM: 4. vval = combine(low, sign_hi)
; COM: 5. recast vval to scalar

; CHECK: @test_sext3
; CHECK: [[CASTED:[^ ]+]] = bitcast i8 %op to <1 x i8>
; CHECK-NEXT: [[EXT32:%[^ ]+]] = sext <1 x i8> [[CASTED]] to <[[ET:1 x i32]]>
; CHECK-NEXT: [[SIGN_PART:%[^ ]+]] = ashr <1 x i32> [[EXT32]], <i32 31>
; CHECK-NEXT: [[P_JOIN:%[^ ]+]] = call <[[CT:2 x i32]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> undef, <[[ET]]> [[EXT32]], [[WR_ARGS:i32 0, i32 1, i32 2]], i16 0
; CHECK-NEXT: [[JOINED:%[^ ]+]] = call <[[CT]]> @llvm.genx.wrregioni.{{[^(]+}}(<[[CT]]> [[P_JOIN]], <[[ET]]> [[SIGN_PART]], [[WR_ARGS]], i16 4
; CHECK-NEXT: [[CASTVI64:%[^ ]+]] = bitcast <[[CT]]> [[JOINED]] to <1 x i64>
; CHECK-NExT: [[RECASTI64:%[^ ]+]] =   bitcast <1 x i64> [[CASTVI64]] to i64

define dllexport spir_kernel void @test_sext3(i32 %0, i8 %op) {
  %sext = sext i8 %op to i64
  ret void
}
