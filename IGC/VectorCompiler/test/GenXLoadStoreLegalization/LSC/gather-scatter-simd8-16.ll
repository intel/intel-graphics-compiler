;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLoadStoreLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

declare <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v3i1.v2i8.v3i64(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <3 x i32>)
declare <6 x i32> @llvm.vc.internal.lsc.load.ugm.v6i32.v3i1.v2i8.v3i64(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <6 x i32>)
declare <3 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v3i32.v3i1.v2i8.v3i64(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <3 x i32>)
declare <6 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v6i32.v3i1.v2i8.v3i64(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <6 x i32>)

declare <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <16 x i32>)
declare <8 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <16 x i32>)

declare <9 x i32> @llvm.vc.internal.lsc.load.ugm.v9i32.v9i1.v2i8.v9i64(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <9 x i32>)
declare <18 x i32> @llvm.vc.internal.lsc.load.ugm.v18i32.v9i1.v2i8.v9i64(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <18 x i32>)
declare <9 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v9i32.v9i1.v2i8.v9i64(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <9 x i32>)
declare <18 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v18i32.v9i1.v2i8.v9i64(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <18 x i32>)

declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v32i1.v2i8.v32i64(<32 x i1>, i8, i8, i8, <2 x i8>, i64, <32 x i64>, i16, i32, <32 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v32i1.v2i8.v32i64(<32 x i1>, i8, i8, i8, <2 x i8>, i64, <32 x i64>, i16, i32, <64 x i32>)

declare <24 x i32> @llvm.vc.internal.lsc.load.ugm.v24i32.v24i1.v2i8.v24i64(<24 x i1>, i8, i8, i8, <2 x i8>, i64, <24 x i64>, i16, i32, <24 x i32>)
declare <48 x i32> @llvm.vc.internal.lsc.load.ugm.v48i32.v24i1.v2i8.v24i64(<24 x i1>, i8, i8, i8, <2 x i8>, i64, <24 x i64>, i16, i32, <64 x i32>)

declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v3i32(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <3 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v6i32(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <6 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v3i1.v2i8.v3i64.v3i32(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <3 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v3i1.v2i8.v3i64.v6i32(<3 x i1>, i8, i8, i8, <2 x i8>, i64, <3 x i64>, i16, i32, <6 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1>, i8, i8, i8, <2 x i8>, i64, <8 x i64>, i16, i32, <16 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v9i1.v2i8.v9i64.v9i32(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <9 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v9i1.v2i8.v9i64.v18i32(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <18 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v9i1.v2i8.v9i64.v9i32(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <9 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.ugm.v9i1.v2i8.v9i64.v18i32(<9 x i1>, i8, i8, i8, <2 x i8>, i64, <9 x i64>, i16, i32, <18 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)

define void @test_simd8(<8 x i1> %pred, <8 x i64> %addr, <8 x i32> %data) {
  ; CHECK: %ld = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  %ld = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  ; CHECK: %ldq = call <8 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  %ldq = call <8 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  call void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v8i32(<8 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <8 x i32> %data)
  ret void
}

define void @test_simd8x2(<8 x i1> %pred, <8 x i64> %addr, <16 x i32> %data) {
  ; CHECK: %ld = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  %ld = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ; CHECK: %ldq = call <16 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  %ldq = call <16 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> %pred, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> %pred, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  call void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> %pred, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <8 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ret void
}

define void @test_simd16(<16 x i1> %pred, <16 x i64> %addr, <16 x i32> %data) {
  ; CHECK: %ld = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  %ld = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <16 x i32> %data)
  ret void
}

define void @test_simd3(<3 x i1> %pred, <3 x i64> %addr, <3 x i32> %data) {
  ; CHECK: [[EXTPREDLD:%[^ ]+]] = call <4 x i1> @llvm.genx.wrpredregion.v4i1.v3i1(<4 x i1> zeroinitializer, <3 x i1> %pred, i32 0)
  ; CHECK: [[EXTADDRLD:%[^ ]+]] = call <4 x i64> @llvm.genx.wrregioni.v4i64.v3i64.i16.i1(<4 x i64> undef, <3 x i64> %addr, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[EXTDATALD:%[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.v3i32.i16.i1(<4 x i32> undef, <3 x i32> %data, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[EXTLD:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v4i1.v2i8.v4i64(<4 x i1> [[EXTPREDLD]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <4 x i64> [[EXTADDRLD]], i16 1, i32 0, <4 x i32> [[EXTDATALD]])
  ; CHECK: %ld = call <3 x i32> @llvm.genx.rdregioni.v3i32.v4i32.i16(<4 x i32> [[EXTLD]], i32 4, i32 3, i32 1, i16 0, i32 undef)
  %ld = call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v3i1.v2i8.v3i64(<3 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <3 x i32> %data)

  ; CHECK: [[EXTPREDST:%[^ ]+]] = call <4 x i1> @llvm.genx.wrpredregion.v4i1.v3i1(<4 x i1> zeroinitializer, <3 x i1> %pred, i32 0)
  ; CHECK: [[EXTADDRST:%[^ ]+]] = call <4 x i64> @llvm.genx.wrregioni.v4i64.v3i64.i16.i1(<4 x i64> undef, <3 x i64> %addr, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[EXTDATAST:%[^ ]+]] = call <4 x i32> @llvm.genx.wrregioni.v4i32.v3i32.i16.i1(<4 x i32> undef, <3 x i32> %data, i32 4, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v4i1.v2i8.v4i64.v4i32(<4 x i1> [[EXTPREDST]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <4 x i64> [[EXTADDRST]], i16 1, i32 0, <4 x i32> [[EXTDATAST]])
  call void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v3i32(<3 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <3 x i32> %data)

  ; CHECK: [[LDQ:%[^ ]+]] = call <4 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v4i32.v4i1.v2i8.v4i64(<4 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <4 x i64> {{[^ ,)]+}}, i16 1, i32 0, <4 x i32> {{[^ ,)]+}})
  %ldq = call <3 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v3i32.v3i1.v2i8.v3i64(<3 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <3 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v4i1.v2i8.v4i64.v4i32(<4 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <4 x i64> {{[^ ,)]+}}, i16 1, i32 0, <4 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.quad.ugm.v3i1.v2i8.v3i64.v3i32(<3 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <3 x i32> %data)

  ret void
}

define void @test_simd3x2(<3 x i1> %pred, <3 x i64> %addr, <6 x i32> %data) {
  ; CHECK: [[EXTPREDLD:%[^ ]+]] = call <8 x i1> @llvm.genx.wrpredregion.v8i1.v3i1(<8 x i1> zeroinitializer, <3 x i1> %pred, i32 0)
  ; CHECK: [[EXTADDRLD:%[^ ]+]] = call <8 x i64> @llvm.genx.wrregioni.v8i64.v3i64.i16.i1(<8 x i64> undef, <3 x i64> %addr, i32 8, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[EXTDATALD:%[^ ]+]] = call <16 x i32> @llvm.genx.wrregioni.v16i32.v6i32.i16.i1(<16 x i32> undef, <6 x i32> %data, i32 8, i32 3, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[EXTLD:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> [[EXTPREDLD]], i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[EXTADDRLD]], i16 1, i32 0, <16 x i32> [[EXTDATALD]])
  ; CHECK: %ld = call <6 x i32> @llvm.genx.rdregioni.v6i32.v16i32.i16(<16 x i32> [[EXTLD]], i32 8, i32 3, i32 1, i16 0, i32 undef)
  %ld = call <6 x i32> @llvm.vc.internal.lsc.load.ugm.v6i32.v3i1.v2i8.v3i64(<3 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <6 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <8 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v6i32(<3 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <6 x i32> %data)

  ; CHECK: {{%[^ ]+}} = call <16 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v16i32.v8i1.v2i8.v8i64(<8 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <8 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  %ldq = call <6 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v6i32.v3i1.v2i8.v3i64(<3 x i1> %pred, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <6 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v8i1.v2i8.v8i64.v16i32(<8 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <8 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.quad.ugm.v3i1.v2i8.v3i64.v6i32(<3 x i1> %pred, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <3 x i64> %addr, i16 1, i32 0, <6 x i32> %data)

  ret void
}

define void @test_simd9(<9 x i1> %pred, <9 x i64> %addr, <9 x i32> %data) {
  ; CHECK: {{%[^ ]+}} = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  %ld = call <9 x i32> @llvm.vc.internal.lsc.load.ugm.v9i32.v9i1.v2i8.v9i64(<9 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <9 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.ugm.v9i1.v2i8.v9i64.v9i32(<9 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <9 x i32> %data)

  ; CHECK: {{%[^ ]+}} = call <16 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  %ldq = call <9 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v9i32.v9i1.v2i8.v9i64(<9 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <9 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <16 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.quad.ugm.v9i1.v2i8.v9i64.v9i32(<9 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <9 x i32> %data)

  ret void
}

define void @test_simd9x2(<9 x i1> %pred, <9 x i64> %addr, <18 x i32> %data) {
  ; CHECK: {{%[^ ]+}} = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <32 x i32> {{[^ ,)]+}})
  %ld = call <18 x i32> @llvm.vc.internal.lsc.load.ugm.v18i32.v9i1.v2i8.v9i64(<9 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <18 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <32 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.ugm.v9i1.v2i8.v9i64.v18i32(<9 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <18 x i32> %data)

  ; CHECK: {{%[^ ]+}} = call <32 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <32 x i32> {{[^ ,)]+}})
  %ldq = call <18 x i32> @llvm.vc.internal.lsc.load.quad.ugm.v18i32.v9i1.v2i8.v9i64(<9 x i1> %pred, i8 3, i8 3, i8 9, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <18 x i32> %data)

  ; CHECK: call void @llvm.vc.internal.lsc.store.quad.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> {{[^ ,)]+}}, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <16 x i64> {{[^ ,)]+}}, i16 1, i32 0, <32 x i32> {{[^ ,)]+}})
  call void @llvm.vc.internal.lsc.store.quad.ugm.v9i1.v2i8.v9i64.v18i32(<9 x i1> %pred, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, <9 x i64> %addr, i16 1, i32 0, <18 x i32> %data)

  ret void
}

define void @test_simd32(<32 x i1> %pred, <32 x i64> %addr, <32 x i32> %data) {
  ; CHECK: [[LDPRED0:%[^ ]+]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
  ; CHECK: [[LDADDR0:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 32, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDDATA0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %data, i32 32, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDRES0:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> [[LDPRED0]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[LDADDR0]], i16 1, i32 0, <16 x i32> [[LDDATA0]])
  ; CHECK: [[LDINS0:%[^ ]+]] = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> undef, <16 x i32> [[LDRES0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[LDPRED16:%[^ ]+]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
  ; CHECK: [[LDADDR16:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 32, i32 16, i32 1, i16 128, i32 undef)
  ; CHECK: [[LDDATA16:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> %data, i32 32, i32 16, i32 1, i16 64, i32 undef)
  ; CHECK: [[LDRES16:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> [[LDPRED16]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[LDADDR16]], i16 1, i32 0, <16 x i32> [[LDDATA16]])
  ; CHECK: %ld = call <32 x i32> @llvm.genx.wrregioni.v32i32.v16i32.i16.i1(<32 x i32> [[LDINS0]], <16 x i32> [[LDRES16]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ld = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v32i1.v2i8.v32i64(<32 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <32 x i32> %data)
  ret void
}

define void @test_simd32x2(<32 x i1> %pred, <32 x i64> %addr, <64 x i32> %data) {
  ; CHECK: [[LDPRED0:%[^ ]+]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 0)
  ; CHECK: [[LDADDR0:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 32, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDDATA0:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %data, i32 32, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDRES0:%[^ ]+]] = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> [[LDPRED0]], i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[LDADDR0]], i16 1, i32 0, <32 x i32> [[LDDATA0]])
  ; CHECK: [[LDINS0:%[^ ]+]] = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32> undef, <32 x i32> [[LDRES0]], i32 32, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[LDPRED16:%[^ ]+]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v32i1(<32 x i1> %pred, i32 16)
  ; CHECK: [[LDADDR16:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v32i64.i16(<32 x i64> %addr, i32 32, i32 16, i32 1, i16 128, i32 undef)
  ; CHECK: [[LDDATA16:%[^ ]+]] = call <32 x i32> @llvm.genx.rdregioni.v32i32.v64i32.i16(<64 x i32> %data, i32 32, i32 16, i32 1, i16 64, i32 undef)
  ; CHECK: [[LDRES16:%[^ ]+]] = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> [[LDPRED16]], i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[LDADDR16]], i16 1, i32 0, <32 x i32> [[LDDATA16]])
  ; CHECK: %ld = call <64 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<64 x i32> [[LDINS0]], <32 x i32> [[LDRES16]], i32 32, i32 16, i32 1, i16 64, i32 undef, i1 true)
  %ld = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v32i1.v2i8.v32i64(<32 x i1> %pred, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <32 x i64> %addr, i16 1, i32 0, <64 x i32> %data)
  ret void
}

define void @test_simd24(<24 x i1> %pred, <24 x i64> %addr, <24 x i32> %data) {
  ; CHECK: [[LDPRED0:%[^ ]+]] = call <16 x i1> @llvm.genx.rdpredregion.v16i1.v24i1(<24 x i1> %pred, i32 0)
  ; CHECK: [[LDADDR0:%[^ ]+]] = call <16 x i64> @llvm.genx.rdregioni.v16i64.v24i64.i16(<24 x i64> %addr, i32 24, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDDATA0:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v24i32.i16(<24 x i32> %data, i32 24, i32 16, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDRES0:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> [[LDPRED0]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> [[LDADDR0]], i16 1, i32 0, <16 x i32> [[LDDATA0]])
  ; CHECK: [[LDINS0:%[^ ]+]] = call <24 x i32> @llvm.genx.wrregioni.v24i32.v16i32.i16.i1(<24 x i32> undef, <16 x i32> [[LDRES0]], i32 24, i32 16, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[LDPRED16:%[^ ]+]] = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v24i1(<24 x i1> %pred, i32 16)
  ; CHECK: [[LDADDR16:%[^ ]+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v24i64.i16(<24 x i64> %addr, i32 24, i32 8, i32 1, i16 128, i32 undef)
  ; CHECK: [[LDDATA16:%[^ ]+]] = call <8 x i32> @llvm.genx.rdregioni.v8i32.v24i32.i16(<24 x i32> %data, i32 24, i32 8, i32 1, i16 64, i32 undef)
  ; CHECK: [[LDRES16:%[^ ]+]] = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v8i1.v2i8.v8i64(<8 x i1> [[LDPRED16]], i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[LDADDR16]], i16 1, i32 0, <8 x i32> [[LDDATA16]])
  ; CHECK: %ld = call <24 x i32> @llvm.genx.wrregioni.v24i32.v8i32.i16.i1(<24 x i32> %5, <8 x i32> %9, i32 24, i32 8, i32 1, i16 64, i32 undef, i1 true)
  %ld = call <24 x i32> @llvm.vc.internal.lsc.load.ugm.v24i32.v24i1.v2i8.v24i64(<24 x i1> %pred, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <24 x i64> %addr, i16 1, i32 0, <24 x i32> %data)
  ret void
}

declare <128 x i32> @llvm.vc.internal.lsc.load.ugm.v128i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <128 x i32>)

define void @test_simd16_vs8_i32(<16 x i1> %pred, <16 x i64> %addr, <128 x i32> %data) {
  ; CHECK: [[LDPRED0:%[^ ]+]] = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v16i1(<16 x i1> %pred, i32 0)
  ; CHECK: [[LDADDR0:%[^ ]+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %addr, i32 16, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDDATA0:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %data, i32 16, i32 8, i32 1, i16 0, i32 undef)
  ; CHECK: [[LDRES0:%[^ ]+]] = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v8i1.v2i8.v8i64(<8 x i1> [[LDPRED0]], i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[LDADDR0]], i16 1, i32 0, <64 x i32> [[LDDATA0]])
  ; CHECK: [[LDINS0:%[^ ]+]] = call <128 x i32> @llvm.genx.wrregioni.v128i32.v64i32.i16.i1(<128 x i32> undef, <64 x i32> [[LDRES0]], i32 16, i32 8, i32 1, i16 0, i32 undef, i1 true)
  ; CHECK: [[LDPRED8:%[^ ]+]] = call <8 x i1> @llvm.genx.rdpredregion.v8i1.v16i1(<16 x i1> %pred, i32 8)
  ; CHECK: [[LDADDR8:%[^ ]+]] = call <8 x i64> @llvm.genx.rdregioni.v8i64.v16i64.i16(<16 x i64> %addr, i32 16, i32 8, i32 1, i16 64, i32 undef)
  ; CHECK: [[LDDATA8:%[^ ]+]] = call <64 x i32> @llvm.genx.rdregioni.v64i32.v128i32.i16(<128 x i32> %data, i32 16, i32 8, i32 1, i16 32, i32 undef)
  ; CHECK: [[LDRES8:%[^ ]+]] = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v8i1.v2i8.v8i64(<8 x i1> [[LDPRED8]], i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, <8 x i64> [[LDADDR8]], i16 1, i32 0, <64 x i32> [[LDDATA8]])
  ; CHECK: %ld = call <128 x i32> @llvm.genx.wrregioni.v128i32.v64i32.i16.i1(<128 x i32> [[LDINS0]], <64 x i32> [[LDRES8]], i32 16, i32 8, i32 1, i16 32, i32 undef, i1 true)
  %ld = call <128 x i32> @llvm.vc.internal.lsc.load.ugm.v128i32.v16i1.v2i8.v16i64(<16 x i1> %pred, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, <16 x i64> %addr, i16 1, i32 0, <128 x i32> %data)
  ret void
}
