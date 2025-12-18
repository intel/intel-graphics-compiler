;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare void @llvm.vc.internal.lsc.store.ugm.v3i1.v3i8.v3i64.v3i32(<3 x i1>, i8, i8, i8, <3 x i8>, i64, <3 x i64>, i16, i32, <3 x i32>)

; CHECK-LABEL: test
define void @test(<3 x i64> %addr, <3 x i32> %value) {
  ; CHECK-NEXT: [[PRED:%[^ ]+]] = call <4 x i1> @llvm.genx.wrpredregion.v4i1.v3i1(<4 x i1> zeroinitializer, <3 x i1> <i1 true, i1 true, i1 true>, i32 0)
  ; CHECK-NEXT: [[ADDR:%[^ ]+]] = call <4 x i64> @llvm.genx.wrregioni.v4i64.v3i64.i16.i1(<4 x i64> undef, <3 x i64> %addr, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: [[DATA:%[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.v3i32.i16.i1(<4 x i32> undef, <3 x i32> %value, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK-NEXT: call void @llvm.vc.internal.lsc.store.ugm.v4i1.v3i8.v4i64.v4i32(<4 x i1> [[PRED]], i8 3, i8 6, i8 1, <3 x i8> zeroinitializer, i64 0, <4 x i64> [[ADDR]], i16 1, i32 0, <4 x i32> [[DATA]])
  call void @llvm.vc.internal.lsc.store.ugm.v3i1.v3i8.v3i64.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 3, i8 6, i8 1, <3 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <3 x i32> %value)
  ret void
}
