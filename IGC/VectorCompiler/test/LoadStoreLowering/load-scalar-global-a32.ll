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
; COM: simplest load from addrspace(6)

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx-unknown-unknown"

; Address space 6 (global 32-bit ptr) operations are lowered into bti(255) intrinsics

define void @replace_load_i8(i8 addrspace(6)* %pi8) {
; CHECK: call <1 x i32> @llvm.genx.gather.scaled.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 0, i16 0, i32 255, i32 %{{[^ ,]+}}, <1 x i32> zeroinitializer, <1 x i32> undef)
; CHECK-LSC: [[LD8:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.bti.v1i32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 255, <1 x i32> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: trunc <1 x i32> [[LD8]] to <1 x i8>
  %loaded = load i8, i8 addrspace(6)* %pi8
  ret void
}

define void @replace_load_i16(i16 addrspace(6)* %pi16) {
; CHECK: call <1 x i32> @llvm.genx.gather.scaled.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 1, i16 0, i32 255, i32 %{{[^ ,]+}}, <1 x i32> zeroinitializer, <1 x i32> undef)
; CHECK-LSC: [[LD16:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.bti.v1i32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 255, <1 x i32> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: trunc <1 x i32> [[LD16]] to <1 x i16>
  %loaded = load i16, i16 addrspace(6)* %pi16
  ret void
}

define void @replace_load_i32(i32 addrspace(6)* %pi32) {
; CHECK: call <1 x i32> @llvm.genx.gather.scaled.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 2, i16 0, i32 255, i32 %{{[^ ,]+}}, <1 x i32> zeroinitializer, <1 x i32> undef)
; CHECK-LSC: call <1 x i32> @llvm.vc.internal.lsc.load.bti.v1i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 255, i32 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
  %loaded = load i32, i32 addrspace(6)* %pi32
  ret void
}

define void @replace_load_i64(i64 addrspace(6)* %pi64) {
; CHECK: call <2 x i32> @llvm.genx.gather.scaled.v2i32.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i32 2, i16 0, i32 255, i32 %{{[^ ,]+}}, <2 x i32> <i32 0, i32 4>, <2 x i32> undef)
; CHECK-LSC: call <1 x i64> @llvm.vc.internal.lsc.load.bti.v1i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 255, i32 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i64> undef)
  %loaded = load i64, i64 addrspace(6)* %pi64
  ret void
}

define void @replace_load_f16(half addrspace(6)* %pf16) {
; CHECK: call <1 x i32> @llvm.genx.gather.scaled.v1i32.v1i1.v1i32(<1 x i1> <i1 true>, i32 1, i16 0, i32 255, i32 %{{[^ ,]+}}, <1 x i32> zeroinitializer, <1 x i32> undef)
; CHECK-LSC: [[LD16F:%[0-9a-zA-Z.]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.bti.v1i32.v1i1.v2i8.v1i32(<1 x i1> <i1 true>, i8 2, i8 6, i8 1, <2 x i8> zeroinitializer, i32 255, <1 x i32> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x i32> undef)
; CHECK-LSC: [[TRUNC16F:%[0-9a-zA-Z.]+]] = trunc <1 x i32> [[LD16F]] to <1 x i16>
; CHECK-LSC: bitcast <1 x i16> [[TRUNC16F]] to <1 x half>
  %loaded = load half, half addrspace(6)* %pf16
  ret void
}

define void @replace_load_f32(float addrspace(6)* %pf32) {
; CHECK: call <1 x float> @llvm.genx.gather.scaled.v1f32.v1i1.v1i32(<1 x i1> <i1 true>, i32 2, i16 0, i32 255, i32 %{{[^ ,]+}}, <1 x i32> zeroinitializer, <1 x float> undef)
; CHECK-LSC: call <1 x float> @llvm.vc.internal.lsc.load.bti.v1f32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 1, <2 x i8> zeroinitializer, i32 255, i32 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x float> undef)
  %loaded = load float, float addrspace(6)* %pf32
  ret void
}

define void @replace_load_f64(double addrspace(6)* %pf64) {
; CHECK: call <2 x i32> @llvm.genx.gather.scaled.v2i32.v2i1.v2i32(<2 x i1> <i1 true, i1 true>, i32 2, i16 0, i32 255, i32 %{{[^ ,]+}}, <2 x i32> <i32 0, i32 4>, <2 x i32> undef)
; CHECK-LSC: call <1 x double> @llvm.vc.internal.lsc.load.bti.v1f64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 1, <2 x i8> zeroinitializer, i32 255, i32 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <1 x double> undef)
  %loaded = load double, double addrspace(6)* %pf64
  ret void
}
