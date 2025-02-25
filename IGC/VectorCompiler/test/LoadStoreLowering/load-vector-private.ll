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
; COM: simplest vector load from addrspace(0)

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 0 (private) operations are lowered into svm/ugm intrinsics

define void @replace_load_i8_block(<16 x i8>* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.v16i8.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8>* %pi8
  ret void
}

define void @replace_load_i16_block(<16 x i16>* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.svm.block.ld.v16i16.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16>* %pi16
  ret void
}

define void @replace_load_i32_block(<16 x i32>* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32>* %pi32
  ret void
}

define void @replace_load_i64_block(<16 x i64>* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.svm.block.ld.v16i64.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64>* %pi64
  ret void
}

define void @replace_load_i8_block_unaligned(<16 x i8>* %pi8) {
  ; CHECK: call <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64
  %loaded = load <16 x i8>, <16 x i8>* %pi8, align 1
  ret void
}

define void @replace_load_i16_block_unaligned(<16 x i16>* %pi16) {
  ; CHECK: call <64 x i8> @llvm.genx.svm.gather.v64i8.v16i1.v16i64
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64
  %loaded = load <16 x i16>, <16 x i16>* %pi16, align 1
  ret void
}

define void @replace_load_i32_block_unaligned(<16 x i32>* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.svm.gather.v16i32.v16i1.v16i64
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v16i1.v2i8.v16i64
  %loaded = load <16 x i32>, <16 x i32>* %pi32, align 1
  ret void
}

define void @replace_load_i64_block_unaligned(<16 x i64>* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.svm.gather.v16i64.v16i1.v16i64
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v16i1.v2i8.v16i64
  %loaded = load <16 x i64>, <16 x i64>* %pi64, align 1
  ret void
}

define void @replace_load_i8_block_dwalign(<16 x i8>* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.unaligned.v16i8.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i32> undef)
  %loaded = load <16 x i8>, <16 x i8>* %pi8, align 4
  ret void
}

define void @replace_load_i16_block_dwalign(<16 x i16>* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.svm.block.ld.unaligned.v16i16.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i32> undef)
  %loaded = load <16 x i16>, <16 x i16>* %pi16, align 4
  ret void
}

define void @replace_load_i32_block_dwalign(<16 x i32>* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.svm.block.ld.unaligned.v16i32.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i32> undef)
  %loaded = load <16 x i32>, <16 x i32>* %pi32, align 4
  ret void
}

define void @replace_load_i64_block_dwalign(<16 x i64>* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.svm.block.ld.unaligned.v16i64.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <32 x i32> undef)
  %loaded = load <16 x i64>, <16 x i64>* %pi64, align 4
  ret void
}

define void @replace_load_i8_block_qwalign(<16 x i8>* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.unaligned.v16i8.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8>* %pi8, align 8
  ret void
}

define void @replace_load_i16_block_qwalign(<16 x i16>* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.svm.block.ld.unaligned.v16i16.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16>* %pi16, align 8
  ret void
}

define void @replace_load_i32_block_qwalign(<16 x i32>* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.svm.block.ld.unaligned.v16i32.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32>* %pi32, align 8
  ret void
}

define void @replace_load_i64_block_qwalign(<16 x i64>* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.svm.block.ld.unaligned.v16i64.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64>* %pi64, align 8
  ret void
}

define void @replace_load_i8_block_owalign(<16 x i8>* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.v16i8.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.ugm.v2i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 2, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8>* %pi8, align 16
  ret void
}

define void @replace_load_i16_block_owalign(<16 x i16>* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.svm.block.ld.v16i16.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16>* %pi16, align 16
  ret void
}

define void @replace_load_i32_block_owalign(<16 x i32>* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32>* %pi32, align 16
  ret void
}

define void @replace_load_i64_block_owalign(<16 x i64>* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.svm.block.ld.v16i64.i64(i64 %{{[0-9a-zA-Z.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64>* %pi64, align 16
  ret void
}

define void @replace_load_i8_block_1023bytes(<1023 x i8>* %pi8) {
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR0:%[0-9a-zA-Z.]+]])
  ; CHECK: [[ADDR128:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 128
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR128]])
  ; CHECK: [[ADDR256:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 256
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR256]])
  ; CHECK: [[ADDR384:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 384
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR384]])
  ; CHECK: [[ADDR512:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 512
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR512]])
  ; CHECK: [[ADDR640:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 640
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR640]])
  ; CHECK: [[ADDR768:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 768
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.v128i8.i64(i64 [[ADDR768]])
  ; CHECK: [[ADDR896:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 896
  ; CHECK: call <64 x i8> @llvm.genx.svm.block.ld.v64i8.i64(i64 [[ADDR896]])
  ; CHECK: [[ADDR960:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 960
  ; CHECK: call <32 x i8> @llvm.genx.svm.block.ld.v32i8.i64(i64 [[ADDR960]])
  ; CHECK: [[ADDR992:%[0-9a-zA-Z.]+]] = add i64 [[ADDR0]], 992
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.v16i8.i64(i64 [[ADDR992]])
  ; CHECK: call <60 x i8> @llvm.genx.svm.gather.v60i8.v15i1.v15i64
  ; CHECK-LSC: call <64 x i64> @llvm.vc.internal.lsc.load.ugm.v64i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i64> undef)
  ; CHECK-LSC: [[ADDR512:%[^ ]+]] = add i64 [[ADDR]], 512
  ; CHECK-LSC: call <32 x i64> @llvm.vc.internal.lsc.load.ugm.v32i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 7, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR512]], i16 1, i32 0, <32 x i64> undef)
  ; CHECK-LSC: [[ADDR768:%[^ ]+]] = add i64 [[ADDR]], 768
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.ugm.v16i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 6, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR768]], i16 1, i32 0, <16 x i64> undef)
  ; CHECK-LSC: [[ADDR896:%[^ ]+]] = add i64 [[ADDR]], 896
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR896]], i16 1, i32 0, <8 x i64> undef)
  ; CHECK-LSC: [[ADDR960:%[^ ]+]] = add i64 [[ADDR]], 960
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.ugm.v4i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR960]], i16 1, i32 0, <4 x i64> undef)
  ; CHECK-LSC: [[ADDR992:%[^ ]+]] = add i64 [[ADDR]], 992
  ; CHECK-LSC: call <3 x i64> @llvm.vc.internal.lsc.load.ugm.v3i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 3, <2 x i8> zeroinitializer, i64 0, i64 [[ADDR992]], i16 1, i32 0, <3 x i64> undef)
  ; CHECK-LSC: call <7 x i32> @llvm.vc.internal.lsc.load.ugm.v7i32.v7i1.v2i8.v7i64(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <7 x i64> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <7 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8>* %pi8
  ret void
}

define void @replace_load_i8_block_1023bytes_dwalign(<1023 x i8>* %pi8) {
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW0:%[0-9a-zA-Z.]+]])
  ; CHECK: [[ADDRDW128:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 128
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW128]])
  ; CHECK: [[ADDRDW256:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 256
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW256]])
  ; CHECK: [[ADDRDW384:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 384
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW384]])
  ; CHECK: [[ADDRDW512:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 512
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW512]])
  ; CHECK: [[ADDRDW640:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 640
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW640]])
  ; CHECK: [[ADDRDW768:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 768
  ; CHECK: call <128 x i8> @llvm.genx.svm.block.ld.unaligned.v128i8.i64(i64 [[ADDRDW768]])
  ; CHECK: [[ADDRDW896:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 896
  ; CHECK: call <64 x i8> @llvm.genx.svm.block.ld.unaligned.v64i8.i64(i64 [[ADDRDW896]])
  ; CHECK: [[ADDRDW960:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 960
  ; CHECK: call <32 x i8> @llvm.genx.svm.block.ld.unaligned.v32i8.i64(i64 [[ADDRDW960]])
  ; CHECK: [[ADDRDW992:%[0-9a-zA-Z.]+]] = add i64 [[ADDRDW0]], 992
  ; CHECK: call <16 x i8> @llvm.genx.svm.block.ld.unaligned.v16i8.i64(i64 [[ADDRDW992]])
  ; CHECK: call <60 x i8> @llvm.genx.svm.gather.v60i8.v15i1.v15i64
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW256:%[^ ]+]] = add i64 [[ADDRDW]], 256
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW256]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW512:%[^ ]+]] = add i64 [[ADDRDW]], 512
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.ugm.v64i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 8, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW512]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW768:%[^ ]+]] = add i64 [[ADDRDW]], 768
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.ugm.v32i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 7, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW768]], i16 1, i32 0, <32 x i32> undef)
  ; CHECK-LSC: [[ADDRDW896:%[^ ]+]] = add i64 [[ADDRDW]], 896
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.ugm.v16i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 6, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW896]], i16 1, i32 0, <16 x i32> undef)
  ; CHECK-LSC: [[ADDRDW960:%[^ ]+]] = add i64 [[ADDRDW]], 960
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.ugm.v8i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 5, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW960]], i16 1, i32 0, <8 x i32> undef)
  ; CHECK-LSC: [[ADDRDW992:%[^ ]+]] = add i64 [[ADDRDW]], 992
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.ugm.v4i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 4, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW992]], i16 1, i32 0, <4 x i32> undef)
  ; CHECK-LSC: [[ADDRDW1008:%[^ ]+]] = add i64 [[ADDRDW]], 1008
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 3, i8 3, <2 x i8> zeroinitializer, i64 0, i64 [[ADDRDW1008]], i16 1, i32 0, <3 x i32> undef)
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.ugm.v3i32.v3i1.v2i8.v3i64(<3 x i1> <i1 true, i1 true, i1 true>, i8 3, i8 5, i8 1, <2 x i8> zeroinitializer, i64 0, <3 x i64> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <3 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8>* %pi8, align 4
  ret void
}
