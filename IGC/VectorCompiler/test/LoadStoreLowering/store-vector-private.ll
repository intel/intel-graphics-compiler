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

define void @replace_store_i8_block(<16 x i8>* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i8(i64 %{{[a-zA-Z0-9.]+}}, <16 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <2 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i8> %val, <16 x i8>* %pi8
  ret void
}

define void @replace_store_i16_block(<16 x i16>* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i16(i64 %{{[a-zA-Z0-9.]+}}, <16 x i16> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <4 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i16> %val, <16 x i16>* %pi16
  ret void
}

define void @replace_store_i32_block(<16 x i32>* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i32(i64 %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <8 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i32> %val, <16 x i32>* %pi32
  ret void
}

define void @replace_store_i64_block(<16 x i64>* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i64(i64 %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i64> %val, <16 x i64>* %pi64
  ret void
}

define void @replace_store_i8_block_unaligned(<16 x i8>* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <64 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32
  store <16 x i8> %val, <16 x i8>* %pi8, align 1
  ret void
}

define void @replace_store_i16_block_unaligned(<16 x i16>* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 1, <16 x i64> %{{[a-zA-Z0-9.]+}}, <64 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32
  store <16 x i16> %val, <16 x i16>* %pi16, align 1
  ret void
}

define void @replace_store_i32_block_unaligned(<16 x i32>* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i32
  store <16 x i32> %val, <16 x i32>* %pi32, align 1
  ret void
}

define void @replace_store_i64_block_unaligned(<16 x i64>* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v16i1.v2i8.v16i64.v16i64
  store <16 x i64> %val, <16 x i64>* %pi64, align 1
  ret void
}

define void @replace_store_i8_block_dwalign(<16 x i8>* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <64 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <4 x i32> %{{[a-zA-Z0-9.]+}})
  store <16 x i8> %val, <16 x i8>* %pi8, align 4
  ret void
}

define void @replace_store_i16_block_dwalign(<16 x i16>* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v64i8(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 1, <16 x i64> %{{[a-zA-Z0-9.]+}}, <64 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <8 x i32> %{{[a-zA-Z0-9.]+}})
  store <16 x i16> %val, <16 x i16>* %pi16, align 4
  ret void
}

define void @replace_store_i32_block_dwalign(<16 x i32>* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <16 x i32> %{{[a-zA-Z0-9.]+}})
  store <16 x i32> %val, <16 x i32>* %pi32, align 4
  ret void
}

define void @replace_store_i64_block_dwalign(<16 x i64>* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <32 x i32> %{{[a-zA-Z0-9.]+}})
  store <16 x i64> %val, <16 x i64>* %pi64, align 4
  ret void
}

define void @replace_store_i8_block_owalign(<16 x i8>* %pi8, <16 x i8> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i8(i64 %{{[a-zA-Z0-9.]+}}, <16 x i8> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v2i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <2 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i8> %val, <16 x i8>* %pi8, align 16
  ret void
}

define void @replace_store_i16_block_owalign(<16 x i16>* %pi16, <16 x i16> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i16(i64 %{{[a-zA-Z0-9.]+}}, <16 x i16> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <4 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i16> %val, <16 x i16>* %pi16, align 16
  ret void
}

define void @replace_store_i32_block_owalign(<16 x i32>* %pi32, <16 x i32> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i32(i64 %{{[a-zA-Z0-9.]+}}, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <8 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i32> %val, <16 x i32>* %pi32, align 16
  ret void
}

define void @replace_store_i64_block_owalign(<16 x i64>* %pi64, <16 x i64> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i64(i64 %{{[a-zA-Z0-9.]+}}, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}})
  store <16 x i64> %val, <16 x i64>* %pi64, align 16
  ret void
}

define void @replace_store_i8_block_1023bytes(<1023 x i8>* %p, <1023 x i8> %val) {
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR0:%[0-9a-zA-Z.]+]], <128 x i8>
; CHECK: [[ADDR128:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 128
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR128]], <128 x i8>
; CHECK: [[ADDR256:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 256
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR256]], <128 x i8>
; CHECK: [[ADDR384:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 384
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR384]], <128 x i8>
; CHECK: [[ADDR512:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 512
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR512]], <128 x i8>
; CHECK: [[ADDR640:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 640
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR640]], <128 x i8>
; CHECK: [[ADDR768:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 768
; CHECK: call void @llvm.genx.svm.block.st.i64.v128i8(i64 [[ADDR768]], <128 x i8>
; CHECK: [[ADDR896:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 896
; CHECK: call void @llvm.genx.svm.block.st.i64.v64i8(i64 [[ADDR896]], <64 x i8>
; CHECK: [[ADDR960:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 960
; CHECK: call void @llvm.genx.svm.block.st.i64.v32i8(i64 [[ADDR960]], <32 x i8>
; CHECK: [[ADDR992:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 992
; CHECK: call void @llvm.genx.svm.block.st.i64.v16i8(i64 [[ADDR992]], <16 x i8>
; CHECK: call void @llvm.genx.svm.scatter.v15i1.v15i64.v60i8
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR:%[a-zA-Z0-9.]+]], i16 1, i32 0, <64 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR512:%[^ ]+]] = add i64 [[ADDR]], 512
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 7, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR512]], i16 1, i32 0, <32 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR768:%[^ ]+]] = add i64 [[ADDR]], 768
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR768]], i16 1, i32 0, <16 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR896:%[^ ]+]] = add i64 [[ADDR]], 896
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR896]], i16 1, i32 0, <8 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR960:%[^ ]+]] = add i64 [[ADDR]], 960
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR960]], i16 1, i32 0, <4 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDR992:%[^ ]+]] = add i64 [[ADDR]], 992
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v3i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 3, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR992]], i16 1, i32 0, <3 x i64> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v7i1.v2i8.v7i64.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <7 x i64> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <7 x i32> %{{[a-zA-Z0-9.]+}})
  store <1023 x i8> %val, <1023 x i8>* %p
  ret void
}

define void @replace_store_i8_block_1023bytes_dwalign(<1023 x i8>* %p, <1023 x i8> %val) {
; CHECK: call void @llvm.genx.svm.scatter.v1023i1.v1023i64.v4092i8
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW:%[a-zA-Z0-9.]+]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW256:%[^ ]+]] = add i64 [[ADDRDW]], 256
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW256]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW512:%[^ ]+]] = add i64 [[ADDRDW]], 512
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v64i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW512]], i16 1, i32 0, <64 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW768:%[^ ]+]] = add i64 [[ADDRDW]], 768
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v32i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW768]], i16 1, i32 0, <32 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW896:%[^ ]+]] = add i64 [[ADDRDW]], 896
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v16i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW896]], i16 1, i32 0, <16 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW960:%[^ ]+]] = add i64 [[ADDRDW]], 960
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v8i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW960]], i16 1, i32 0, <8 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW992:%[^ ]+]] = add i64 [[ADDRDW]], 992
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW992]], i16 1, i32 0, <4 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: [[ADDRDW1008:%[^ ]+]] = add i64 [[ADDRDW]], 1008
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v3i32(<1 x i1> <i1 true>, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW1008]], i16 1, i32 0, <3 x i32> %{{[a-zA-Z0-9.]+}})
; CHECK-LSC: call void @llvm.vc.internal.lsc.store.ugm.v3i1.v2i8.v3i64.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %{{[a-zA-Z0-9.]+}}, i16 1, i32 0, <3 x i32> %{{[a-zA-Z0-9.]+}})
  store <1023 x i8> %val, <1023 x i8>* %p, align 4
  ret void
}
