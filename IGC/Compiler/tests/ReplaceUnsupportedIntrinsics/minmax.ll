;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus

; RUN: igc_opt -igc-replace-unsupported-intrinsics -dce -S < %s 2>&1 | FileCheck %s

define i64 @test_smax_i64(i64 %argL, i64 %argR) {
;   CHECK-LABEL: define i64 @test_smax_i64(
;   %[[CMP:.*]] = icmp slt i64 %argL, %argR
;   %[[RES:.*]] = select i1 %[[CMP]], i64 %argL, %argR
;   ret %[[RES]]
    %1 = call i64 @llvm.smax.i64(i64 %argL, i64 %argR)
    ret i64 %1
}

define i64 @test_smin_i64(i64 %argL, i64 %argR) {
;   CHECK-LABEL: define i64 @test_smin_i64(
;   %[[CMP:.*]] = icmp sgt i64 %argL, %argR
;   %[[RES:.*]] = select i1 %[[CMP]], i64 %argL, %argR
;   ret %[[RES]]
    %1 = call i64 @llvm.smin.i64(i64 %argL, i64 %argR)
    ret i64 %1
}

define i64 @test_umax_i64(i64 %argL, i64 %argR) {
;   CHECK-LABEL: define i64 @test_umax_i64(
;   %[[CMP:.*]] = icmp ult i64 %argL, %argR
;   %[[RES:.*]] = select i1 %[[CMP]], i64 %argL, %argR
;   ret %[[RES]]
    %1 = call i64 @llvm.umax.i64(i64 %argL, i64 %argR)
    ret i64 %1
}

define i64 @test_umin_i64(i64 %argL, i64 %argR) {
;   CHECK-LABEL: define i64 @test_umin_i64(
;   %[[CMP:.*]] = icmp ugt i64 %argL, %argR
;   %[[RES:.*]] = select i1 %[[CMP]], i64 %argL, %argR
;   ret %[[RES]]
    %1 = call i64 @llvm.umin.i64(i64 %argL, i64 %argR)
    ret i64 %1
}

define i1 @test_smax_i1(i1 %argL, i1 %argR) {
;   CHECK-LABEL: define i1 @test_smax_i1(
;   %[[MAX:.*]] = or i1 %argL, %argR
;   ret %[[MAX]]
    %1 = call i1 @llvm.smax.i1(i1 %argL, i1 %argR)
    ret i1 %1
}

define i1 @test_umax_i1(i1 %argL, i1 %argR) {
;   CHECK-LABEL: define i1 @test_umax_i1(
;   %[[MAX:.*]] = or ult i1 %argL, %argR
;   ret %[[MAX]]
    %1 = call i1 @llvm.umax.i1(i1 %argL, i1 %argR)
    ret i1 %1
}

define i1 @test_smin_i1(i1 %argL, i1 %argR) {
;   CHECK-LABEL: define i1 @test_smin_i1(
;   %[[MIN:.*]] = and i1 %argL, %argR
;   ret %[[MIN]]
    %1 = call i1 @llvm.smin.i1(i1 %argL, i1 %argR)
    ret i1 %1
}

define i1 @test_umin_i1(i1 %argL, i1 %argR) {
;   CHECK-LABEL: define i1 @test_umin_i1(
;   %[[MIN:.*]] = and i1 %argL, %argR
;   ret %[[MIN]]
    %1 = call i1 @llvm.umin.i1(i1 %argL, i1 %argR)
    ret i1 %1
}

define i32 @test_smax_i32(i32 %argL, i32 %argR) {
;   CHECK-LABEL: define i32 @test_smax_i32(
;   %1 = call i32 @llvm.smax.i32(i32 %argL, i32 %argR)
;   ret %1
    %1 = call i32 @llvm.smax.i32(i32 %argL, i32 %argR)
    ret i32 %1
}

declare i64 @llvm.smax.i64(i64, i64)
declare i64 @llvm.smin.i64(i64, i64)
declare i64 @llvm.umax.i64(i64, i64)
declare i64 @llvm.umin.i64(i64, i64)
declare i1 @llvm.smax.i1(i1, i1)
declare i1 @llvm.umax.i1(i1, i1)
declare i1 @llvm.smin.i1(i1, i1)
declare i1 @llvm.umin.i1(i1, i1)
declare i32 @llvm.smax.i32(i32, i32)
