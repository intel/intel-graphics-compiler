;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

; RUN: %opt %use_old_pass_manager% -GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s
; RUN: %opt_new_pm_typed -passes=GenXTranslateIntrinsics -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s



declare <32 x i32> @llvm.genx.raw.sendg.v32i32.v32i1.v32i32.v32i32(i16, i1, i1, i8, <32 x i1>, <32 x i32>, i16, <32 x i32>, i16, i64, i64, i64, <32 x i32>)

define <32 x i32> @test(<32 x i1> %mask, <32 x i32> %src0, <32 x i32> %src1, <32 x i32> %passthru, i64 %ind0) {
  ; CHECK: %dst = call <32 x i32> @llvm.vc.internal.raw.sendg.v32i32.v32i1.v32i32.v32i32(i16 128, i1 false, i1 false, i8 15, <32 x i1> %mask, <32 x i32> %src0, i16 128, <32 x i32> %src1, i16 128, i64 %ind0, i64 undef, i64 20015998343868, <32 x i32> %passthru)
  %dst = call <32 x i32> @llvm.genx.raw.sendg.v32i32.v32i1.v32i32.v32i32(i16 128, i1 false, i1 false, i8 15, <32 x i1> %mask, <32 x i32> %src0, i16 128, <32 x i32> %src1, i16 128, i64 %ind0, i64 undef, i64 20015998343868, <32 x i32> %passthru)
  ret <32 x i32> %dst
}
