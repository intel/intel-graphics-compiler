;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

declare <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <1 x i32>)
declare <2 x i32> @llvm.vc.internal.lsc.load.ugm.v2i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <2 x i32>)
declare <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <3 x i32>)
declare <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <4 x i32>)
declare <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <8 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <16 x i32>)
declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <32 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <64 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v1i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <1 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v2i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <2 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v3i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <3 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v4i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <4 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v8i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v32i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v64i32.v1i1.v1i64(<1 x i1>, i8, i8, i8, i8, i8, i64, <1 x i64>, i16, i32, <64 x i32>)

define void @test_i32(<1 x i64> %addr) {
  ; CHECK: %v1 = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <1 x i32> undef)
  ; CHECK: %v2 = call <2 x i32> @llvm.vc.internal.lsc.load.ugm.v2i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <2 x i32> undef)
  ; CHECK: %v3 = call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <3 x i32> undef)
  ; CHECK: %v4 = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <4 x i32> undef)
  ; CHECK: %v8 = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <8 x i32> undef)
  ; CHECK: %v16 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <16 x i32> undef)
  ; CHECK: %v32 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <32 x i32> undef)
  ; CHECK: %v64 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <64 x i32> undef)
  %v1 = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <1 x i32> undef)
  %v2 = call <2 x i32> @llvm.vc.internal.lsc.load.ugm.v2i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <2 x i32> undef)
  %v3 = call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <3 x i32> undef)
  %v4 = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <4 x i32> undef)
  %v8 = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <8 x i32> undef)
  %v16 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <16 x i32> undef)
  %v32 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <32 x i32> undef)
  %v64 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <64 x i32> undef)

  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <1 x i32> %v1)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v2i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <2 x i32> %v2)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v3i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <3 x i32> %v3)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v4i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <4 x i32> %v4)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v8i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <8 x i32> %v8)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <16 x i32> %v16)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v32i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <32 x i32> %v32)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v64i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <64 x i32> %v64)

  call void @llvm.vc.internal.lsc.store.ugm.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <1 x i32> %v1)
  call void @llvm.vc.internal.lsc.store.ugm.v2i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <2 x i32> %v2)
  call void @llvm.vc.internal.lsc.store.ugm.v3i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <3 x i32> %v3)
  call void @llvm.vc.internal.lsc.store.ugm.v4i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <4 x i32> %v4)
  call void @llvm.vc.internal.lsc.store.ugm.v8i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <8 x i32> %v8)
  call void @llvm.vc.internal.lsc.store.ugm.v16i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <16 x i32> %v16)
  call void @llvm.vc.internal.lsc.store.ugm.v32i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <32 x i32> %v32)
  call void @llvm.vc.internal.lsc.store.ugm.v64i32.v1i1.v1i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, i8 0, i8 0, i64 0, <1 x i64> %addr, i16 1, i32 0, <64 x i32> %v64)

  ret void
}
