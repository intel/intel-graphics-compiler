;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare i1 @llvm.genx.all.v1i1(<1 x i1>) #1
declare i1 @llvm.genx.all.v1i8(<1 x i8>) #1
declare i1 @llvm.genx.any.v1i1(<1 x i1>) #1
declare i1 @llvm.genx.any.v1i8(<1 x i8>) #1

define i1 @test_scalar_all_generic(<1 x i8> %src) {
  ; CHECK: [[ext:%[a-z0-9.]+]] = zext <1 x i8> %src to <1 x i16>
  ; CHECK-NEXT: [[mask:%[a-z0-9.]+]] = icmp ne <1 x i16> [[ext]], zeroinitializer
  ; CHECK-NEXT: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> [[mask]] to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.all.v1i8(<1 x i8> %src)
  ret i1 %dst
}

define i1 @test_scalar_all_sext(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = sext <1 x i1> %src to <1 x i8>
  %dst = call i1 @llvm.genx.all.v1i8(<1 x i8> %ext)
  ret i1 %dst
}

define i1 @test_scalar_all_zext(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = zext <1 x i1> %src to <1 x i8>
  %dst = call i1 @llvm.genx.all.v1i8(<1 x i8> %ext)
  ret i1 %dst
}

define i1 @test_scalar_all(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.all.v1i1(<1 x i1> %src)
  ret i1 %dst
}

define i1 @test_scalar_any_generic(<1 x i8> %src) {
  ; CHECK: [[ext:%[a-z0-9.]+]] = zext <1 x i8> %src to <1 x i16>
  ; CHECK-NEXT: [[mask:%[a-z0-9.]+]] = icmp ne <1 x i16> [[ext]], zeroinitializer
  ; CHECK-NEXT: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> [[mask]] to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.any.v1i8(<1 x i8> %src)
  ret i1 %dst
}

define i1 @test_scalar_any_sext(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = sext <1 x i1> %src to <1 x i8>
  %dst = call i1 @llvm.genx.any.v1i8(<1 x i8> %ext)
  ret i1 %dst
}

define i1 @test_scalar_any_zext(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = zext <1 x i1> %src to <1 x i8>
  %dst = call i1 @llvm.genx.any.v1i8(<1 x i8> %ext)
  ret i1 %dst
}

define i1 @test_scalar_any(<1 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = bitcast <1 x i1> %src to i1
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.any.v1i1(<1 x i1> %src)
  ret i1 %dst
}

declare i1 @llvm.genx.all.v8i8(<8 x i8>) #1
declare i1 @llvm.genx.any.v8i8(<8 x i8>) #1

define i1 @test_all_generic(<8 x i8> %src) {
  ; CHECK: [[ext:%[a-z0-9.]+]] = zext <8 x i8> %src to <8 x i16>
  ; CHECK-NEXT: [[mask:%[a-z0-9.]+]] = icmp ne <8 x i16> [[ext]], zeroinitializer
  ; CHECK-NEXT: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.all.v8i1(<8 x i1> [[mask]])
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.all.v8i8(<8 x i8> %src)
  ret i1 %dst
}

define i1 @test_all_sext(<8 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.all.v8i1(<8 x i1> %src)
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = sext <8 x i1> %src to <8 x i8>
  %dst = call i1 @llvm.genx.all.v8i8(<8 x i8> %ext)
  ret i1 %dst
}

define i1 @test_all_zext(<8 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.all.v8i1(<8 x i1> %src)
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = zext <8 x i1> %src to <8 x i8>
  %dst = call i1 @llvm.genx.all.v8i8(<8 x i8> %ext)
  ret i1 %dst
}

define i1 @test_any_generic(<8 x i8> %src) {
  ; CHECK: [[ext:%[a-z0-9.]+]] = zext <8 x i8> %src to <8 x i16>
  ; CHECK-NEXT: [[mask:%[a-z0-9.]+]] = icmp ne <8 x i16> [[ext]], zeroinitializer
  ; CHECK-NEXT: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.any.v8i1(<8 x i1> [[mask]])
  ; CHECK-NEXT: ret i1 [[res]]
  %dst = call i1 @llvm.genx.any.v8i8(<8 x i8> %src)
  ret i1 %dst
}

define i1 @test_any_sext(<8 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.any.v8i1(<8 x i1> %src)
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = sext <8 x i1> %src to <8 x i8>
  %dst = call i1 @llvm.genx.any.v8i8(<8 x i8> %ext)
  ret i1 %dst
}

define i1 @test_any_zext(<8 x i1> %src) {
  ; CHECK: [[res:%[a-z0-9.]+]] = call i1 @llvm.genx.any.v8i1(<8 x i1> %src)
  ; CHECK-NEXT: ret i1 [[res]]
  %ext = zext <8 x i1> %src to <8 x i8>
  %dst = call i1 @llvm.genx.any.v8i8(<8 x i8> %ext)
  ret i1 %dst
}
