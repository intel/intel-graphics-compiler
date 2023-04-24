;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that lsc intrinsics are converted to bindless versions with SSO parameters.

; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare <64 x i32> @llvm.genx.lsc.load.quad.bti.v64i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
declare void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <64 x i32>, i32)
declare void @llvm.genx.lsc.store.quad.bti.v16i1.v16i32.v64i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <64 x i32>, i32)
declare <16 x i32> @llvm.genx.lsc.xatomic.bti.v16i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, <16 x i32>, <16 x i32>, i32, <16 x i32>)

declare void @sink.v64i32(<64 x i32>)
declare void @sink.v16i32(<16 x i32>)
declare <16 x i1> @src.v16i1()
declare <16 x i32> @src.v16i32()
declare <64 x i32> @src.v64i32()

; COM: LSC intrinsics.
; COM: ===============
; CHECK-LABEL: @lsc_load(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: [[RES:%[^ ]*]] = call <64 x i32> @llvm.genx.lsc.load.bindless.v64i32.v16i1.v16i32(<16 x i1> [[PRED]], i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> [[ADDRS]], i32 [[SSO]])
; CHECK-NEXT: call void @sink.v64i32(<64 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @lsc_load(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %res = call <64 x i32> @llvm.genx.lsc.load.bti.v64i32.v16i1.v16i32(<16 x i1> %pred, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %addrs, i32 %buf)
  call void @sink.v64i32(<64 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_load_quad(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: [[RES:%[^ ]*]] = call <64 x i32> @llvm.genx.lsc.load.quad.bindless.v64i32.v16i1.v16i32(<16 x i1> [[PRED]], i8 2, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 15, <16 x i32> [[ADDRS]], i32 [[SSO]])
; CHECK-NEXT: call void @sink.v64i32(<64 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @lsc_load_quad(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %res = call <64 x i32> @llvm.genx.lsc.load.quad.bti.v64i32.v16i1.v16i32(<16 x i1> %pred, i8 2, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 15, <16 x i32> %addrs, i32 %buf)
  call void @sink.v64i32(<64 x i32> %res)
  ret void
}

; CHECK-LABEL: @lsc_prefetch(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: call void @llvm.genx.lsc.prefetch.bindless.v16i1.v16i32(<16 x i1> [[PRED]], i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> [[ADDRS]], i32 [[SSO]])
; CHECK-NEXT: ret void
define spir_func void @lsc_prefetch(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  call void @llvm.genx.lsc.prefetch.bti.v16i1.v16i32(<16 x i1> %pred, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %addrs, i32 %buf)
  ret void
}

; CHECK-LABEL: @lsc_store(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: [[DATA:%[^ ]*]] = call <64 x i32> @src.v64i32()
; CHECK-NEXT: call void @llvm.genx.lsc.store.bindless.v16i1.v16i32.v64i32(<16 x i1> [[PRED]], i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> [[ADDRS]], <64 x i32> [[DATA]], i32 [[SSO]])
; CHECK-NEXT: ret void
define spir_func void @lsc_store(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %data = call <64 x i32> @src.v64i32()
  call void @llvm.genx.lsc.store.bti.v16i1.v16i32.v64i32(<16 x i1> %pred, i8 4, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 0, <16 x i32> %addrs, <64 x i32> %data, i32 %buf)
  ret void
}

; CHECK-LABEL: @lsc_store_quad(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: [[DATA:%[^ ]*]] = call <64 x i32> @src.v64i32()
; CHECK-NEXT: call void @llvm.genx.lsc.store.quad.bindless.v16i1.v16i32.v64i32(<16 x i1> [[PRED]], i8 6, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 15, <16 x i32> [[ADDRS]], <64 x i32> [[DATA]], i32 [[SSO]])
; CHECK-NEXT: ret void
define spir_func void @lsc_store_quad(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  %data = call <64 x i32> @src.v64i32()
  call void @llvm.genx.lsc.store.quad.bti.v16i1.v16i32.v64i32(<16 x i1> %pred, i8 6, i8 0, i8 0, i16 1, i32 0, i8 3, i8 4, i8 1, i8 15, <16 x i32> %addrs, <64 x i32> %data, i32 %buf)
  ret void
}

; CHECK-LABEL: @lsc_atomic(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <16 x i1> @src.v16i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <16 x i32> @src.v16i32()
; CHECK-NEXT: [[RES:%[^ ]*]] = call <16 x i32> @llvm.genx.lsc.xatomic.bindless.v16i32.v16i1.v16i32(<16 x i1> [[PRED]], i8 8, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> [[ADDRS]], <16 x i32> undef, <16 x i32> undef, i32 [[SSO]], <16 x i32> undef)
; CHECK-NEXT: call void @sink.v16i32(<16 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @lsc_atomic(i32 %buf) {
  %pred = call <16 x i1> @src.v16i1()
  %addrs = call <16 x i32> @src.v16i32()
  ; Atomic increment.
  %res = call <16 x i32> @llvm.genx.lsc.xatomic.bti.v16i32.v16i1.v16i32(<16 x i1> %pred, i8 8, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %addrs, <16 x i32> undef, <16 x i32> undef, i32 %buf, <16 x i32> undef)
  call void @sink.v16i32(<16 x i32> %res)
  ret void
}
