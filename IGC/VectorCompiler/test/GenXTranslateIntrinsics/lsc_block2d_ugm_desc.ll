;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% %pass_pref%GenXTranslateIntrinsics -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

declare <16 x i32> @llvm.genx.lsc.load.2d.ugm.desc.v16i32.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)
declare <32 x i16> @llvm.genx.lsc.load.2d.ugm.desc.v32i16.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <32 x i16>)
declare <16 x i32> @llvm.genx.lsc.load.2d.ugm.desc.transpose.v16i32.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)
declare <64 x i8> @llvm.genx.lsc.load.2d.ugm.desc.vnni.v64i8.v2i8(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <64 x i8>)

declare void @llvm.genx.lsc.prefetch.2d.ugm.desc.v2i8.i64(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, i64)

declare void @llvm.genx.lsc.store.2d.ugm.desc.v2i8.v16i32(i1, <2 x i8>, i8, i16, i16, <16 x i32>, i32, i32, <16 x i32>)

define void @test(i64 %base, i32 %width, i32 %height, i32 %pitch, i32 %x, i32 %y) {
  %vbase = bitcast i64 %base to <2 x i32>
  %1 = shufflevector <2 x i32> %vbase, <2 x i32> undef, <16 x i32> <i32 0, i32 1, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2, i32 2>
  %2 = insertelement <16 x i32> %1, i32 %width, i32 2
  %3 = insertelement <16 x i32> %2, i32 %height, i32 3
  %4 = insertelement <16 x i32> %3, i32 %pitch, i32 4
  %5 = insertelement <16 x i32> %4, i32 %x, i32 5
  %desc = insertelement <16 x i32> %5, i32 %y, i32 6

  %desc.1 = insertelement <16 x i32> %desc, i32 263, i32 7 ; 8x2
  ; CHECK: %load = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.v16i32.v2i8(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.1, i32 0, i32 0, <16 x i32> undef)
  %load = call <16 x i32> @llvm.genx.lsc.load.2d.ugm.desc.v16i32.v2i8(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.1, i32 0, i32 0, <16 x i32> undef)

  %desc.2 = insertelement <16 x i32> %desc.1, i32 65799, i32 7 ; 2x8x2
  ; CHECK: %load.a2 = call <32 x i16> @llvm.vc.internal.lsc.load.2d.ugm.desc.v32i16.v2i8(i1 true, <2 x i8> <i8 2, i8 1>, i8 2, i16 8, i16 2, <16 x i32> %desc.2, i32 0, i32 0, <32 x i16> undef)
  %load.a2 = call <32 x i16> @llvm.genx.lsc.load.2d.ugm.desc.v32i16.v2i8(i1 true, <2 x i8> <i8 2, i8 1>, i8 2, i16 8, i16 2, <16 x i32> %desc.2, i32 0, i32 0, <32 x i16> undef)

  %desc.3 = insertelement <16 x i32> %desc.2, i32 1793, i32 7 ; 1x2x8
  ; CHECK: %load.t = call <16 x i32> @llvm.vc.internal.lsc.load.2d.ugm.desc.transpose.v16i32.v2i8(i1 true, <2 x i8> <i8 5, i8 1>, i8 1, i16 2, i16 8, <16 x i32> %desc.3, i32 0, i32 0, <16 x i32> undef)
  %load.t = call <16 x i32> @llvm.genx.lsc.load.2d.ugm.desc.transpose.v16i32.v2i8(i1 true, <2 x i8> <i8 5, i8 1>, i8 1, i16 2, i16 8, <16 x i32> %desc.3, i32 0, i32 0, <16 x i32> undef)

  %desc.4 = insertelement <16 x i32> %desc.3, i32 3843, i32 7 ; 1x4x16
  ; CHECK: %load.v = call <64 x i8> @llvm.vc.internal.lsc.load.2d.ugm.desc.vnni.v64i8.v2i8(i1 true, <2 x i8> <i8 5, i8 2>, i8 1, i16 4, i16 16, <16 x i32> %desc.4, i32 0, i32 0, <64 x i8> undef)
  %load.v = call <64 x i8> @llvm.genx.lsc.load.2d.ugm.desc.vnni.v64i8.v2i8(i1 true, <2 x i8> <i8 5, i8 2>, i8 1, i16 4, i16 16, <16 x i32> %desc.4, i32 0, i32 0, <64 x i8> undef)

  %desc.5 = insertelement <16 x i32> %desc.4, i32 263, i32 7 ; 1x8x2
  ; CHECK: call void @llvm.vc.internal.lsc.prefetch.2d.ugm.desc.v2i8.i64(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, i64 undef)
  call void @llvm.genx.lsc.prefetch.2d.ugm.desc.v2i8.i64(i1 true, <2 x i8> <i8 1, i8 2>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, i64 undef)

  ; CHECK: call void @llvm.vc.internal.lsc.store.2d.ugm.desc.v2i8.v16i32(i1 true, <2 x i8> <i8 4, i8 3>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, <16 x i32> %load)
  call void @llvm.genx.lsc.store.2d.ugm.desc.v2i8.v16i32(i1 true, <2 x i8> <i8 4, i8 3>, i8 1, i16 8, i16 2, <16 x i32> %desc.5, i32 0, i32 0, <16 x i32> %load)
  ret void
}
