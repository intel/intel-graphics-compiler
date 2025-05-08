;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

declare <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <32 x i16>)
declare <16 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i8.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i8>)
declare <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)
declare <65 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v65i8.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <65 x i8>)
declare <80 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v80i16.v2i8(i1, i8, <2 x i8>, i8, i16, i16, i64, i32, i32, i32, i32, i32, i32, i32, <80 x i16>)

; CHECK-LABEL: @test_load(
define <80 x i16> @test_load(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, <80 x i16> %passthru) {

; CHECK: [[WRREG:%[^ ]+]] = call <96 x i16> @llvm.genx.wrregioni.v96i16.v80i16.i16.i1(<96 x i16> undef, <80 x i16> %passthru, i32 1, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: [[LOAD:%[^ ]+]] = call <96 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v96i16.v2i8(i1 true, i8 2, <2 x i8> zeroinitializer, i8 1, i16 6, i16 10, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <96 x i16> [[WRREG]])
; CHECK: [[RDREG:%[^ ]+]] = call <80 x i16> @llvm.genx.rdregioni.v80i16.v96i16.i16(<96 x i16> [[LOAD]], i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: ret <80 x i16> [[RDREG]]

  %load = call <80 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.vnni.v80i16.v2i8(i1 true, i8 2, <2 x i8> zeroinitializer, i8 1, i16 6, i16 10, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <80 x i16> %passthru)
  ret <80 x i16> %load
}

; CHECK-LABEL: @test_load_whole_grf(
define <32 x i16> @test_load_whole_grf(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
; CHECK: [[LOAD:%[^ ]+]] = call <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 4, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <32 x i16> undef)
; CHECK: ret <32 x i16> [[LOAD]]

  %load = call <32 x i16> @llvm.vc.internal.lsc.load.block.2d.ugm.v32i16.v2i8(i1 true, i8 3, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 4, i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y, i32 0, i32 0, <32 x i16> undef)
  ret <32 x i16> %load
}

; CHECK-LABEL: test_load_desc(
define <16 x i8> @test_load_desc(<16 x i32> %addr) {
; CHECK: [[WRREG:%[^ ]+]] = call <64 x i8> @llvm.genx.wrregioni.v64i8.v16i8.i16.i1(<64 x i8> undef, <16 x i8> zeroinitializer, i32 1, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: [[LOAD:%[^ ]+]] = call <64 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.v64i8.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 16, i16 1, <16 x i32> %addr, i32 0, i32 0, <64 x i8> [[WRREG]])
; CHECK: [[RDREG:%[^ ]+]] = call <16 x i8> @llvm.genx.rdregioni.v16i8.v64i8.i16(<64 x i8> [[LOAD]], i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: ret <16 x i8> [[RDREG]]

  %load = call <16 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i8.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 16, i16 1, <16 x i32> %addr, i32 0, i32 0, <16 x i8> zeroinitializer)
  ret <16 x i8> %load
}

; CHECK-LABEL: test_load_desc_whole_grf(
define <16 x i32> @test_load_desc_whole_grf(<16 x i32> %addr) {
; CHECK: [[LOAD:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 16, i16 1, <16 x i32> %addr, i32 0, i32 0, <16 x i32> zeroinitializer)
; CHECK: ret <16 x i32> [[LOAD]]

  %load = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 16, i16 1, <16 x i32> %addr, i32 0, i32 0, <16 x i32> zeroinitializer)
  ret <16 x i32> %load
}

; CHECK-LABEL: test_load_desc_transpose_undef(
define <65 x i8> @test_load_desc_transpose_undef(<16 x i32> %addr) {
; CHECK: [[LOAD:%[^ ]+]] = call <128 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v128i8.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 13, i16 5, <16 x i32> %addr, i32 0, i32 0, <128 x i8> undef)
; CHECK: [[RDREG:%[^ ]+]] = call <65 x i8> @llvm.genx.rdregioni.v65i8.v128i8.i16(<128 x i8> %1, i32 1, i32 1, i32 0, i16 0, i32 undef)
; CHECK: ret <65 x i8> [[RDREG]]

  %load = call <65 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v65i8.v2i8(i1 true, <2 x i8> <i8 2, i8 2>, i8 1, i16 13, i16 5, <16 x i32> %addr, i32 0, i32 0, <65 x i8> undef)
  ret <65 x i8> %load
}
