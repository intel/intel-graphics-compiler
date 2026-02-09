;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3PLPG -vc-skip-ocl-runtime-info -mattr=+efficient_64b_enabled -finalizer-opts='-dumpvisa -dumpcommonisa -isaasmToConsole' -o /dev/null | \
; RUN: FileCheck %s



target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <32 x i32>)
declare <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v48i32.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <48 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v64i32.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <64 x i32>)

declare void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)

declare void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v16i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v32i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v48i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <48 x i32>)
declare void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v64i32(<16 x i1>, <2 x i8>, i8, i64, i8, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, <64 x i32>)

; CHECK: .decl [[PRED:P[0-9]+]] v_type=P num_elts=16

define spir_kernel void @test_typed(i64 %surf, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, i16 %pred) local_unnamed_addr #0 {
  %mask = bitcast i16 %pred to <16 x i1>
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VX:V[0-9]+]]:d32.x  surf([[SURF:V[0-9]+]]){{.}}[[U:V[0-9]+]],[[V:V[0-9]+]],[[R:V[0-9]+]],[[LOD:V[0-9]+]]{{.}}:a32
  %x = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 1, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.y  surf([[SURF]]){{.}}[[U]],[[V]],[[R]]{{.}}:a32
  %y = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 2, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> undef, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.z  surf([[SURF]]){{.}}[[U]],[[V]]{{.}}:a32
  %z = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 4, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.w  surf([[SURF]]){{.}}[[U]]{{.}}:a32
  %w = call <16 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v16i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 8, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef, <16 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXY:V[0-9]+]]:d32.xy  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xy = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 3, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xz = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 5, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 9, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yz = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 6, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 10, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.zw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %zw = call <32 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v32i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 12, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXYZ:V[0-9]+]]:d32.xyz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyz = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v48i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 7, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xyw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v48i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 11, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.xzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xzw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v48i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 13, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)
; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  V{{[0-9]+}}:d32.yzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %yzw = call <48 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v48i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 14, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm (M1, 16)  [[VXYZW:V[0-9]+]]:d32.xyzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  %xyzw = call <64 x i32> @llvm.vc.internal.lsc.load.quad.tgm.surf.v64i32.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 15, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <64 x i32> undef)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.x  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 1, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.y  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 2, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.z  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 4, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.w  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 8, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xy  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 3, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 5, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 9, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.yz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 6, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.yw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 10, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.zw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 12, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xyz  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 7, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xyw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 11, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 13, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)
; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.yzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 14, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_load_quad.tgm.ca.ca.uc (M1, 16)  %null:d32.xyzw  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32
  call void @llvm.vc.internal.lsc.prefetch.quad.tgm.surf.v16i1.v2i8.v16i32(<16 x i1> %mask, <2 x i8> <i8 2, i8 2>, i8 15, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod)

; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VX]]:d32.x
  call void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v16i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 1, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <16 x i32> %x)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXY]]:d32.xy
  call void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v32i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 3, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <32 x i32> %xy)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXYZ]]:d32.xyz
  call void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v48i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 7, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <48 x i32> %xyz)
; CHECK: ([[PRED]]) lsc_store_quad.tgm (M1, 16)  surf([[SURF]]){{.}}[[U]],[[V]],[[R]],[[LOD]]{{.}}:a32  [[VXYZW]]:d32.xyzw
  call void @llvm.vc.internal.lsc.store.quad.tgm.surf.v16i1.v2i8.v16i32.v64i32(<16 x i1> %mask, <2 x i8> zeroinitializer, i8 15, i64 %surf, i8 0, <16 x i32> %u, <16 x i32> %v, <16 x i32> %r, <16 x i32> %lod, <64 x i32> %xyzw)

  ret void
}


attributes #0 = { noinline nounwind "CMGenxMain" }

!spirv.Source = !{!1}
!opencl.spir.version = !{!2}
!opencl.ocl.version = !{!1}
!opencl.used.extensions = !{!0}
!opencl.used.optional.core.features = !{!0}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{}
!1 = !{i32 0}
!2 = !{i32 1, i32 1}
!3 = !{i16 6, i16 14}
!4 = !{void (i64, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i16)* @test_typed, !"test_typed", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 8, i32 72, i32 136, i32 200, i32 264, i32 328}
!7 = !{!"image2d_t"}
!8 = !{void (i64, <16 x i32>, <16 x i32>, <16 x i32>, <16 x i32>, i16)* @test_typed, null, null, null, null}
