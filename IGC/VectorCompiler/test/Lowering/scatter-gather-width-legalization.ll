;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

; COM: check each generated instruction as it is because they all related to width legalization functionality.

declare void @llvm.genx.scatter.scaled.v31i1.v31i32.v31i32(<31 x i1>, i32, i16, i32, i32, <31 x i32>, <31 x i32>)

; CHECK-LABEL: @test.scatter.scaled
define void @test.scatter.scaled(i32 %buf, <31 x i1> %pred, <31 x i32> %addr, <31 x i32> %in) {
; CHECK: [[PREDSPLIT:[^ ]*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v31i1(<31 x i1> %pred, i32 0)
; CHECK: [[ADDRSPLIT:[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v31i32.i16(<31 x i32> %addr, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: [[DATASPLIT:[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v31i32.i16(<31 x i32> %in, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v16i1.v16i32.v16i32(<16 x i1> [[PREDSPLIT]], i32 2, i16 0, i32 %buf, i32 0, <16 x i32> [[ADDRSPLIT]], <16 x i32> [[DATASPLIT]])
; CHECK: [[PREDSPLIT1:[^ ]*]] = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v31i1(<31 x i1> %pred, i32 16)
; CHECK: [[ADDRSPLIT2:[^ ]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v31i32.i16(<31 x i32> %addr, i32 0, i32 8, i32 1, i16 64, i32 undef)
; CHECK: [[DATASPLIT3:[^ ]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v31i32.i16(<31 x i32> %in, i32 0, i32 8, i32 1, i16 64, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> [[PREDSPLIT1]], i32 2, i16 0, i32 %buf, i32 0, <8 x i32> [[ADDRSPLIT2]], <8 x i32> [[DATASPLIT3]])
; CHECK: [[PREDSPLIT4:[^ ]*]] = call <4 x i1> @llvm.genx.rdpredregion.v4i1.v31i1(<31 x i1> %pred, i32 24)
; CHECK: [[ADDRSPLIT5:[^ ]*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v31i32.i16(<31 x i32> %addr, i32 0, i32 4, i32 1, i16 96, i32 undef)
; CHECK: [[DATASPLIT6:[^ ]*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v31i32.i16(<31 x i32> %in, i32 0, i32 4, i32 1, i16 96, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v4i1.v4i32.v4i32(<4 x i1> [[PREDSPLIT4]], i32 2, i16 0, i32 %buf, i32 0, <4 x i32> [[ADDRSPLIT5]], <4 x i32> [[DATASPLIT6]])
; CHECK: [[PREDSPLIT7:[^ ]*]] = call <2 x i1> @llvm.genx.rdpredregion.v2i1.v31i1(<31 x i1> %pred, i32 28)
; CHECK: [[ADDRSPLIT8:[^ ]*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v31i32.i16(<31 x i32> %addr, i32 0, i32 2, i32 1, i16 112, i32 undef)
; CHECK: [[DATASPLIT9:[^ ]*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v31i32.i16(<31 x i32> %in, i32 0, i32 2, i32 1, i16 112, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v2i1.v2i32.v2i32(<2 x i1> [[PREDSPLIT7]], i32 2, i16 0, i32 %buf, i32 0, <2 x i32> [[ADDRSPLIT8]], <2 x i32> [[DATASPLIT9]])
; CHECK: [[PREDSPLIT10:[^ ]*]] = call <1 x i1> @llvm.genx.rdpredregion.v1i1.v31i1(<31 x i1> %pred, i32 30)
; CHECK: [[ADDRSPLIT11:[^ ]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v31i32.i16(<31 x i32> %addr, i32 0, i32 1, i32 1, i16 120, i32 undef)
; CHECK: [[DATASPLIT12:[^ ]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v31i32.i16(<31 x i32> %in, i32 0, i32 1, i32 1, i16 120, i32 undef)
; CHECK: call void @llvm.genx.scatter.scaled.v1i1.v1i32.v1i32(<1 x i1> [[PREDSPLIT10]], i32 2, i16 0, i32 %buf, i32 0, <1 x i32> [[ADDRSPLIT11]], <1 x i32> [[DATASPLIT12]])

  tail call void @llvm.genx.scatter.scaled.v31i1.v31i32.v31i32(<31 x i1> %pred, i32 2, i16 0, i32 %buf, i32 0, <31 x i32> %addr, <31 x i32> %in)
  ret void
}

declare void @llvm.genx.svm.scatter.v15i1.v15i32.v15i32(<15 x i1>, i32, <15 x i32>, <15 x i32>)

; CHECK-LABEL: @test.svm.scatter
define void @test.svm.scatter(<15 x i1> %pred, <15 x i32> %addr, <15 x i32> %in) {
; CHECK: [[PREDSPLIT:[^ ]*]] = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v15i1(<15 x i1> %pred, i32 0)
; CHECK: [[ADDRSPLIT:[^ ]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v15i32.i16(<15 x i32> %addr, i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK: [[DATASPLIT:[^ ]*]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v15i32.i16(<15 x i32> %in, i32 0, i32 8, i32 1, i16 0, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v8i1.v8i32.v8i32(<8 x i1> [[PREDSPLIT]], i32 0, <8 x i32> [[ADDRSPLIT]], <8 x i32> [[DATASPLIT]])
; CHECK: [[PREDSPLIT1:[^ ]*]] = call <4 x i1> @llvm.genx.rdpredregion.v4i1.v15i1(<15 x i1> %pred, i32 8)
; CHECK: [[ADDRSPLIT2:[^ ]*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v15i32.i16(<15 x i32> %addr, i32 0, i32 4, i32 1, i16 32, i32 undef)
; CHECK: [[DATASPLIT3:[^ ]*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v15i32.i16(<15 x i32> %in, i32 0, i32 4, i32 1, i16 32, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v4i1.v4i32.v4i32(<4 x i1> [[PREDSPLIT1]], i32 0, <4 x i32> [[ADDRSPLIT2]], <4 x i32> [[DATASPLIT3]])
; CHECK: [[PREDSPLIT4:[^ ]*]] = call <2 x i1> @llvm.genx.rdpredregion.v2i1.v15i1(<15 x i1> %pred, i32 12)
; CHECK: [[ADDRSPLIT5:[^ ]*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v15i32.i16(<15 x i32> %addr, i32 0, i32 2, i32 1, i16 48, i32 undef)
; CHECK: [[DATASPLIT6:[^ ]*]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v15i32.i16(<15 x i32> %in, i32 0, i32 2, i32 1, i16 48, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v2i1.v2i32.v2i32(<2 x i1> [[PREDSPLIT4]], i32 0, <2 x i32> [[ADDRSPLIT5]], <2 x i32> [[DATASPLIT6]])
; CHECK: [[PREDSPLIT7:[^ ]*]] = call <1 x i1> @llvm.genx.rdpredregion.v1i1.v15i1(<15 x i1> %pred, i32 14)
; CHECK: [[ADDRSPLIT8:[^ ]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v15i32.i16(<15 x i32> %addr, i32 0, i32 1, i32 1, i16 56, i32 undef)
; CHECK: [[DATASPLIT9:[^ ]*]] = call <1 x i32> @llvm.genx.rdregioni.v1i32.v15i32.i16(<15 x i32> %in, i32 0, i32 1, i32 1, i16 56, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i32.v1i32(<1 x i1> [[PREDSPLIT7]], i32 0, <1 x i32> [[ADDRSPLIT8]], <1 x i32> [[DATASPLIT9]])

  tail call void @llvm.genx.svm.scatter.v15i1.v15i32.v15i32(<15 x i1> %pred, i32 0, <15 x i32> %addr, <15 x i32> %in)
  ret void
}

declare void @llvm.genx.svm.scatter.v5i1.v5i64.v20i32(<5 x i1>, i32, <5 x i64>, <20 x i32>)

; CHECK-LABEL: @test.svm.scatter.2blocks
define void @test.svm.scatter.2blocks(<5 x i1> %pred, <5 x i64> %addr, <20 x i32> %in) {
; CHECK: [[PREDSPLIT:[^ ]*]] = call <4 x i1> @llvm.genx.rdpredregion.v4i1.v5i1(<5 x i1> %pred, i32 0)
; CHECK: [[ADDRSPLIT:[^ ]*]] = call <4 x i64> @llvm.genx.rdregioni.v4i64.v5i64.i16(<5 x i64> %addr, i32 0, i32 4, i32 1, i16 0, i32 undef)
; CHECK: [[DATASPLIT:[^ ]*]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v20i32.i16(<20 x i32> %in, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v4i1.v4i64.v16i32(<4 x i1> [[PREDSPLIT]], i32 2, <4 x i64> [[ADDRSPLIT]], <16 x i32> [[DATASPLIT]])
; CHECK: [[PREDSPLIT1:[^ ]*]] = call <1 x i1> @llvm.genx.rdpredregion.v1i1.v5i1(<5 x i1> %pred, i32 4)
; CHECK: [[ADDRSPLIT2:[^ ]*]] = call <1 x i64> @llvm.genx.rdregioni.v1i64.v5i64.i16(<5 x i64> %addr, i32 0, i32 1, i32 1, i16 32, i32 undef)
; CHECK: [[DATASPLIT3:[^ ]*]] = call <4 x i32> @llvm.genx.rdregioni.v4i32.v20i32.i16(<20 x i32> %in, i32 0, i32 4, i32 1, i16 64, i32 undef)
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i64.v4i32(<1 x i1> [[PREDSPLIT1]], i32 2, <1 x i64> [[ADDRSPLIT2]], <4 x i32> [[DATASPLIT3]])

  tail call void @llvm.genx.svm.scatter.v5i1.v5i64.v20i32(<5 x i1> %pred, i32 2, <5 x i64> %addr, <20 x i32> %in)
  ret void
}

declare <32 x i32> @llvm.genx.svm.gather.v32i32.v32i1.v32i64(<32 x i1>, i32, <32 x i64>, <32 x i32>)

; CHECK-LABEL: @test.svm.gather
define <32 x i32> @test.svm.gather(<32 x i1> %pred, <32 x i64> %addr) {
; CHECK: [[PREDSPLIT:[^ ]*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
; CHECK: [[ADDRSPLIT:[^ ]*]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: [[RETSPLIT:[^ ]*]] = call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1> [[PREDSPLIT]], i32 0, <16 x i64> [[ADDRSPLIT]], <16 x i32> undef)
; CHECK: [[JOIN:[^ ]*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[RETSPLIT]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[PREDSPLIT1:[^ ]*]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
; CHECK: [[ADDRSPLIT2:[^ ]*]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 0, i32 16, i32 1, i16 128, i32 undef)
; CHECK: [[RETSPLIT3:[^ ]*]] = call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64(<16 x i1> [[PREDSPLIT1]], i32 0, <16 x i64> [[ADDRSPLIT2]], <16 x i32> undef)
; CHECK: [[JOIN4:[^ ]*]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[JOIN]], <16 x i32> [[RETSPLIT3]], i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK: ret <32 x i32> [[JOIN4]]

  %ret = tail call <32 x i32> @llvm.genx.svm.gather.v32i32.v32i1.v32i64(<32 x i1> %pred, i32 0, <32 x i64> %addr, <32 x i32> undef)
  ret <32 x i32> %ret
}
