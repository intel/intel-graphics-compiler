;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -vc-use-bindless-images -GenXPromoteStatefulToBindless -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=XeHPG -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPromoteStatefulToBindless
; ------------------------------------------------
; This test checks that GenXPromoteStatefulToBindless translates
; legacy media/typed/sampler intrinsics to bindless ones
;

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <12 x i32> @llvm.genx.media.ld.v12i32(i32, i32, i32, i32, i32, i32)
declare void @llvm.genx.media.st.v12i32(i32, i32, i32, i32, i32, i32, <12 x i32>)

; CHECK: @llvm.vc.predef.var.bss = external global i32
; CHECK: @llvm.vc.predef.var.bindless.sampler = external global i32

define spir_func void @media_ldst(i32 %bti1, i32 %bti2) {
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti1)
; CHECK: [[DST:%[^ ]*]] = call <12 x i32> @llvm.vc.internal.media.ld.predef.surface.v12i32.p0i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 0, i32 8, i32 0, i32 0)
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti2)
; CHECK: call void @llvm.vc.internal.media.st.predef.surface.p0i32.v12i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 0, i32 8, i32 0, i32 0, <12 x i32> [[DST]])
  %dst = tail call <12 x i32> @llvm.genx.media.ld.v12i32(i32 0, i32 %bti1, i32 0, i32 8, i32 0, i32 0)
  tail call void @llvm.genx.media.st.v12i32(i32 0, i32 %bti2, i32 0, i32 8, i32 0, i32 0, <12 x i32> %dst)
  ret void
}

declare <32 x float> @llvm.genx.gather4.typed.v32f32.v8i1.v8i32(i32, <8 x i1>, i32, <8 x i32>, <8 x i32>, <8 x i32>, <32 x float>)
declare void @llvm.genx.scatter4.typed.v8i1.v8i32.v32f32(i32, <8 x i1>, i32, <8 x i32>, <8 x i32>, <8 x i32>, <32 x float>)

define spir_func void @typed_ldst(i32 %bti1, i32 %bti2, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r) {
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti1)
; CHECK: [[DST:%[^ ]*]] = call <32 x float> @llvm.vc.internal.gather4.typed.predef.surface.v32f32.v8i1.p0i32.v8i32(i32 1, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <32 x float> undef)
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti2)
; CHECK: call void @llvm.vc.internal.scatter4.typed.predef.surface.v8i1.p0i32.v8i32.v32f32(i32 1, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <32 x float> [[DST]])
  %dst = tail call <32 x float> @llvm.genx.gather4.typed.v32f32.v8i1.v8i32(i32 1, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 %bti1, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <32 x float> undef)
  tail call void @llvm.genx.scatter4.typed.v8i1.v8i32.v32f32(i32 1, <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 %bti2, <8 x i32> %u, <8 x i32> %v, <8 x i32> %r, <32 x float> %dst)
  ret void
}

declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.add.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.sub.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.inc.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.dec.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.min.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.max.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.imin.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.imax.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.xchg.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.and.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.or.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.xor.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)
declare <4 x i32> @llvm.genx.typed.atomic.cmpxchg.v4i32.v4i1.v4i32(<4 x i1>, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)

define spir_func void @atomic(i32 %bti, <4 x i32> %src, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %passthru, i64 %addr) {
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST1:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.add.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> %src, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %passthru)
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST2:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.sub.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST1]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %src)
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST3:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.and.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST2]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST1]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST4:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.or.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST3]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST2]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST5:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.xor.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST4]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST3]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST6:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.xchg.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST5]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST4]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST7:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.min.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST6]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST5]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST8:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.max.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST7]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST6]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST9:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.imin.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST8]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST7]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST10:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.imax.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST9]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST8]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST11:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.inc.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 false, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST9]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST12:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.dec.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 false, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST10]])
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST13:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.typed.atomic.cmpxchg.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32* @llvm.vc.predef.var.bss, <4 x i32> [[DST10]], <4 x i32> [[DST11]], <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> [[DST12]])
; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> [[DST13]])

  %dst1 = tail call <4 x i32> @llvm.genx.typed.atomic.add.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %src, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %passthru)
  %dst2 = tail call <4 x i32> @llvm.genx.typed.atomic.sub.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst1, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %src)
  %dst3 = tail call <4 x i32> @llvm.genx.typed.atomic.and.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst2, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst1)
  %dst4 = tail call <4 x i32> @llvm.genx.typed.atomic.or.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst3, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst2)
  %dst5 = tail call <4 x i32> @llvm.genx.typed.atomic.xor.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst4, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst3)
  %dst6 = tail call <4 x i32> @llvm.genx.typed.atomic.xchg.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst5, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst4)
  %dst7 = tail call <4 x i32> @llvm.genx.typed.atomic.min.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst6, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst5)
  %dst8 = tail call <4 x i32> @llvm.genx.typed.atomic.max.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst7, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst6)
  %dst9 = tail call <4 x i32> @llvm.genx.typed.atomic.imin.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst8, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst7)
  %dst10 = tail call <4 x i32> @llvm.genx.typed.atomic.imax.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst9, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst8)
  %dst11 = tail call <4 x i32> @llvm.genx.typed.atomic.inc.v4i32.v4i1.v4i32(<4 x i1> <i1 false, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst9)
  %dst12 = tail call <4 x i32> @llvm.genx.typed.atomic.dec.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 false, i1 true, i1 true>, i32 %bti, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst10)
  %dst13 = tail call <4 x i32> @llvm.genx.typed.atomic.cmpxchg.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i32 %bti, <4 x i32> %dst10, <4 x i32> %dst11, <4 x i32> %u, <4 x i32> %v, <4 x i32> %r, <4 x i32> %dst12)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> %dst13)
  ret void
}

declare <4 x i32> @llvm.vc.internal.sampler.load.bti.v4i32.v4i1.v4i32(<4 x i1>, i16, i8, i16, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)

define spir_func void @sampler_load(i32 %bti, <4 x i32> %param1, <4 x i32> %param2, i64 %addr) {
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: [[DST:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.sampler.load.predef.surface.v4i32.v4i1.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 26, i8 8, i16 0, i32* @llvm.vc.predef.var.bss, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> [[DST]])

  %dst = call <4 x i32> @llvm.vc.internal.sampler.load.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 26, i8 8, i16 0, i32 %bti, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> %dst)
  ret void
}

declare <4 x i32> @llvm.vc.internal.sample.bti.v4i32.v4i1.v4i32(<4 x i1>, i16, i8, i16, i32, i32, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>, <4 x i32>)

define spir_func void @sample(i32 %bti, i32 %smpl, <4 x i32> %param1, <4 x i32> %param2, i64 %addr) {
; CHECK: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 %bti)
; CHECK: call void @llvm.vc.internal.write.predef.sampler.p0i32(i32* @llvm.vc.predef.var.bindless.sampler, i32 %smpl)
; CHECK: [[DST:%[^ ]*]] = call <4 x i32> @llvm.vc.internal.sample.predef.surface.v4i32.v4i1.p0i32.p0i32.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i32* @llvm.vc.predef.var.bss, i32* @llvm.vc.predef.var.bindless.sampler, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
; CHECK: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> [[DST]])

  %dst = tail call <4 x i32> @llvm.vc.internal.sample.bti.v4i32.v4i1.v4i32(<4 x i1> <i1 true, i1 true, i1 true, i1 true>, i16 0, i8 1, i16 0, i32 %bti, i32 %smpl, <4 x i32> undef, <4 x i32> %param1, <4 x i32> %param2, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer, <4 x i32> zeroinitializer)
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %addr, i16 1, i32 0, <4 x i32> %dst)
  ret void
}
