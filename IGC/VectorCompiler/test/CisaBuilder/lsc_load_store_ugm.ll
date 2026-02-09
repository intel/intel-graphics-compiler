;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
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

declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)
declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <32 x i32>)
declare <48 x i32> @llvm.vc.internal.lsc.load.ugm.v48i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <48 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <64 x i32>)
declare <128 x i32> @llvm.vc.internal.lsc.load.ugm.v128i32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <128 x i32>)
declare <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i64>)
declare <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x float>)
declare <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x double>)

declare <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i32>, i16, i32, <16 x double>)

declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v48i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <48 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v64i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <64 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v128i32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <128 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x i64>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x float>)
declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i64>, i16, i32, <16 x double>)

declare void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16f64(<16 x i1>, i8, i8, i8, <2 x i8>, i64, <16 x i32>, i16, i32, <16 x double>)

declare <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <1 x i32>)
declare <2 x i32> @llvm.vc.internal.lsc.load.ugm.v2i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <2 x i32>)
declare <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <3 x i32>)
declare <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <4 x i32>)
declare <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i32>)
declare <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i32>)
declare <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <32 x i32>)
declare <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <64 x i32>)

declare <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v3i8.i64(<1 x i1>, i8, i8, i8, <3 x i8>, i64, i64, i16, i32, <64 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v1i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <1 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <2 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v3i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <3 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <4 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <16 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <32 x i32>)
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <64 x i32>)

declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v3i8.i64.v64i32(<1 x i1>, i8, i8, i8, <3 x i8>, i64, i64, i16, i32, <64 x i32>)

; CHECK-DAG: .decl [[BASE:V[0-9]+]] v_type=G type=q num_elts=1 align=qword
; CHECK-DAG: .decl [[BLOCK:V[0-9]+]] v_type=G type=q num_elts=1 align=GRF
; CHECK-DAG: .decl [[UBLOCK:V[0-9]+]] v_type=G type=uq num_elts=1 alias=<[[BLOCK]], 0>
; CHECK-DAG: .decl [[PRED:P[0-9]+]] v_type=P num_elts=16

; CHECK-DAG: .input [[BASE]] offset=64 size=8
; CHECK-DAG: .input [[SINDEX:V[0-9]+]] offset=128 size=128

define spir_kernel void @test(i64 %base, i64 %block, <16 x i64> %indices, i16 %pred) local_unnamed_addr #0 {
  %mask = bitcast i16 %pred to <16 x i1>

  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32 flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x2t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x3t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x4t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x8t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x16t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x32t flat[[[UBLOCK]]]:a64
  ; CHECK: lsc_load.ugm (M1, 1)  V{{[0-9]+}}:d32x64t flat[[[UBLOCK]]]:a64
  %b1 = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <1 x i32> undef)
  %b2 = call <2 x i32> @llvm.vc.internal.lsc.load.ugm.v2i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <2 x i32> undef)
  %b3 = call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <3 x i32> undef)
  %b4 = call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <4 x i32> undef)
  %b8 = call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <8 x i32> undef)
  %b16 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <16 x i32> undef)
  %b32 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <32 x i32> undef)
  %b64 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <64 x i32> undef)

  ; CHECK: lsc_load.ugm.st.uc.ca (M1, 1)  V{{[0-9]+}}:d32x64t flat[[[UBLOCK]]]:a64
  %b64l2 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <3 x i8> <i8 5, i8 1, i8 2>, i64 0, i64 %block, i16 1, i32 0, <64 x i32> undef)

  ; CHECK: lsc_load.ugm.ri.ri.ri (M1, 1)  V{{[0-9]+}}:d32x64t flat[[[UBLOCK]]]:a64
  %b64ri = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v3i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <3 x i8> <i8 6, i8 1, i8 2>, i64 0, i64 %block, i16 1, i32 0, <64 x i32> undef)

  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x2t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x3t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x4t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x8t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x16t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x32t
  ; CHECK: lsc_store.ugm (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x64t
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v1i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <1 x i32> %b1)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <2 x i32> %b2)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v3i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <3 x i32> %b3)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <4 x i32> %b4)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <8 x i32> %b8)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <16 x i32> %b16)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <32 x i32> %b32)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 %block, i16 1, i32 0, <64 x i32> %b64)

  ; CHECK: lsc_store.ugm.wt.uc.wb (M1, 1)  flat[[[UBLOCK]]]:a64 V{{[0-9]+}}:d32x64t
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v3i8.i64.v64i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <3 x i8> <i8 4, i8 1, i8 3>, i64 0, i64 %block, i16 1, i32 0, <64 x i32> %b64l2)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d8u32 flat[[[INDEX:V[0-9]+]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d8u32
  %c0 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %c0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d8u32 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d8u32
  %c1 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %c1)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d8u32 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d8u32
  %c2 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %c2)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d8u32 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d8u32
  %c3 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 5, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %c3)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d16u32 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d16u32
  %s0 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %s0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d16u32 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d16u32
  %s1 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %s1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d16u32 flat[0x2*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x2*[[INDEX]]]:a64 V{{[0-9]+}}:d16u32
  %s2 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 2, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 2, i32 0, <16 x i32> %s2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d16u32 flat[0x2*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x2*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d16u32
  %s3 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 2, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 2, i32 256, <16 x i32> %s3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d16u32 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d16u32
  %s4 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %s4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d16u32 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d16u32
  %s5 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %s5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d16u32 flat([[BASE]])[0x2*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x2*[[INDEX]]]:a64 V{{[0-9]+}}:d16u32
  %s6 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 2, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 2, i32 0, <16 x i32> %s6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d16u32 flat([[BASE]])[0x2*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x2*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d16u32
  %s7 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 2, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 6, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 2, i32 256, <16 x i32> %s7)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %i0 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %i0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %i1 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %i1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat[0x4*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x4*[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %i2 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 4, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 4, i32 0, <16 x i32> %i2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32 flat[0x4*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x4*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %i3 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 4, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 4, i32 256, <16 x i32> %i3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %i4 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i32> %i4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %i5 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i32> %i5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[0x4*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x4*[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %i6 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 4, i32 0, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 4, i32 0, <16 x i32> %i6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[0x4*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x4*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %i7 = call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 4, i32 256, <16 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 4, i32 256, <16 x i32> %i7)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32x2 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d32x2
  %vi0 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <32 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> %mask, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <32 x i32> %vi0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32x3 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32x3
  %vi1 = call <48 x i32> @llvm.vc.internal.lsc.load.ugm.v48i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <48 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v48i32(<16 x i1> %mask, i8 3, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <48 x i32> %vi1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32x4 flat[0x10*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x10*[[INDEX]]]:a64 V{{[0-9]+}}:d32x4
  %vi2 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 4, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 16, i32 0, <64 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v64i32(<16 x i1> %mask, i8 3, i8 3, i8 4, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 16, i32 0, <64 x i32> %vi2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32x8 flat[0x20*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x20*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32x8
  %vi3 = call <128 x i32> @llvm.vc.internal.lsc.load.ugm.v128i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 5, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 32, i32 256, <128 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v128i32(<16 x i1> %mask, i8 3, i8 3, i8 5, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 32, i32 256, <128 x i32> %vi3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32x2 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d32x2
  %vi4 = call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <32 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v32i32(<16 x i1> %mask, i8 3, i8 3, i8 2, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <32 x i32> %vi4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32x3 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32x3
  %vi5 = call <48 x i32> @llvm.vc.internal.lsc.load.ugm.v48i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <48 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v48i32(<16 x i1> %mask, i8 3, i8 3, i8 3, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <48 x i32> %vi5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32x4 flat([[BASE]])[0x10*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x10*[[INDEX]]]:a64 V{{[0-9]+}}:d32x4
  %vi6 = call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 4, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 16, i32 0, <64 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v64i32(<16 x i1> %mask, i8 3, i8 3, i8 4, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 16, i32 0, <64 x i32> %vi6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32x8 flat([[BASE]])[0x20*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x20*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32x8
  %vi7 = call <128 x i32> @llvm.vc.internal.lsc.load.ugm.v128i32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 5, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 32, i32 256, <128 x i32> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v128i32(<16 x i1> %mask, i8 3, i8 3, i8 5, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 32, i32 256, <128 x i32> %vi7)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %f0 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x float> %f0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %f1 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x float> %f1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat[0x4*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x4*[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %f2 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 4, i32 0, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 4, i32 0, <16 x float> %f2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32 flat[0x4*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x4*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %f3 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 4, i32 256, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 4, i32 256, <16 x float> %f3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %f4 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x float> %f4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %f5 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x float> %f5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[0x4*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x4*[[INDEX]]]:a64 V{{[0-9]+}}:d32
  %f6 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 4, i32 0, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 4, i32 0, <16 x float> %f6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d32 flat([[BASE]])[0x4*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x4*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d32
  %f7 = call <16 x float> @llvm.vc.internal.lsc.load.ugm.v16f32.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 4, i32 256, <16 x float> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f32(<16 x i1> %mask, i8 3, i8 3, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 4, i32 256, <16 x float> %f7)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d64 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %l0 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x i64> %l0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %l1 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x i64> %l1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat[0x8*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x8*[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %l2 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 8, i32 0, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 8, i32 0, <16 x i64> %l2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d64 flat[0x8*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x8*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %l3 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 8, i32 256, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 8, i32 256, <16 x i64> %l3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %l4 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x i64> %l4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %l5 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x i64> %l5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[0x8*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x8*[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %l6 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 8, i32 0, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 8, i32 0, <16 x i64> %l6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[0x8*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x8*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %l7 = call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 8, i32 256, <16 x i64> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 8, i32 256, <16 x i64> %l7)

; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d64 flat[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat[[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %d0 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, <16 x i64> %indices, i16 1, i32 0, <16 x double> %d0)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %d1 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, <16 x i64> %indices, i16 1, i32 256, <16 x double> %d1)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat[0x8*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat[0x8*[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %d2 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 8, i32 0, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 0, <16 x i64> %indices, i16 8, i32 0, <16 x double> %d2)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d64 flat[0x8*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat[0x8*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %d3 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 2, i8 2>, i64 0, <16 x i64> %indices, i16 8, i32 256, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 3, i8 3>, i64 0, <16 x i64> %indices, i16 8, i32 256, <16 x double> %d3)
; CHECK: ([[PRED]]) lsc_load.ugm (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm (M1, 16) flat([[BASE]])[[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %d4 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 %base, <16 x i64> %indices, i16 1, i32 0, <16 x double> %d4)
; CHECK: ([[PRED]]) lsc_load.ugm.uc.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.uc.uc.uc (M1, 16) flat([[BASE]])[[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %d5 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 %base, <16 x i64> %indices, i16 1, i32 256, <16 x double> %d5)
; CHECK: ([[PRED]]) lsc_load.ugm.st.uc.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[0x8*[[INDEX]]]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.st.uc.uc (M1, 16) flat([[BASE]])[0x8*[[INDEX]]]:a64 V{{[0-9]+}}:d64
  %d6 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 8, i32 0, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 5, i8 1>, i64 %base, <16 x i64> %indices, i16 8, i32 0, <16 x double> %d6)
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[0x8*[[INDEX]]+0x100]:a64
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x8*[[INDEX]]+0x100]:a64 V{{[0-9]+}}:d64
  %d7 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i64> %indices, i16 8, i32 256, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16f64(<16 x i1> %mask, i8 3, i8 4, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i64> %indices, i16 8, i32 256, <16 x double> %d7)

; CHECK: mov (M1, 16) [[IND32:V[0-9]+]](0,0)<1>
; CHECK: ([[PRED]]) lsc_load.ugm.ca.ca.uc (M1, 16) V{{[0-9]+}}:d64 flat([[BASE]])[0x8*[[UIND32:V[0-9]+]]+0x100]:a32u
; CHECK: ([[PRED]]) lsc_store.ugm.wb.wb.uc (M1, 16) flat([[BASE]])[0x8*[[IND32:V[0-9]+]]+0x100]:a32s V{{[0-9]+}}:d64
  %ind32 = trunc <16 x i64> %indices to <16 x i32>
  %d8 = call <16 x double> @llvm.vc.internal.lsc.load.ugm.v16f64.v16i1.v2i8.v16i32(<16 x i1> %mask, i8 4, i8 4, i8 1, <2 x i8> <i8 2, i8 2>, i64 %base, <16 x i32> %ind32, i16 8, i32 256, <16 x double> undef)
  call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i32.v16f64(<16 x i1> %mask, i8 5, i8 4, i8 1, <2 x i8> <i8 3, i8 3>, i64 %base, <16 x i32> %ind32, i16 8, i32 256, <16 x double> %d8)

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
!4 = !{void (i64, i64, <16 x i64>, i16)* @test, !"test", !5, i32 0, !6, !1, !7, i32 0}
!5 = !{i32 0, i32 0, i32 0, i32 0}
!6 = !{i32 64, i32 72, i32 128, i32 256}
!7 = !{!"image2d_t"}
!8 = !{void (i64, i64, <16 x i64>, i16)* @test, null, null, null, null}
