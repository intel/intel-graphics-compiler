;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that lsc intrinsics are converted to bindless versions with SSO parameters.

; RUN: %opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-images -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i32.v2i8(<2 x i8>, i32, i32, i32, i32, i32)
declare void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i32(<2 x i8>, i32, i32, i32, i32, i32, <16 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v2i8.v16i32.v16i32(<16 x i1>, <2 x i8>, i8, i32, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)


declare void @sink.v64i32(<64 x i32>)
declare void @sink.v16i32(<16 x i32>)
declare <16 x i1> @src.v16i1()
declare <16 x i32> @src.v16i32()
declare <64 x i32> @src.v64i32()

; CHECK-LABEL: @lsc_load_2d(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: %res = call <16 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bss.v16i32.v2i8(<2 x i8> <i8 1, i8 2>, i32 [[SSO]], i32 2, i32 8, i32 24, i32 42)
define spir_func void @lsc_load_2d(i32 %buf) {
  %res = call <16 x i32> @llvm.vc.internal.lsc.load.2d.tgm.bti.v16i32.v2i8(<2 x i8> <i8 1, i8 2>, i32 %buf, i32 2, i32 8, i32 24, i32 42)
  call void @sink.v16i32(<16 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_store_2d(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.bss.v2i8.v16i32(<2 x i8> <i8 1, i8 2>, i32 [[SSO]], i32 2, i32 8, i32 24, i32 42, <16 x i32> %data)
define spir_func void @lsc_store_2d(i32 %buf) {
  %data = call <16 x i32> @src.v16i32()
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v16i32(<2 x i8> <i8 1, i8 2>, i32 %buf, i32 2, i32 8, i32 24, i32 42, <16 x i32> %data)
  ret void
}

; CHECK-LABEL: @lsc_load_quad_tgm(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: %res = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.bss.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 [[SSO]], <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> undef)
define spir_func void @lsc_load_quad_tgm(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %u = call <16 x i32> @src.v16i32()
  %v = call <16 x i32> @src.v16i32()
  %r = call <16 x i32> @src.v16i32()
  %lod = call <16 x i32> @src.v16i32()
  %res = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 %buf, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> undef)
  call void @sink.v16i32(<16 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_store_quad_tgm(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.store.quad.tgm.bss.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 [[SSO]], <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> %data)
define spir_func void @lsc_store_quad_tgm(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %u = call <16 x i32> @src.v16i32()
  %v = call <16 x i32> @src.v16i32()
  %r = call <16 x i32> @src.v16i32()
  %lod = call <16 x i32> @src.v16i32()
  %data = call <16 x i32> @src.v16i32()
  call void @llvm.vc.internal.lsc.store.quad.tgm.v16i1.v2i8.v16i32.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 %buf, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> %data)
  ret void
}


; CHECK-LABEL: @lsc_prefetch_quad_tgm(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.tgm.bss.v16i1.v2i8.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 [[SSO]], <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
define spir_func void @lsc_prefetch_quad_tgm(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %u = call <16 x i32> @src.v16i32()
  %v = call <16 x i32> @src.v16i32()
  %r = call <16 x i32> @src.v16i32()
  %lod = call <16 x i32> @src.v16i32()
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v16i1.v2i8.v16i32(<16 x i1> %pred, <2 x i8> zeroinitializer, i8 1, i32 %buf, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
  ret void
}
