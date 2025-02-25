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
; COM: simplest load from addrspace(0)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 0 (private) operations are lowered into svm/stateless intrinsics

define i1 @replace_load_i1(i1* %p) {
; CHECK: [[LOAD:%[^ ]+]] = call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(
; CHECK: [[CAST1:%[^ ]+]] = bitcast <4 x i8> [[LOAD]] to <1 x i32>
; CHECK: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[CAST1]] to <1 x i8>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <1 x i8> [[TRUNC]] to i8
; CHECK: %res = trunc i8 [[CAST2]] to i1
; CHECK-LSC: [[LOAD:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(
; CHECK-LSC: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[LOAD]] to <1 x i8>
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <1 x i8> [[TRUNC]] to i8
; CHECK-LSC: %res = trunc i8 [[CAST]] to i1
  %res = load i1, i1* %p
  ret i1 %res
}

define <8 x i1> @replace_load_v8i1(<8 x i1>* %p) {
; CHECK: [[LOAD:%[^ ]+]] = call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i8> [[LOAD]] to <1 x i32>
; CHECK: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[CAST1]] to <1 x i8>
; CHECK: %res = bitcast <1 x i8> [[TRUNC]] to <8 x i1>
; CHECK-LSC: [[LOAD:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(
; CHECK-LSC: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[LOAD]] to <1 x i8>
; CHECK-LSC: %res = bitcast <1 x i8> [[TRUNC]] to <8 x i1>
  %res = load <8 x i1>, <8 x i1>* %p
  ret <8 x i1> %res
}

define <16 x i1> @replace_load_v16i1(<16 x i1>* %p) {
; CHECK: [[LOAD:%[^ ]+]] = call <4 x i8> @llvm.genx.svm.gather.v4i8.v1i1.v1i64(
; CHECK: [[CAST:%[^ ]+]] = bitcast <4 x i8> [[LOAD]] to <1 x i32>
; CHECK: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[CAST1]] to <1 x i16>
; CHECK: %res = bitcast <1 x i16> [[TRUNC]] to <16 x i1>
; CHECK-LSC: [[LOAD:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.v1i64(
; CHECK-LSC: [[TRUNC:%[^ ]+]] = trunc <1 x i32> [[LOAD]] to <1 x i16>
; CHECK-LSC: %res = bitcast <1 x i16> [[TRUNC]] to <16 x i1>
  %res = load <16 x i1>, <16 x i1>* %p
  ret <16 x i1> %res
}

define <32 x i1> @replace_load_v32i1(<32 x i1>* %p) {
; CHECK: [[LOAD:%[^ ]+]] = call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(
; CHECK: %res = bitcast <1 x i32> [[LOAD]] to <32 x i1>
; CHECK-LSC: [[LOAD:%[^ ]+]] = call <1 x i32> @llvm.vc.internal.lsc.load.ugm.v1i32.v1i1.v2i8.i64(
; CHECK-LSC: %res = bitcast <1 x i32> [[LOAD]] to <32 x i1>
  %res = load <32 x i1>, <32 x i1>* %p
  ret <32 x i1> %res
}

define <17 x i1> @replace_load_v17i1(<17 x i1>* %p) {
; CHECK: [[LOAD:%[^ ]+]] = call <12 x i8> @llvm.genx.svm.gather.v12i8.v3i1.v3i64(
; CHECK: [[CAST1:%[^ ]+]] = bitcast <12 x i8> [[LOAD]] to <3 x i32>
; CHECK: [[TRUNC:%[^ ]+]] = trunc <3 x i32> [[CAST1]] to <3 x i8>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <3 x i8> [[TRUNC]] to <24 x i1>
; CHECK: %res = call <17 x i1> @llvm.genx.rdpredregion.v17i1.v24i1(<24 x i1> [[CAST2]], i32 0)
; CHECK-LSC: [[LOAD:%[^ ]+]] = call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v3i1.v2i8.v3i64(
; CHECK-LSC: [[TRUNC:%[^ ]+]] = trunc <3 x i32> [[LOAD]] to <3 x i8>
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <3 x i8> [[TRUNC]] to <24 x i1>
; CHECK-LSC: %res = call <17 x i1> @llvm.genx.rdpredregion.v17i1.v24i1(<24 x i1> [[CAST]], i32 0)
  %res = load <17 x i1>, <17 x i1>* %p
  ret <17 x i1> %res
}
