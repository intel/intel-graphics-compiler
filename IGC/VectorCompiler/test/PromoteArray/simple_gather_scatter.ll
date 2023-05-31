;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare <8 x i1> @get_pred()
declare <8 x float> @get_data()
declare <8 x i32> @get_data_i()
declare void @llvm.masked.scatter.v8p0f32.v8f32(<8 x float>, <8 x float*>, i32, <8 x i1>)
declare void @llvm.masked.scatter.v8p0i32.v8i32(<8 x i32>, <8 x i32*>, i32, <8 x i1>)
declare <2 x float> @llvm.masked.gather.v2f32.v2p0f32(<2 x float*>, i32, <2 x i1>, <2 x float>)

define dllexport void @f_f_st() {
  %alloca = alloca [8 x float], align 64
  ; CHECK: %[[ALLOCA_0:[^ ]+]] = alloca <8 x float>
  %pred = call <8 x i1> @get_pred()
  %data = call <8 x float> @get_data()
  %ptrtoint = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: insertelement <8 x i64> undef, i64 0, i32 0
  %inselem0 = insertelement <8 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <8 x i64> %inselem0, <8 x i64> undef, <8 x i32> zeroinitializer
  %address = add <8 x i64> %base, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28>
  %ptr = inttoptr <8 x i64> %address to <8 x float*>
  call void @llvm.masked.scatter.v8p0f32.v8f32(<8 x float> %data, <8 x float*> %ptr, i32 4, <8 x i1> %pred)
  ; CHECK: %[[LD_0:[^ ]+]] = load <8 x float>, <8 x float>* %[[ALLOCA_0]]
  ; CHECK: %[[ADDR_0:[^ ]+]] = trunc <8 x i64> %{{.*}} to <8 x i16>
  ; CHECK: %[[WRR_0:[^ ]+]] = call <8 x float> @llvm.genx.wrregionf.v8f32.v8f32.v8i16.v8i1(<8 x float> %[[LD_0]], <8 x float> %data, i32 0, i32 1, i32 0, <8 x i16> %[[ADDR_0]], i32 0, <8 x i1> %pred)
  ; CHECK: store <8 x float> %[[WRR_0]], <8 x float>* %[[ALLOCA_0]]
  ret void
}

define dllexport <2 x float> @f_f_ld([8 x float] %input, <2 x i64> %offsets, <2 x i1> %pred) {
  %alloca = alloca [8 x float], align 64
  ; CHECK: %[[ALLOCA_1:[^ ]+]] = alloca <8 x float>
  store [8 x float] %input, [8 x float]* %alloca
  %ptrtoint = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: insertelement <2 x i64> undef, i64 0, i32 0
  %inselem0 = insertelement <2 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <2 x i64> %inselem0, <2 x i64> undef, <2 x i32> zeroinitializer
  %address = add <2 x i64> %base, %offsets
  %ptr = inttoptr <2 x i64> %address to <2 x float*>
  ; CHECK: %[[LD_1:[^ ]+]] = load <8 x float>, <8 x float>* %[[ALLOCA_1]]
  ; CHECK: %[[ADDR_1:[^ ]+]] = trunc <2 x i64> %{{.*}} to <2 x i16>
  ; CHECK: %[[RES:[^ ]+]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v8f32.v2i16(<8 x float> %[[LD_1]], i32 0, i32 1, i32 0, <2 x i16> %[[ADDR_1]], i32 0)
  ; CHECK: ret <2 x float> %[[RES]]
  %res = call <2 x float> @llvm.masked.gather.v2f32.v2p0f32(<2 x float*> %ptr, i32 4, <2 x i1> %pred, <2 x float> undef)
  ret <2 x float> %res
}

define dllexport <2 x float> @f_f_ld_masksplat([8 x float] %input, <2 x i64> %offsets, <2 x float> %src) {
  %alloca = alloca [8 x float], align 64
  ; CHECK: %[[ALLOCA_1:[^ ]+]] = alloca <8 x float>
  store [8 x float] %input, [8 x float]* %alloca
  %ptrtoint = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: insertelement <2 x i64> undef, i64 0, i32 0
  %inselem0 = insertelement <2 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <2 x i64> %inselem0, <2 x i64> undef, <2 x i32> zeroinitializer
  %address = add <2 x i64> %base, %offsets
  %ptr = inttoptr <2 x i64> %address to <2 x float*>
  ; CHECK: %[[LD_1:[^ ]+]] = load <8 x float>, <8 x float>* %[[ALLOCA_1]]
  ; CHECK: %[[ADDR_1:[^ ]+]] = trunc <2 x i64> %{{.*}} to <2 x i16>
  ; CHECK: %[[RES:[^ ]+]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v8f32.v2i16(<8 x float> %[[LD_1]], i32 0, i32 1, i32 0, <2 x i16> %[[ADDR_1]], i32 0)
  ; CHECK: ret <2 x float> %[[RES]]
  %res = call <2 x float> @llvm.masked.gather.v2f32.v2p0f32(<2 x float*> %ptr, i32 4, <2 x i1> <i1 true, i1 true>, <2 x float> %src)
  ret <2 x float> %res
}

define dllexport <2 x float> @f_f_ld_maskconst([8 x float] %input, <2 x i64> %offsets, <2 x float> %src) {
  %alloca = alloca [8 x float], align 64
  ; CHECK: %[[ALLOCA_1:[^ ]+]] = alloca <8 x float>
  store [8 x float] %input, [8 x float]* %alloca
  %ptrtoint = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: insertelement <2 x i64> undef, i64 0, i32 0
  %inselem0 = insertelement <2 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <2 x i64> %inselem0, <2 x i64> undef, <2 x i32> zeroinitializer
  %address = add <2 x i64> %base, %offsets
  %ptr = inttoptr <2 x i64> %address to <2 x float*>
  ; CHECK: %[[LD_1:[^ ]+]] = load <8 x float>, <8 x float>* %[[ALLOCA_1]]
  ; CHECK: %[[ADDR_1:[^ ]+]] = trunc <2 x i64> %{{.*}} to <2 x i16>
  ; CHECK: %[[RDRGN:[^ ]+]] = call <2 x float> @llvm.genx.rdregionf.v2f32.v8f32.v2i16(<8 x float> %[[LD_1]], i32 0, i32 1, i32 0, <2 x i16> %[[ADDR_1]], i32 0)
  ; CHECK: %[[SEL:[^ ]+]] = select <2 x i1> <i1 false, i1 true>, <2 x float> %[[RDRGN]], <2 x float> %src
  ; CHECK: ret <2 x float> %[[SEL]]
  %res = call <2 x float> @llvm.masked.gather.v2f32.v2p0f32(<2 x float*> %ptr, i32 4, <2 x i1> <i1 false, i1 true>, <2 x float> %src)
  ret <2 x float> %res
}

; this tests if we ignore allocas that are already vectors of a proper type
define dllexport void @f_f_same() {
  %alloca = alloca <8 x float>, align 64
  ; CHECK: %[[ALLOCA_2:[^ ]+]] = alloca <8 x float>
  %pred = call <8 x i1> @get_pred()
  %data = call <8 x float> @get_data()
  %ptrtoint = ptrtoint <8 x float>* %alloca to i64
  ; CHECK: %[[PTI:[^ ]+]] = ptrtoint <8 x float>* %alloca to i64
  ; CHECK: insertelement <8 x i64> undef, i64 %[[PTI]], i32 0
  %inselem0 = insertelement <8 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <8 x i64> %inselem0, <8 x i64> undef, <8 x i32> zeroinitializer
  %address = add <8 x i64> %base, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28>
  %ptr = inttoptr <8 x i64> %address to <8 x float*>
  call void @llvm.masked.scatter.v8p0f32.v8f32(<8 x float> %data, <8 x float*> %ptr, i32 4, <8 x i1> %pred)
  ; CHECK: masked.scatter
  ret void
}

define dllexport void @f_f_mismatch() {
  %alloca = alloca [8 x float], align 64
  ; CHECK: %[[ALLOCA_3:[^ ]+]] = alloca [8 x float]
  %pred = call <8 x i1> @get_pred()
  %data = call <8 x i32> @get_data_i()
  %ptrtoint = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: %[[PTI:[^ ]+]] = ptrtoint [8 x float]* %alloca to i64
  ; CHECK: insertelement <8 x i64> undef, i64 %[[PTI]], i32 0
  %inselem0 = insertelement <8 x i64> undef, i64 %ptrtoint, i32 0
  %base = shufflevector <8 x i64> %inselem0, <8 x i64> undef, <8 x i32> zeroinitializer
  %address = add <8 x i64> %base, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28>
  %ptr = inttoptr <8 x i64> %address to <8 x i32*>
  call void @llvm.masked.scatter.v8p0i32.v8i32(<8 x i32> %data, <8 x i32*> %ptr, i32 4, <8 x i1> %pred)
  ; CHECK: masked.scatter
  ret void
}
