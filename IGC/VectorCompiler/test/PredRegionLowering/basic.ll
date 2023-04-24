;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -FunctionGroupAnalysis -GenXGroupBalingWrapper \
; RUN: -GenXPredRegionLowering -vc-lower-predregion=true -march=genx64 -mcpu=Gen9 \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1>, i32)
declare <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1>, <16 x i1>, i32)

define spir_func void @test_lone_rdpredregion(<32 x i1> %pred) #0 {
  %pred.rdpr = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
  ret void

; CHECK-LABEL: test_lone_rdpredregion
; CHECK: %[[rdpr:.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK: %[[sel:.*]] = select <16 x i1> %[[rdpr]], <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <16 x i16> zeroinitializer
; CHECK: %[[cmp:.*]] = icmp eq <16 x i16> %[[sel]], <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>
}

define spir_func void @test_lone_wrpredregion(<32 x i1> %pred, <16 x i1> %pred.new) #0 {
  %pred.wrpr = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1> %pred, <16 x i1> %pred.new, i32 16)
  ret void

; CHECK-LABEL: test_lone_wrpredregion
; CHECK: %[[sel:.*]] = select <16 x i1> %pred.new, <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <16 x i16> zeroinitializer
; CHECK: %[[cmp:.*]] = icmp eq <16 x i16> %[[sel]], <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>
; CHECK: %[[wrpr:.*]] = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1> %pred, <16 x i1> %[[cmp]], i32 16)
}

define spir_func void @test_rd_and_wrpredregion_bale(<32 x i1> %pred, <32 x i1> %pred2) #0 {
  %pred.rdpr = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
  %pred.wrpr = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1> %pred2, <16 x i1> %pred.rdpr, i32 16)
  ret void

; CHECK-LABEL: test_rd_and_wrpredregion_bale
; CHECK: %[[rdpr:.*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK: %[[sel:.*]] = select <16 x i1> %[[rdpr]], <16 x i16> <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>, <16 x i16> zeroinitializer
; CHECK: %[[cmp:.*]] = icmp eq <16 x i16> %[[sel]], <i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1, i16 1>
; CHECK: %[[wrpr:.*]] = call <32 x i1> @llvm.genx.wrpredregion.v32i1.v16i1(<32 x i1> %pred2, <16 x i1> %[[cmp]], i32 16)
}

attributes #0 = { "CMStackCall" }
