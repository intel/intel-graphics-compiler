;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check that lsc intrinsics are converted to bindless versions.

; RUN: %opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -march=genx64 -mcpu=Xe3PLPG -mattr=+efficient_64b_enabled -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.vc.internal.lsc.prefetch.bti.v1i1.v3i8.i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32)
declare <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1>, i8, i8, i8, <3 x i8>, i32, i32, i16, i32, <8 x i32>)

; CHECK: @test_ugm
; CHECK: call void @llvm.vc.internal.lsc.prefetch.surf.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 5, <3 x i8> zeroinitializer, i64 %arg1, i8 0, i32 32, i16 1, i32 0)
; CHECK: [[LOAD:%[^ ]*]] = call <8 x i32> @llvm.vc.internal.lsc.load.surf.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 5, <3 x i8> zeroinitializer, i64 %arg1, i8 0, i32 32, i16 1, i32 0, <8 x i32> undef)
; CHECK: call void @llvm.vc.internal.lsc.store.surf.v1i1.v3i8.i32.v8i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 5, <3 x i8> zeroinitializer, i64 %arg2, i8 0, i32 16, i16 1, i32 0, <8 x i32> [[LOAD]])
define spir_kernel void @test_ugm(i64 %arg1, i64 %arg2) {
  %1 = bitcast i64 %arg1 to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = bitcast i64 %arg2 to <2 x i32>
  %4 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %3, i32 2, i32 1, i32 2, i16 0, i32 undef)
  call void @llvm.vc.internal.lsc.prefetch.bti.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %2, i32 32, i16 1, i32 0)
  %5 = call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %2, i32 32, i16 1, i32 0, <8 x i32> undef)
  call void @llvm.vc.internal.lsc.store.bti.v1i1.v3i8.i32.v8i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %4, i32 16, i16 1, i32 0, <8 x i32> %5)
  ret void
}

declare <32 x i16> @llvm.vc.internal.lsc.load.2d.tgm.bti.v32i16.v2i8(<2 x i8>, i32, i32, i32, i32, i32)
declare void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v32i16(<2 x i8>, i32, i32, i32, i32, i32, <32 x i16>)

; CHECK: @test_tgm_2d
; CHECK: [[LOAD:%[^ ]*]] = call <32 x i16> @llvm.vc.internal.lsc.load.2d.tgm.surf.v32i16.v2i8(<2 x i8> <i8 1, i8 1>, i64 %arg1, i8 0, i32 2, i32 8, i32 %x, i32 %y)
; CHECK: call void @llvm.vc.internal.lsc.store.2d.tgm.surf.v2i8.v32i16(<2 x i8> <i8 1, i8 1>, i64 %arg2, i8 0, i32 2, i32 8, i32 %x, i32 %y, <32 x i16> [[LOAD]])

define spir_kernel void @test_tgm_2d(i64 %arg1, i64 %arg2, i32 %x, i32 %y) {
  %1 = bitcast i64 %arg1 to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = bitcast i64 %arg2 to <2 x i32>
  %4 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %3, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %5 = call <32 x i16> @llvm.vc.internal.lsc.load.2d.tgm.bti.v32i16.v2i8(<2 x i8> <i8 1, i8 1>, i32 %2, i32 2, i32 8, i32 %x, i32 %y)
  call void @llvm.vc.internal.lsc.store.2d.tgm.bti.v2i8.v32i16(<2 x i8> <i8 1, i8 1>, i32 %4, i32 2, i32 8, i32 %x, i32 %y, <32 x i16> %5)
  ret void
}

declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.v4i1.v2i8.v4i32(<4 x i1>, <2 x i8>, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v4i1.v2i8.v4i32(<4 x i1>, <2 x i8>, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.v4i1.v2i8.v4i32.v16i32(<4 x i1>, <2 x i8>, i8, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <16 x i32>)

; CHECK: @test_tgm_quad
; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 1, i64 %arg1, i8 0, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod)
; CHECK: [[LOAD:%[^ ]*]] = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i64 %arg1, i8 0, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru)
; CHECK: call void @llvm.vc.internal.lsc.store.quad.tgm.surf.v4i1.v2i8.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i64 %arg2, i8 0, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> [[LOAD]])

define void @test_tgm_quad(i64 %arg1, i64 %arg2, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru) {
  %1 = bitcast i64 %arg1 to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = bitcast i64 %arg2 to <2 x i32>
  %4 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %3, i32 2, i32 1, i32 2, i16 0, i32 undef)
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 1, i32 %2, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod)
  %5 = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.v16i32.v4i1.v2i8.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i32 %2, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %passthru)
  call void @llvm.vc.internal.lsc.store.quad.tgm.v4i1.v2i8.v4i32.v16i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, <2 x i8> zeroinitializer, i8 15, i32 %4, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %lod, <16 x i32> %5)
  ret void
}

declare <1 x float> @llvm.vc.internal.lsc.atomic.bti.v1f32.v1i1.v2i8.v1i32(<1 x i1>, i8, i8, i8, <2 x i8>, i32, <1 x i32>, i16, i32, <1 x float>, <1 x float>, <1 x float>)

; CHECK: @test_atomic_ugm
; CHECK: call <1 x float> @llvm.vc.internal.lsc.atomic.surf.v1f32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 20, i8 4, i8 3, <2 x i8> zeroinitializer, i64 %arg1, i8 0, <1 x i32> %arg2, i16 1, i32 0, <1 x float> %arg3, <1 x float> undef, <1 x float> undef)
define <1 x float> @test_atomic_ugm(i64 %arg1, <1 x i32> %arg2, <1 x float> %arg3) {
  %1 = bitcast i64 %arg1 to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = tail call <1 x float> @llvm.vc.internal.lsc.atomic.bti.v1f32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 20, i8 2, i8 3, <2 x i8> zeroinitializer, i32 %2, <1 x i32> %arg2, i16 1, i32 0, <1 x float> %arg3, <1 x float> undef, <1 x float> undef)
  ret <1 x float> %3
}

declare void @llvm.vc.internal.lsc.prefetch.quad.bti.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32)
declare <64 x i32> @llvm.vc.internal.lsc.load.quad.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i32, <16 x i32>, i16, i32, <64 x i32>)

; CHECK: @test_cmask_ugm
; CHECK: call void @llvm.vc.internal.lsc.prefetch.quad.surf.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %bti, i8 0, <16 x i32> %addrs, i16 1, i32 0)
; CHECK: [[LOAD:%[^ ]*]] = call <64 x i32> @llvm.vc.internal.lsc.load.quad.surf.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %bti, i8 0, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %passthru)
; CHECK: call void @llvm.vc.internal.lsc.store.quad.surf.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 4, i8 3, i8 15, <2 x i8> zeroinitializer, i64 %bti, i8 0, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> [[LOAD]])
define void @test_cmask_ugm(<16 x i1> %pred, i64 %bti, <16 x i32> %addrs, <64 x i32> %passthru) {
  %1 = bitcast i64 %bti to <2 x i32>
  %2 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  call void @llvm.vc.internal.lsc.prefetch.quad.bti.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %2, <16 x i32> %addrs, i16 1, i32 0)
  %3 = call <64 x i32> @llvm.vc.internal.lsc.load.quad.bti.v64i32.v16i1.v2i8.v16i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %2, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %passthru)
  call void @llvm.vc.internal.lsc.store.quad.bti.v16i1.v2i8.v16i32.v64i32(<16 x i1> %pred, i8 2, i8 3, i8 15, <2 x i8> zeroinitializer, i32 %2, <16 x i32> %addrs, i16 1, i32 0, <64 x i32> %3)
  ret void
}

declare i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16>, i32, i32, i32, i16, i32)
declare i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64>, i32, i32, i32, i16, i32)

; CHECK: @test_ugm_bti_array
define <8 x i32> @test_ugm_bti_array(<8 x i64> %bti, i32 %idx) {
  %1 = bitcast i32 %idx to <2 x i16>
  %2 = call i16 @llvm.genx.rdregioni.i16.v2i16.i16(<2 x i16> %1, i32 2, i32 1, i32 2, i16 0, i32 undef)
  %3 = shl i16 %2, 2
  %4 = mul i16 %3, 2
; CHECK: [[RDREG:%[^ ]*]] = call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %bti, i32 0, i32 1, i32 1, i16 {{[^,]+}}, i32 0)
  %5 = call i64 @llvm.genx.rdregioni.i64.v8i64.i16(<8 x i64> %bti, i32 0, i32 1, i32 1, i16 %4, i32 0)
  %6 = bitcast i64 %5 to <2 x i32>
  %7 = call i32 @llvm.genx.rdregioni.i32.v2i32.i16(<2 x i32> %6, i32 2, i32 1, i32 2, i16 0, i32 undef)
; CHECK: call <8 x i32> @llvm.vc.internal.lsc.load.surf.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 4, i8 3, i8 5, <3 x i8> zeroinitializer, i64 [[RDREG]], i8 0, i32 32, i16 1, i32 0, <8 x i32> undef)
  %8 = call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v3i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <3 x i8> zeroinitializer, i32 %7, i32 32, i16 1, i32 0, <8 x i32> undef)
  ret <8 x i32> %8
}
