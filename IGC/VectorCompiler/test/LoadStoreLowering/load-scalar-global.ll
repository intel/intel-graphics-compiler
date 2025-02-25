;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on load lowering pass
; COM: simplest load from addrspace(1)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 1 (global) operations are lowered into svm/ugm intrinsics

define void @replace_load_i8(i8 addrspace(1)* %pi8) {
; CHECK: call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, <4 x i8> undef)
; CHECK-LSC: [[LD8:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: trunc <1 x i32> [[LD8]] to <1 x i8>
  %loaded = load i8, i8 addrspace(1)* %pi8
  ret void
}

define void @replace_load_i16(i16 addrspace(1)* %pi16) {
; CHECK: call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(<1 x i1> <i1 true>, i32 1, <1 x i64> %{{[a-zA-Z0-9.]+}}, <4 x i8> undef)
; CHECK-LSC: [[LD16:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: trunc <1 x i32> [[LD16]] to <1 x i16>
  %loaded = load i16, i16 addrspace(1)* %pi16
  ret void
}

define void @replace_load_i32(i32 addrspace(1)* %pi32) {
; CHECK: call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, <1 x i32> undef)
; CHECK-LSC: call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
  %loaded = load i32, i32 addrspace(1)* %pi32
  ret void
}

define void @replace_load_i64(i64 addrspace(1)* %pi64) {
; CHECK: call <1 x i64> @llvm.genx.svm.gather.v1i64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, <1 x i64> undef)
; CHECK-LSC: call <1 x i64> @llvm.vc.internal.lsc.load.ugm.v1i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i64> undef)
  %loaded = load i64, i64 addrspace(1)* %pi64
  ret void
}

define void @replace_load_f16(half addrspace(1)* %pf16) {
; CHECK: call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(<1 x i1> <i1 true>, i32 1, <1 x i64> %{{[a-zA-Z0-9.]+}}, <4 x i8> undef)
; CHECK-LSC: [[LD16F:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(<1 x i1> <i1 true>, i8 3, i8 6, i8 1, <2 x i8> zeroinitializer, i64 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: [[TRUNC16F:%[0-9a-zA-Z.]+]] = trunc <1 x i32> [[LD16F]] to <1 x i16>
; CHECK-LSC: bitcast <1 x i16> [[TRUNC16F]] to <1 x half>
  %loaded = load half, half addrspace(1)* %pf16
  ret void
}

define void @replace_load_f32(float addrspace(1)* %pf32) {
; CHECK: call <1 x float> @llvm.genx.svm.gather.v1f32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, <1 x float> undef)
; CHECK-LSC: call <1 x float> @llvm.vc.internal.lsc.load.ugm.v1f32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 1, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x float> undef)
  %loaded = load float, float addrspace(1)* %pf32
  ret void
}

define void @replace_load_f64(double addrspace(1)* %pf64) {
; CHECK: call <1 x double> @llvm.genx.svm.gather.v1f64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %{{[a-zA-Z0-9.]+}}, <1 x double> undef)
; CHECK-LSC: call <1 x double> @llvm.vc.internal.lsc.load.ugm.v1f64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 1, <2 x i8> <i8 1, i8 1>, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x double> undef)
  %loaded = load double, double addrspace(1)* %pf64, !nontemporal !0
  ret void
}

!0 = !{i32 1}
