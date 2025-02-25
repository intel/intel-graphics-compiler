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

; COM: Basic test on store lowering pass
; COM: simplest store to addrspace(0)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 0 (private) operations are lowered into svm/stateless intrinsics

define void @replace_store_i1(i1* %p, i1 %val) {
; CHECK: [[ZEXT1:%[^ ]+]] = zext i1 %val to i8
; CHECK: [[CAST1:%[^ ]+]] = bitcast i8 [[ZEXT1]] to <1 x i8>
; CHECK: [[ZEXT2:%[^ ]+]] = zext <1 x i8> [[CAST1]] to <1 x i32>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <1 x i32> [[ZEXT2]] to <4 x i8>
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i64.v4i8(
; CHECK-SAME: <4 x i8> [[CAST2]])
; CHECK-LSC: [[ZEXT1:%[^ ]+]] = zext i1 %val to i8
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast i8 [[ZEXT1]] to <1 x i8>
; CHECK-LSC: [[ZEXT2:%[^ ]+]] = zext <1 x i8> [[CAST]] to <1 x i32>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.v1i64.v1i32(
; CHECK-LSC-SAME: <1 x i32> [[ZEXT2]]
  store i1 %val, i1* %p
  ret void
}

define void @replace_store_v8i1(<8 x i1>* %p, <8 x i1> %val) {
; CHECK: [[CAST1:%[^ ]+]] = bitcast <8 x i1> %val to <1 x i8>
; CHECK: [[ZEXT2:%[^ ]+]] = zext <1 x i8> [[CAST1]] to <1 x i32>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <1 x i32> [[ZEXT2]] to <4 x i8>
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i64.v4i8(
; CHECK-SAME: <4 x i8> [[CAST2]])
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <8 x i1> %val to <1 x i8>
; CHECK-LSC: [[ZEXT:%[^ ]+]] = zext <1 x i8> [[CAST]] to <1 x i32>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.v1i64.v1i32(
; CHECK-LSC-SAME: <1 x i32> [[ZEXT]]
  store <8 x i1> %val, <8 x i1>* %p
  ret void
}

define void @replace_store_v16i1(<16 x i1>* %p, <16 x i1> %val) {
; CHECK: [[CAST1:%[^ ]+]] = bitcast <16 x i1> %val to <1 x i16>
; CHECK: [[ZEXT2:%[^ ]+]] = zext <1 x i16> [[CAST1]] to <1 x i32>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <1 x i32> [[ZEXT2]] to <4 x i8>
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i64.v4i8(
; CHECK-SAME: <4 x i8> [[CAST2]])
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <16 x i1> %val to <1 x i16>
; CHECK-LSC: [[ZEXT:%[^ ]+]] = zext <1 x i16> [[CAST]] to <1 x i32>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.v1i64.v1i32(
; CHECK-LSC-SAME: <1 x i32> [[ZEXT]]
  store <16 x i1> %val, <16 x i1>* %p
  ret void
}

define void @replace_store_v32i1(<32 x i1>* %p, <32 x i1> %val) {
; CHECK: [[CAST:%[^ ]+]] = bitcast <32 x i1> %val to <1 x i32>
; CHECK: call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(
; CHECK-SAME: <1 x i32> [[CAST]])
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <32 x i1> %val to <1 x i32>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v1i32(
; CHECK-LSC-SAME: <1 x i32> [[CAST]]
  store <32 x i1> %val, <32 x i1>* %p
  ret void
}

define void @replace_store_v17i1(<17 x i1>* %p, <17 x i1> %val) {
; CHECK: [[WRRGN:%[^ ]+]] = call <24 x i1> @llvm.genx.wrpredregion.v24i1.v17i1(<24 x i1> zeroinitializer, <17 x i1> %val, i32 0)
; CHECK: [[CAST:%[^ ]+]] = bitcast <24 x i1> [[WRRGN]] to <3 x i8>
; CHECK: [[ZEXT:%[^ ]+]] = zext <3 x i8> [[CAST]] to <3 x i32>
; CHECK: [[CAST2:%[^ ]+]] = bitcast <3 x i32> [[ZEXT]] to <12 x i8>
; CHECK: call void @llvm.genx.svm.scatter.v3i1.v3i64.v12i8(
; CHECK-SAME: <12 x i8> [[CAST2]])
; CHECK-LSC: [[WRRGN:%[^ ]+]] = call <24 x i1> @llvm.genx.wrpredregion.v24i1.v17i1(<24 x i1> zeroinitializer, <17 x i1> %val, i32 0)
; CHECK-LSC: [[CAST:%[^ ]+]] = bitcast <24 x i1> [[WRRGN]] to <3 x i8>
; CHECK-LSC: [[ZEXT:%[^ ]+]] = zext <3 x i8> [[CAST]] to <3 x i32>
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v3i32(
; CHECK-LSC-SAME: <3 x i32> [[ZEXT]]
  store <17 x i1> %val, <17 x i1>* %p
  ret void
}
