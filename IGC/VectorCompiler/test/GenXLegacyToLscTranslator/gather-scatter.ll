;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -enable-debugify -GenXLegacyToLscTranslator -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -mattr=+translate_legacy_message -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1>, i32, i16, i32, i32, <16 x i32>, <16 x i32>)
declare <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32, i16, i32, i32, <16 x i32>)
declare <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32, i16, i32, i32, <16 x i32>, <16 x i1>)

declare void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1>, i32, i16, i32, i32, <16 x i32>, <16 x i32>)

declare <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <64 x i8>)
declare <128 x i8> @llvm.genx.svm.gather.v128i8.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <128 x i8>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1>, i32, <16 x i64>, <64 x i8>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v128i8(<16 x i1>, i32, <16 x i64>, <128 x i8>)

declare <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <16 x i32>)
declare <32 x i32> @llvm.genx.svm.gather.v32i32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <32 x i32>)
declare <64 x i32> @llvm.genx.svm.gather.v64i32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <64 x i32>)
declare <16 x i64> @llvm.genx.svm.gather.v16i64.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <16 x i64>)
declare <32 x i64> @llvm.genx.svm.gather.v32i64.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <32 x i64>)
declare <64 x i64> @llvm.genx.svm.gather.v64i64.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <64 x i64>)

declare void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1>, i32, <16 x i64>, <16 x i32>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v32i32(<16 x i1>, i32, <16 x i64>, <32 x i32>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v64i32(<16 x i1>, i32, <16 x i64>, <64 x i32>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1>, i32, <16 x i64>, <16 x i64>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v32i64(<16 x i1>, i32, <16 x i64>, <32 x i64>)
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v64i64(<16 x i1>, i32, <16 x i64>, <64 x i64>)

; CHECK-LABEL: test_bti
define void @test_bti(<16 x i1> %pred, i32 %offset, <16 x i32> %addr, <16 x i32> %data) {
  ; CHECK: [[INS1:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT1:%[^ ]+]] = shufflevector <16 x i32> [[INS1]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR1:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT1]]
  ; CHECK: %ld8 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR1]], i16 1, i32 0, <16 x i32> %data)
  %ld8 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 0, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS2:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT2:%[^ ]+]] = shufflevector <16 x i32> [[INS2]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR2:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT2]]
  ; CHECK: %ld16 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR2]], i16 1, i32 0, <16 x i32> %data)
  %ld16 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 1, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS3:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT3:%[^ ]+]] = shufflevector <16 x i32> [[INS3]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR3:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT3]]
  ; CHECK: %ld32 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR3]], i16 1, i32 0, <16 x i32> %data)
  %ld32 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 2, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS4:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT4:%[^ ]+]] = shufflevector <16 x i32> [[INS4]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR4:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT4]]
  ; CHECK: %ldx8 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR4]], i16 1, i32 0, <16 x i32> undef)
  %ldx8 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 0, i16 0, i32 1, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS5:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT5:%[^ ]+]] = shufflevector <16 x i32> [[INS5]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR5:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT5]]
  ; CHECK: %ldx16 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR5]], i16 1, i32 0, <16 x i32> undef)
  %ldx16 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 1, i16 0, i32 1, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS6:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT6:%[^ ]+]] = shufflevector <16 x i32> [[INS6]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR6:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT6]]
  ; CHECK: %ldx32 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR6]], i16 1, i32 0, <16 x i32> undef)
  %ldx32 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 2, i16 0, i32 1, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS7:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT7:%[^ ]+]] = shufflevector <16 x i32> [[INS7]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR7:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT7]]
  ; CHECK: %ldm8 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR7]], i16 1, i32 0, <16 x i32> undef)
  %ldm8 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 0, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS8:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT8:%[^ ]+]] = shufflevector <16 x i32> [[INS8]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR8:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT8]]
  ; CHECK: %ldm16 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR8]], i16 1, i32 0, <16 x i32> undef)
  %ldm16 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 1, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS9:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT9:%[^ ]+]] = shufflevector <16 x i32> [[INS9]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR9:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT9]]
  ; CHECK: %ldm32 = call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR9]], i16 1, i32 0, <16 x i32> undef)
  %ldm32 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS10:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT10:%[^ ]+]] = shufflevector <16 x i32> [[INS10]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR10:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT10]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR10]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 0, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS11:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT11:%[^ ]+]] = shufflevector <16 x i32> [[INS11]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR11:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT11]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR11]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 1, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS12:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT12:%[^ ]+]] = shufflevector <16 x i32> [[INS12]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR12:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT12]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 1, <16 x i32> [[ADDR12]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 2, i16 0, i32 1, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ret void
}

; CHECK-LABEL: test_slm
define void @test_slm(<16 x i1> %pred, i32 %offset, <16 x i32> %addr, <16 x i32> %data) {
  ; CHECK: [[INS1:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT1:%[^ ]+]] = shufflevector <16 x i32> [[INS1]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR1:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT1]]
  ; CHECK: %ld8 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR1]], i16 1, i32 0, <16 x i32> %data)
  %ld8 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 0, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS2:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT2:%[^ ]+]] = shufflevector <16 x i32> [[INS2]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR2:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT2]]
  ; CHECK: %ld16 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR2]], i16 1, i32 0, <16 x i32> %data)
  %ld16 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 1, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS3:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT3:%[^ ]+]] = shufflevector <16 x i32> [[INS3]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR3:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT3]]
  ; CHECK: %ld32 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR3]], i16 1, i32 0, <16 x i32> %data)
  %ld32 = call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> %pred, i32 2, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS4:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT4:%[^ ]+]] = shufflevector <16 x i32> [[INS4]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR4:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT4]]
  ; CHECK: %ldx8 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR4]], i16 1, i32 0, <16 x i32> undef)
  %ldx8 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 0, i16 0, i32 254, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS5:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT5:%[^ ]+]] = shufflevector <16 x i32> [[INS5]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR5:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT5]]
  ; CHECK: %ldx16 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR5]], i16 1, i32 0, <16 x i32> undef)
  %ldx16 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 1, i16 0, i32 254, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS6:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT6:%[^ ]+]] = shufflevector <16 x i32> [[INS6]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR6:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT6]]
  ; CHECK: %ldx32 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> {{<(i1 true(, )?){16}>}}, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR6]], i16 1, i32 0, <16 x i32> undef)
  %ldx32 = call <16 x i32> @llvm.genx.gather.scaled2.v16i32.v16i1.v16i32(i32 2, i16 0, i32 254, i32 %offset, <16 x i32> %addr)

  ; CHECK: [[INS7:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT7:%[^ ]+]] = shufflevector <16 x i32> [[INS7]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR7:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT7]]
  ; CHECK: %ldm8 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR7]], i16 1, i32 0, <16 x i32> undef)
  %ldm8 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 0, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS8:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT8:%[^ ]+]] = shufflevector <16 x i32> [[INS8]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR8:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT8]]
  ; CHECK: %ldm16 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR8]], i16 1, i32 0, <16 x i32> undef)
  %ldm16 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 1, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS9:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT9:%[^ ]+]] = shufflevector <16 x i32> [[INS9]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR9:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT9]]
  ; CHECK: %ldm32 = call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR9]], i16 1, i32 0, <16 x i32> undef)
  %ldm32 = call <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32 2, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i1> %pred)

  ; CHECK: [[INS10:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT10:%[^ ]+]] = shufflevector <16 x i32> [[INS10]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR10:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT10]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.slm.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR10]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 0, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS11:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT11:%[^ ]+]] = shufflevector <16 x i32> [[INS11]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR11:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT11]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.slm.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR11]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 1, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ; CHECK: [[INS12:%[^ ]+]] = insertelement <16 x i32> poison, i32 %offset, i32 0
  ; CHECK: [[SPLAT12:%[^ ]+]] = shufflevector <16 x i32> [[INS12]], <16 x i32> poison, <16 x i32> zeroinitializer
  ; CHECK: [[ADDR12:%[^ ]+]] = add <16 x i32> %addr, [[SPLAT12]]
  ; CHECK: call void @llvm.vc.internal.lsc.store.slm.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 0, <16 x i32> [[ADDR12]], i16 1, i32 0, <16 x i32> %data)
  call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> %pred, i32 2, i16 0, i32 254, i32 %offset, <16 x i32> %addr, <16 x i32> %data)

  ret void
}

; CHECK-LABEL: test_ugm_unaligned_dword
define void @test_ugm_unaligned_dword(<16 x i1> %pred, <16 x i64> %addr, <64 x i8> %data) {
  ; CHECK: [[PRECAST1:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: [[LD1:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST1]])
  ; CHECK: %ld8 = bitcast <16 x i32> [[LD1]] to <64 x i8>
  %ld8 = call <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64(<16 x i1> %pred, i32 0, <16 x i64> %addr, <64 x i8> %data)

  ; CHECK: [[PRECAST2:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: [[LD2:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST2]])
  ; CHECK: %ld16 = bitcast <16 x i32> [[LD2]] to <64 x i8>
  %ld16 = call <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64(<16 x i1> %pred, i32 1, <16 x i64> %addr, <64 x i8> %data)

  ; CHECK: [[PRECAST3:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: [[LD3:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST3]])
  ; CHECK: %ld32 = bitcast <16 x i32> [[LD3]] to <64 x i8>
  %ld32 = call <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i8> %data)

  ; CHECK: [[PRECAST4:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST4]])
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> %pred, i32 0, <16 x i64> %addr, <64 x i8> %data)

  ; CHECK: [[PRECAST5:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST5]])
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> %pred, i32 1, <16 x i64> %addr, <64 x i8> %data)

  ; CHECK: [[PRECAST6:%[^ ]+]] = bitcast <64 x i8> %data to <16 x i32>
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> [[PRECAST6]])
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i8> %data)

  ret void
}

; CHECK-LABEL: test_ugm_unaligned_oword
define void @test_ugm_unaligned_oword(<16 x i1> %pred, <16 x i64> %addr, <128 x i8> %data) {
  ; CHECK: [[PRECAST1:%[^ ]+]] = bitcast <128 x i8> %data to <16 x i64>
  ; CHECK: [[LD1:%[^ ]+]] = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> [[PRECAST1]])
  ; CHECK: %ld64 = bitcast <16 x i64> [[LD1]] to <128 x i8>
  %ld64 = call <128 x i8> @llvm.genx.svm.gather.v128i8.v16i1.v16i64(<16 x i1> %pred, i32 3, <16 x i64> %addr, <128 x i8> %data)

  ; CHECK: [[PRECAST6:%[^ ]+]] = bitcast <128 x i8> %data to <16 x i64>
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> [[PRECAST6]])
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v128i8(<16 x i1> %pred, i32 3, <16 x i64> %addr, <128 x i8> %data)

  ret void
}

; CHECK-LABEL: test_ugm
define void @test_ugm(<16 x i1> %pred, <16 x i64> %addr) {
  ; CHECK: %ld1 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> undef)
  ; CHECK: %ld2 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <32 x i32> undef)
  ; CHECK: %ld4 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <64 x i32> undef)
  %ld1 = call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1> %pred, i32 0, <16 x i64> %addr, <16 x i32> undef)
  %ld2 = call <32 x i32> @llvm.genx.svm.gather.v32i32.v16i1.v16i64(<16 x i1> %pred, i32 1, <16 x i64> %addr, <32 x i32> undef)
  %ld4 = call <64 x i32> @llvm.genx.svm.gather.v64i32.v16i1.v16i64(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i32> undef)


  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %ld1)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <32 x i32> %ld2)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v64i32(<16 x i1> %pred, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <64 x i32> %ld4)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1> %pred, i32 0, <16 x i64> %addr, <16 x i32> %ld1)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v32i32(<16 x i1> %pred, i32 1, <16 x i64> %addr, <32 x i32> %ld2)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i32(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i32> %ld4)

  ; CHECK: %lq1 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> undef)
  ; CHECK: %lq2 = call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <32 x i64> undef)
  ; CHECK: %lq4 = call <64 x i64> @llvm.vc.internal.lsc.load.ugm.v64i64.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <64 x i64> undef)
  %lq1 = call <16 x i64> @llvm.genx.svm.gather.v16i64.v16i1.v16i64(<16 x i1> %pred, i32 0, <16 x i64> %addr, <16 x i64> undef)
  %lq2 = call <32 x i64> @llvm.genx.svm.gather.v32i64.v16i1.v16i64(<16 x i1> %pred, i32 1, <16 x i64> %addr, <32 x i64> undef)
  %lq4 = call <64 x i64> @llvm.genx.svm.gather.v64i64.v16i1.v16i64(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i64> undef)


  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %pred, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i64> %lq1)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i64(<16 x i1> %pred, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <32 x i64> %lq2)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v64i64(<16 x i1> %pred, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <64 x i64> %lq4)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1> %pred, i32 0, <16 x i64> %addr, <16 x i64> %lq1)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v32i64(<16 x i1> %pred, i32 1, <16 x i64> %addr, <32 x i64> %lq2)
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i64(<16 x i1> %pred, i32 2, <16 x i64> %addr, <64 x i64> %lq4)

  ret void
}
