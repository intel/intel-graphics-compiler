;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=Xe2 -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <128 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v128i32.v64i1.v64i32(<64 x i1>, i8, i8, i8, i32, <64 x i32>, <64 x i32>, <64 x i32>, <64 x i32>, <128 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v64i1.v64i32.v128i32(<64 x i1>, i8, i8, i8, i32, <64 x i32>, <64 x i32>, <64 x i32>, <64 x i32>, <128 x i32>)
declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.v64i1.v64i32(<64 x i1>, i8, i8, i8, i32, <64 x i32>, <64 x i32>, <64 x i32>, <64 x i32>)

declare <66 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v66i32.v33i1.v33i32(<33 x i1>, i8, i8, i8, i32, <33 x i32>, <33 x i32>, <33 x i32>, <33 x i32>, <66 x i32>)

define <128 x i32> @test_load_split(<64 x i1> %pred, <64 x i32> %u, <128 x i32> %src) {
  ; CHECK: [[PRED0:%[^ ]+]] = call <32 x i1> @llvm.genx.rdpredregion.v32i1.v64i1(<64 x i1> %pred, i32 0)
  ; CHECK: [[LU0:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: [[LSRC0:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src, i32 64, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: [[LOAD0:%[^ ]+]] = call <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v64i32.v32i1.v32i32(<32 x i1> [[PRED0]], i8 0, i8 0, i8 3, i32 1, <32 x i32> [[LU0]],
  ; CHECK-SAME:  <64 x i32> [[LSRC0]])
  ; CHECK: [[INS0:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v64i32.i16.i1(<128 x i32> undef, <64 x i32> [[LOAD0]], i32 64, i32 32, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[PRED32:%[^ ]+]] = call <32 x i1> @llvm.genx.rdpredregion.v32i1.v64i1(<64 x i1> %pred, i32 32)
  ; CHECK: [[LU32:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK: [[LSRC32:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src, i32 64, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK: [[LOAD32:%[^ ]+]] = call <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v64i32.v32i1.v32i32(<32 x i1> [[PRED32]], i8 0, i8 0, i8 3, i32 1, <32 x i32> [[LU32]],
  ; CHECK-SAME:  <64 x i32> [[LSRC32]])
  ; CHECK: %res = call <128 x i32> @llvm.genx.wrregioni.v128i32.v64i32.i16.i1(<128 x i32> [[INS0]], <64 x i32> [[LOAD32]], i32 64, i32 32, i32 1, i16 128, i32 undef, i1 true)
  %res = call <128 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v128i32.v64i1.v64i32(<64 x i1> %pred, i8 0, i8 0, i8 3, i32 1, <64 x i32> %u, <64 x i32> undef, <64 x i32> undef, <64 x i32> undef, <128 x i32> %src)
  ret <128 x i32> %res
}

define void @test_store_split(<64 x i1> %pred, <64 x i32> %u, <128 x i32> %src) {
  ; CHECK: [[SU0:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: [[SRC0:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src, i32 64, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.tgm.v32i1.v32i32.v64i32(<32 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <32 x i32> [[SU0]],
  ; CHECK-SAME:  <64 x i32> [[SRC0]])
  ; CHECK: [[SU32:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK: [[SRC32:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %src, i32 64, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.tgm.v32i1.v32i32.v64i32(<32 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <32 x i32> [[SU32]],
  ; CHECK-SAME:  <64 x i32> [[SRC32]])
  call void @llvm.vc.internal.lsc.store.quad.tgm.v64i1.v64i32.v128i32(<64 x i1> %pred, i8 0, i8 0, i8 3, i32 1, <64 x i32> %u, <64 x i32> undef, <64 x i32> undef, <64 x i32> undef, <128 x i32> %src)
  ret void
}

define void @test_prefetch_split(<64 x i1> %pred, <64 x i32> %u) {
  ; CHECK: [[PU0:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v32i1.v32i32(<32 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <32 x i32> [[PU0]],
  ; CHECK: [[PU32:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %u, i32 64, i32 32, i32 1, i16 128, i32 undef)
  ; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v32i1.v32i32(<32 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <32 x i32> [[PU32]],
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v64i1.v64i32(<64 x i1> %pred, i8 0, i8 0, i8 3, i32 1, <64 x i32> %u, <64 x i32> undef, <64 x i32> undef, <64 x i32> undef)
  ret void
}

define <66 x i32> @test_load_split_tail(<33 x i1> %pred, <33 x i32> %u, <66 x i32> %src) {
  ; CHECK: [[U0:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v33i32.i16(<33 x i32> %u, i32 33, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: [[SRC0:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v66i32.i16(<66 x i32> %src, i32 33, i32 32, i32 1, i16 0, i32 undef)
  ; CHECK: [[TLOAD0:%[^ ]+]] = call <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v64i32.v32i1.v32i32(<32 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <32 x i32> [[U0]],
  ; CHECK-SAME:  <64 x i32> [[SRC0]])
  ; CHECK: [[INS0:%[^ ]+]] = call <66 x i32> @llvm.genx.wrregioni.v66i32.v64i32.i16.i1(<66 x i32> undef, <64 x i32> [[TLOAD0]], i32 33, i32 32, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[TLOAD32:%[^ ]+]] = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v32i32.v16i1.v16i32(<16 x i1> %{{[^,]+}}, i8 0, i8 0, i8 3, i32 1, <16 x i32> %{{[^,]+}},
  ; CHECK: [[LOADEXTR:%[^ ]+]] = call <2 x i32> @llvm.genx.rdregioni.v2i32.v32i32.i16(<32 x i32> [[TLOAD32]], i32 16, i32 1, i32 1, i16 0, i32 undef)
  ; CHECK: %res = call <66 x i32> @llvm.genx.wrregioni.v66i32.v2i32.i16.i1(<66 x i32> [[INS0]], <2 x i32> [[LOADEXTR]], i32 33, i32 1, i32 1, i16 128, i32 undef, i1 true)
  %res = call <66 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v66i32.v33i1.v33i32(<33 x i1> %pred, i8 0, i8 0, i8 3, i32 1, <33 x i32> %u, <33 x i32> undef, <33 x i32> undef, <33 x i32> undef, <66 x i32> %src)
  ret <66 x i32> %res
}
