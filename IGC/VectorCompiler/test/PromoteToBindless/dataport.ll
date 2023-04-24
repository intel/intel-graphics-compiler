;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Check old dataport buffer arguments are converted to SSO parameters.

; RUN: opt %use_old_pass_manager% -GenXPromoteStatefulToBindless -vc-use-bindless-buffers -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <8 x i32> @llvm.genx.dword.atomic2.add.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.sub.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.min.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.max.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.xchg.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.and.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.or.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.xor.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.imin.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.imax.v8i32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x i32>)

declare <8 x float> @llvm.genx.dword.atomic2.fmin.v8f32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x float>)
declare <8 x float> @llvm.genx.dword.atomic2.fmax.v8f32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x float>)
declare <8 x float> @llvm.genx.dword.atomic2.fadd.v8f32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x float>)
declare <8 x float> @llvm.genx.dword.atomic2.fsub.v8f32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x float>)

declare <8 x i32> @llvm.genx.dword.atomic2.inc.v8i32.v8i1(<8 x i1>, i32, <8 x i32>)
declare <8 x i32> @llvm.genx.dword.atomic2.dec.v8i32.v8i1(<8 x i1>, i32, <8 x i32>)

declare <8 x i32> @llvm.genx.dword.atomic2.cmpxchg.v8i32.v8i1(<8 x i1>, i32, <8 x i32>, <8 x i32>, <8 x i32>)
declare <8 x float> @llvm.genx.dword.atomic2.fcmpwr.v8f32.v8i1.v8i32(<8 x i1>, i32, <8 x i32>, <8 x float>, <8 x float>)

declare <8 x i32> @llvm.genx.gather.masked.scaled2.v8i32.v8i32.v8i1(i32, i16, i32, i32, <8 x i32>, <8 x i1>)
declare <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v8i32.v8i1(i32, i16, i32, i32, <8 x i32>, <8 x i1>)
declare void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1>, i32, i16, i32, i32, <8 x i32>, <8 x i32>)
declare void @llvm.genx.scatter4.scaled.v8i1.v8i32.v32i32(<8 x i1>, i32, i16, i32, i32, <8 x i32>, <32 x i32>)

declare <8 x i32> @llvm.genx.oword.ld.v8i32(i32, i32, i32)
declare <8 x i32> @llvm.genx.oword.ld.unaligned.v8i32(i32, i32, i32)
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)

declare void @sink.v8i32(<8 x i32>)
declare void @sink.v8f32(<8 x float>)
declare void @sink.v32i32(<32 x i32>)
declare <8 x i1> @src.v8i1()
declare <8 x i32> @src.v8i32()
declare <8 x float> @src.v8f32()
declare <32 x i32> @src.v32i32()
declare i32 @src.i32()

; COM: DWORD two-src atomics.
; COM: =======================
; CHECK-LABEL: @dword_atomic_add(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.add.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_add(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.add.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_sub(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.sub.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_sub(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.sub.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_min(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.min.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_min(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.min.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_max(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.max.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_max(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.max.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_xchg(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.xchg.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_xchg(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.xchg.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_and(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.and.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_and(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.and.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_or(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.or.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_or(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.or.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_xor(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.xor.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_xor(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.xor.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_imin(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.imin.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_imin(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.imin.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_imax(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.imax.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_imax(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.imax.v8i32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; COM: Floating point atomics.
; COM: =======================
; CHECK-LABEL: @dword_atomic_fmin(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x float> @llvm.genx.dword.atomic2.fmin.predef.surface.v8f32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x float> [[SRC]])
; CHECK-NEXT: call void @sink.v8f32(<8 x float> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_fmin(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x float> @src.v8f32()
  %res = call <8 x float> @llvm.genx.dword.atomic2.fmin.v8f32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x float> %src)
  call void @sink.v8f32(<8 x float> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_fmax(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x float> @llvm.genx.dword.atomic2.fmax.predef.surface.v8f32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x float> [[SRC]])
; CHECK-NEXT: call void @sink.v8f32(<8 x float> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_fmax(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x float> @src.v8f32()
  %res = call <8 x float> @llvm.genx.dword.atomic2.fmax.v8f32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x float> %src)
  call void @sink.v8f32(<8 x float> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_fadd(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x float> @llvm.genx.dword.atomic2.fadd.predef.surface.v8f32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x float> [[SRC]])
; CHECK-NEXT: call void @sink.v8f32(<8 x float> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_fadd(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x float> @src.v8f32()
  %res = call <8 x float> @llvm.genx.dword.atomic2.fadd.v8f32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x float> %src)
  call void @sink.v8f32(<8 x float> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_fsub(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x float> @llvm.genx.dword.atomic2.fsub.predef.surface.v8f32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x float> [[SRC]])
; CHECK-NEXT: call void @sink.v8f32(<8 x float> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_fsub(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x float> @src.v8f32()
  %res = call <8 x float> @llvm.genx.dword.atomic2.fsub.v8f32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x float> %src)
  call void @sink.v8f32(<8 x float> %res)
  ret void
}

; COM: DWORD unary atomics.
; COM: ====================
; CHECK-LABEL: @dword_atomic_inc(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.inc.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_inc(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.inc.v8i32.v8i1(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_dec(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.dec.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_dec(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.dec.v8i32.v8i1(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; COM: DWORD ternary atomics.
; COM: ====================
; CHECK-LABEL: @dword_atomic_cmpxchg(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC1:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC2:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.dword.atomic2.cmpxchg.predef.surface.v8i32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x i32> [[SRC1]], <8 x i32> [[SRC2]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_cmpxchg(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src1 = call <8 x i32> @src.v8i32()
  %src2 = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.dword.atomic2.cmpxchg.v8i32.v8i1(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x i32> %src1, <8 x i32> %src2)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @dword_atomic_fcmpwr(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC1:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: [[SRC2:%[^ ]*]] = call <8 x float> @src.v8f32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x float> @llvm.genx.dword.atomic2.fcmpwr.predef.surface.v8f32.v8i1.p0i32.v8i32(<8 x i1> [[PRED]], i32* @llvm.vc.predef.var.bss, <8 x i32> [[ADDRS]], <8 x float> [[SRC1]], <8 x float> [[SRC2]])
; CHECK-NEXT: call void @sink.v8f32(<8 x float> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @dword_atomic_fcmpwr(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src1 = call <8 x float> @src.v8f32()
  %src2 = call <8 x float> @src.v8f32()
  %res = call <8 x float> @llvm.genx.dword.atomic2.fcmpwr.v8f32.v8i1.v8i32(<8 x i1> %pred, i32 %buf, <8 x i32> %addrs, <8 x float> %src1, <8 x float> %src2)
  call void @sink.v8f32(<8 x float> %res)
  ret void
}

; COM: Gather/scatter instructions.
; COM: ============================
; CHECK-LABEL: @gather_scaled(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.gather.masked.scaled2.predef.surface.v8i32.p0i32.v8i32.v8i1(i32 0, i16 0, i32* @llvm.vc.predef.var.bss, i32 0, <8 x i32> [[ADDRS]], <8 x i1> [[PRED]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @gather_scaled(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %res = call <8 x i32> @llvm.genx.gather.masked.scaled2.v8i32.v8i32.v8i1(i32 0, i16 0, i32 %buf, i32 0, <8 x i32> %addrs, <8 x i1> %pred)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @gather4_scaled(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <32 x i32> @llvm.genx.gather4.masked.scaled2.predef.surface.v32i32.p0i32.v8i32.v8i1(i32 15, i16 0, i32* @llvm.vc.predef.var.bss, i32 0, <8 x i32> [[ADDRS]], <8 x i1> [[PRED]])
; CHECK-NEXT: call void @sink.v32i32(<32 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @gather4_scaled(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %res = call <32 x i32> @llvm.genx.gather4.masked.scaled2.v32i32.v8i32.v8i1(i32 15, i16 0, i32 %buf, i32 0, <8 x i32> %addrs, <8 x i1> %pred)
  call void @sink.v32i32(<32 x i32> %res)
  ret void
}

; CHECK-LABEL: @scatter_scaled(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: call void @llvm.genx.scatter.scaled.predef.surface.v8i1.p0i32.v8i32.v8i32(<8 x i1> [[PRED]], i32 0, i16 0, i32* @llvm.vc.predef.var.bss, i32 0, <8 x i32> [[ADDRS]], <8 x i32> [[SRC]])
; CHECK-NEXT: ret void
define spir_func void @scatter_scaled(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <8 x i32> @src.v8i32()
  call void @llvm.genx.scatter.scaled.v8i1.v8i32.v8i32(<8 x i1> %pred, i32 0, i16 0, i32 %buf, i32 0, <8 x i32> %addrs, <8 x i32> %src)
  ret void
}

; CHECK-LABEL: @scatter4_scaled(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[PRED:%[^ ]*]] = call <8 x i1> @src.v8i1()
; CHECK-NEXT: [[ADDRS:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <32 x i32> @src.v32i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: call void @llvm.genx.scatter4.scaled.predef.surface.v8i1.p0i32.v8i32.v32i32(<8 x i1> [[PRED]], i32 15, i16 0, i32* @llvm.vc.predef.var.bss, i32 0, <8 x i32> [[ADDRS]], <32 x i32> [[SRC]])
; CHECK-NEXT: ret void
define spir_func void @scatter4_scaled(i32 %buf) {
  %pred = call <8 x i1> @src.v8i1()
  %addrs = call <8 x i32> @src.v8i32()
  %src = call <32 x i32> @src.v32i32()
  call void @llvm.genx.scatter4.scaled.v8i1.v8i32.v32i32(<8 x i1> %pred, i32 15, i16 0, i32 %buf, i32 0, <8 x i32> %addrs, <32 x i32> %src)
  ret void
}

; COM: Oword instructions.
; COM: ===================
; CHECK-LABEL: @oword_ld(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[ADDR:%[^ ]*]] = call i32 @src.i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.oword.ld.predef.surface.v8i32.p0i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 [[ADDR]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @oword_ld(i32 %buf) {
  %addr = call i32 @src.i32()
  %res = call <8 x i32> @llvm.genx.oword.ld.v8i32(i32 0, i32 %buf, i32 %addr)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @oword_ld_unaligned(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[ADDR:%[^ ]*]] = call i32 @src.i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: [[RES:%[^ ]*]] = call <8 x i32> @llvm.genx.oword.ld.unaligned.predef.surface.v8i32.p0i32(i32 0, i32* @llvm.vc.predef.var.bss, i32 [[ADDR]])
; CHECK-NEXT: call void @sink.v8i32(<8 x i32> [[RES]])
; CHECK-NEXT: ret void
define spir_func void @oword_ld_unaligned(i32 %buf) {
  %addr = call i32 @src.i32()
  %res = call <8 x i32> @llvm.genx.oword.ld.unaligned.v8i32(i32 0, i32 %buf, i32 %addr)
  call void @sink.v8i32(<8 x i32> %res)
  ret void
}

; CHECK-LABEL: @oword_st(
; CHECK: i32 [[SSO:%[^) ]*]]
; CHECK-NEXT: [[ADDR:%[^ ]*]] = call i32 @src.i32()
; CHECK-NEXT: [[SRC:%[^ ]*]] = call <8 x i32> @src.v8i32()
; CHECK-NEXT: call void @llvm.genx.write.predef.surface.p0i32(i32* @llvm.vc.predef.var.bss, i32 [[SSO]])
; CHECK-NEXT: call void @llvm.genx.oword.st.predef.surface.p0i32.v8i32(i32* @llvm.vc.predef.var.bss, i32 [[ADDR]], <8 x i32> [[SRC]])
; CHECK-NEXT: ret void
define spir_func void @oword_st(i32 %buf) {
  %addr = call i32 @src.i32()
  %src = call <8 x i32> @src.v8i32()
  call void @llvm.genx.oword.st.v8i32(i32 %buf, i32 %addr, <8 x i32> %src)
  ret void
}
