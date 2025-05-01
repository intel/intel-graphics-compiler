;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -mattr=+translate_legacy_message -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <32 x i32> @llvm.genx.gather4.scaled.v32i32.v16i1.v16i32(<16 x i1>, i32, i16, i32, i32, <16 x i32>, <32 x i32>)
declare <32 x i32> @llvm.genx.gather4.scaled2.v32i32.v16i32(i32, i16, i32, i32, <16 x i32>)
declare <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32, i16, i32, i32, <16 x i32>, <16 x i1>)

declare <32 x i32> @llvm.genx.svm.gather4.scaled.v32i32.v16i1.v16i64(<16 x i1>, i32, i16, i64, <16 x i64>, <32 x i32>)

declare void @llvm.genx.scatter4.scaled.v16i1.v16i32.v32i32(<16 x i1>, i32, i16, i32, i32, <16 x i32>, <32 x i32>)

declare void @llvm.genx.svm.scatter4.scaled.v16i1.v16i64.v32i32(<16 x i1>, i32, i16, i64, <16 x i64>, <32 x i32>)

declare <32 x float> @llvm.genx.gather4.masked.scaled2.v32f32.v8i32.v8i1(i32, i16, i32, i32, <8 x i32>, <8 x i1>)
declare void @llvm.genx.scatter4.scaled.v8i1.v8i32.v32f32(<8 x i1>, i32, i16, i32, i32, <8 x i32>, <32 x float>)


; CHECK-LABEL: test_bti
define void @test_bti(<16 x i1> %pred, i32 %offset, <16 x i32> %addr, <32 x i32> %data) {
  ; CHECK: [[INS1:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT1:%[^ ]+]] = shufflevector <16 x i32> [[INS1]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR1:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT1]]
  ; CHECK: %ld = call <32 x i32> @llvm.vc.internal.lsc.load.quad.bti.v32i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 123, <16 x i32> [[ADDR1]], i16 1, i32 0, <32 x i32> %data)
  %ld = call <32 x i32> @llvm.genx.gather4.scaled.v32i32.v16i1.v16i32(<16 x i1> %pred, i32 9, i16 0, i32 123, i32 %offset, <16 x i32> %addr, <32 x i32> %data)

  ; CHECK: [[INS2:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT2:%[^ ]+]] = shufflevector <16 x i32> [[INS2]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR2:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT2]]
  ; CHECK: %ldx = call <32 x i32> @llvm.vc.internal.lsc.load.quad.bti.v32i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 123, <16 x i32> [[ADDR2]], i16 1, i32 0, <32 x i32> undef)
  %ldx = call <32 x i32> @llvm.genx.gather4.scaled2.v32i32.v16i32(i32 9, i16 0, i32 123, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS3:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT3:%[^ ]+]] = shufflevector <16 x i32> [[INS3]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR3:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT3]]
  ; CHECK: %ldm = call <32 x i32> @llvm.vc.internal.lsc.load.quad.bti.v32i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 123, <16 x i32> [[ADDR3]], i16 1, i32 0, <32 x i32> undef)
  %ldm = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 9, i16 0, i32 123, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS4:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT4:%[^ ]+]] = shufflevector <16 x i32> [[INS4]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR4:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT4]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v32i32(<16 x i1> %pred, i8 2, i8 3, i8 3, <2 x i8> zeroinitializer, i32 123, <16 x i32> [[ADDR4]], i16 1, i32 0, <32 x i32> %data)
  call void @llvm.genx.scatter4.scaled.v16i1.v16i32.v32i32(<16 x i1> %pred, i32 3, i16 0, i32 123, i32 %offset, <16 x i32> %addr, <32 x i32> %data)

  ret void
}

; CHECK-LABEL: test_slm
define void @test_slm(<16 x i1> %pred, i32 %offset, <16 x i32> %addr, <32 x i32> %data) {
  ; CHECK: [[INS1:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT1:%[^ ]+]] = shufflevector <16 x i32> [[INS1]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR1:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT1]]
  ; CHECK: %ld = call <32 x i32> @llvm.vc.internal.lsc.load.quad.slm.v32i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR1]], i16 1, i32 0, <32 x i32> %data)
  %ld = call <32 x i32> @llvm.genx.gather4.scaled.v32i32.v16i1.v16i32(<16 x i1> %pred, i32 9, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <32 x i32> %data)

  ; CHECK: [[INS2:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT2:%[^ ]+]] = shufflevector <16 x i32> [[INS2]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR2:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT2]]
  ; CHECK: %ldx = call <32 x i32> @llvm.vc.internal.lsc.load.quad.slm.v32i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR2]], i16 1, i32 0, <32 x i32> undef)
  %ldx = call <32 x i32> @llvm.genx.gather4.scaled2.v32i32.v16i32(i32 9, i16 0, i32 254, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS3:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT3:%[^ ]+]] = shufflevector <16 x i32> [[INS3]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR3:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT3]]
  ; CHECK: %ldm = call <32 x i32> @llvm.vc.internal.lsc.load.quad.slm.v32i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 9, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR3]], i16 1, i32 0, <32 x i32> undef)
  %ldm = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v16i32.v16i1(i32 9, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS4:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT4:%[^ ]+]] = shufflevector <16 x i32> [[INS4]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR4:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT4]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.slm.v16i1.v2i8.v16i32.v32i32(<16 x i1> %pred, i8 2, i8 3, i8 3, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR4]], i16 1, i32 0, <32 x i32> %data)
  call void @llvm.genx.scatter4.scaled.v16i1.v16i32.v32i32(<16 x i1> %pred, i32 3, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <32 x i32> %data)

  ret void
}

; CHECK-LABEL: test_ugm
define void @test_ugm(<16 x i1> %pred, i64 %offset, <16 x i64> %addr, <32 x i32> %data) {
  ; CHECK: [[INS1:%[^ ]+]] = insertelement <16 x i64> poison, i64 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT1:%[^ ]+]] = shufflevector <16 x i64> [[INS1]], <16 x i64> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR1:%[^ ]+]] = add <16 x i64> %addr, [[SPLAT1]]
  ; CHECK: %ld = call <32 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[ADDR1]], i16 1, i32 0, <32 x i32> %data)
  %ld = call <32 x i32> @llvm.genx.svm.gather4.scaled.v32i32.v16i1.v16i64(<16 x i1> %pred, i32 9, i16 0, i64 %offset, <16 x i64> %addr, <32 x i32> %data)

  ; CHECK: [[INS2:%[^ ]+]] = insertelement <16 x i64> poison, i64 %offset, {{(i32)?(i64)?}} 0
  ; CHECK: [[SPLAT2:%[^ ]+]] = shufflevector <16 x i64> [[INS2]], <16 x i64> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR2:%[^ ]+]] = add <16 x i64> %addr, [[SPLAT2]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> %pred, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[ADDR2]], i16 1, i32 0, <32 x i32> %data)
  call void @llvm.genx.svm.scatter4.scaled.v16i1.v16i64.v32i32(<16 x i1> %pred, i32 3, i16 0, i64 %offset, <16 x i64> %addr, <32 x i32> %data)

  ret void
}

; CHECK-LABEL: test_ugm_mask1
define void @test_ugm_mask1(<8 x i32> %addr1, <8 x i32> %addr2) {
  ; CHECK: [[LOAD:%[^ ]+]] = call <8 x float> @llvm.vc.internal.lsc.load.quad.bti.v8f32.v8i1.v2i8.v8i32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <8 x i32> %1, i16 1, i32 0, <8 x float> undef)
  ; CHECK: [[WRREG:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v8f32.i16.i1(<32 x float> undef, <8 x float> [[LOAD]], i32 0, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[RDREG:%[^ ]+]] = call <8 x float> @llvm.genx.rdregionf.v8f32.v32f32.i16(<32 x float> [[WRREG]], i32 0, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.bti.v8i1.v2i8.v8i32.v8f32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <8 x i32> %3, i16 1, i32 0, <8 x float> [[RDREG]])

  %ld = call <32 x float> @llvm.genx.gather4.masked.scaled2.v32f32.v8i32.v8i1(i32 1, i16 0, i32 1, i32 0, <8 x i32> %addr1, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  call void @llvm.genx.scatter4.scaled.v8i1.v8i32.v32f32(<8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 1, i16 0, i32 1, i32 0, <8 x i32> %addr2, <32 x float> %ld)

  ret void
}
