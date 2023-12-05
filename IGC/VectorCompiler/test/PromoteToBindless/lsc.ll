;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that lsc intrinsics are converted to bindless versions with SSO parameters.

; RUN: %opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <64 x i32> @llvm.vc.internal.lsc.load.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.quad.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)
declare void @llvm.vc.internal.lsc.prefetch.bti.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32)
declare void @llvm.vc.internal.lsc.prefetch.quad.bti.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32)
declare void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <16 x i32>, <16 x i32>, <16 x i32>)

declare void @sink.v64i32(<64 x i32>)
declare void @sink.v16i32(<16 x i32>)
declare <16 x i1> @src.v16i1()
declare <16 x i32> @src.v16i32()
declare <64 x i32> @src.v64i32()

; COM: LSC intrinsics.
; COM: ===============
; CHECK-LABEL: @lsc_load(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: %res = call <64 x i32> @llvm.vc.internal.lsc.load.bss.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0, <64 x i32> undef)
define spir_func void @lsc_load(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %res = call <64 x i32> @llvm.vc.internal.lsc.load.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> undef)
  call void @sink.v64i32(<64 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_load_quad(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: %res = call <64 x i32> @llvm.vc.internal.lsc.load.quad.bss.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0, <64 x i32> undef)
define spir_func void @lsc_load_quad(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %res = call <64 x i32> @llvm.vc.internal.lsc.load.quad.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> undef)
  call void @sink.v64i32(<64 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_prefetch(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.prefetch.bss.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0)
define spir_func void @lsc_prefetch(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  call void @llvm.vc.internal.lsc.prefetch.bti.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0)
  ret void
}

; CHECK-LABEL: @lsc_prefetch_quad(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.bss.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0)
define spir_func void @lsc_prefetch_quad(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  call void @llvm.vc.internal.lsc.prefetch.quad.bti.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0)
  ret void
}

; CHECK-LABEL: @lsc_store(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.store.bss.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %data)
define spir_func void @lsc_store(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %data = call <64 x i32> @src.v64i32()
  call void @llvm.vc.internal.lsc.store.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %data)
  ret void
}

; CHECK-LABEL: @lsc_store_quad(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: call void @llvm.vc.internal.lsc.store.quad.bss.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %data)
define spir_func void @lsc_store_quad(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %data = call <64 x i32> @src.v64i32()
  call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %data)
  ret void
}

; CHECK-LABEL: @lsc_atomic(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK: %res = call <16 x i32> @llvm.vc.internal.lsc.atomic.bss.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 [[SSO]], <16 x i32> %addrs, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
define spir_func void @lsc_atomic(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  ; Atomic increment.
  %res = call <16 x i32> @llvm.vc.internal.lsc.atomic.bti.v16i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 8, i8 2, i8 3, <2 x i8> zeroinitializer, i32 %buf, <16 x i32> %addrs, i16 1, i32 0, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
  call void @sink.v16i32(<16 x i32> %res)
  ret void
}
