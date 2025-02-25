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
; COM: simplest vector load from addrspace(6)

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 6 (32-bit global) operations are lowered into bti(255) intrinsics

define void @replace_load_i8_block(<16 x i8> addrspace(6)* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.bti.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(6)* %pi8
  ret void
}

define void @replace_load_i16_block(<16 x i16> addrspace(6)* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.oword.ld.v16i16(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.bti.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(6)* %pi16
  ret void
}

define void @replace_load_i32_block(<16 x i32> addrspace(6)* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.bti.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(6)* %pi32
  ret void
}

define void @replace_load_i64_block(<16 x i64> addrspace(6)* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.bti.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(6)* %pi64
  ret void
}

define void @replace_load_i8_block_dwalign(<16 x i8> addrspace(6)* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.bti.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i32> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(6)* %pi8, align 4
  ret void
}

define void @replace_load_i16_block_dwalign(<16 x i16> addrspace(6)* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.oword.ld.unaligned.v16i16(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i32> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(6)* %pi16, align 4
  ret void
}

define void @replace_load_i32_block_dwalign(<16 x i32> addrspace(6)* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 6, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i32> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(6)* %pi32, align 4
  ret void
}

define void @replace_load_i64_block_dwalign(<16 x i64> addrspace(6)* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.oword.ld.unaligned.v16i64(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.bti.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 7, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <32 x i32> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(6)* %pi64, align 4
  ret void
}

define void @replace_load_i8_block_qwalign(<16 x i8> addrspace(6)* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.bti.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(6)* %pi8, align 8
  ret void
}

define void @replace_load_i16_block_qwalign(<16 x i16> addrspace(6)* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.oword.ld.unaligned.v16i16(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.bti.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(6)* %pi16, align 8
  ret void
}

define void @replace_load_i32_block_qwalign(<16 x i32> addrspace(6)* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.bti.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(6)* %pi32, align 8
  ret void
}

define void @replace_load_i64_block_qwalign(<16 x i64> addrspace(6)* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.oword.ld.unaligned.v16i64(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.bti.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(6)* %pi64, align 8
  ret void
}

define void @replace_load_i8_block_owalign(<16 x i8> addrspace(6)* %pi8) {
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.bti.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(6)* %pi8, align 16
  ret void
}

define void @replace_load_i16_block_owalign(<16 x i16> addrspace(6)* %pi16) {
  ; CHECK: call <16 x i16> @llvm.genx.oword.ld.v16i16(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.bti.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(6)* %pi16, align 16
  ret void
}

define void @replace_load_i32_block_owalign(<16 x i32> addrspace(6)* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.bti.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(6)* %pi32, align 16
  ret void
}

define void @replace_load_i64_block_owalign(<16 x i64> addrspace(6)* %pi64) {
  ; CHECK: call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 255, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.bti.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(6)* %pi64, align 16
  ret void
}

define void @replace_load_i8_block_1023bytes(<1023 x i8> addrspace(6)* %pi8) {
  ; CHECK: %[[ADDR0:[^ ]+]] = lshr i32 %[[BASE:[^,]+]], 4
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR0]])
  ; CHECK: %[[ADDR128:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 8
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR128]])
  ; CHECK: %[[ADDR256:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 16
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR256]])
  ; CHECK: %[[ADDR384:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 24
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR384]])
  ; CHECK: %[[ADDR512:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 32
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR512]])
  ; CHECK: %[[ADDR640:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 40
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR640]])
  ; CHECK: %[[ADDR768:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 48
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 255, i32 %[[ADDR768]])
  ; CHECK: %[[ADDR896:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 56
  ; CHECK: call <64 x i8> @llvm.genx.oword.ld.v64i8(i32 0, i32 255, i32 %[[ADDR896]])
  ; CHECK: %[[ADDR960:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 60
  ; CHECK: call <32 x i8> @llvm.genx.oword.ld.v32i8(i32 0, i32 255, i32 %[[ADDR960]])
  ; CHECK: %[[ADDR992:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 62
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 255, i32 %[[ADDR992]])
  ; CHECK: call <15 x i32> @llvm.genx.gather.scaled.v15i32.v15i1.v15i32(<15 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, i16 0, i32 255, i32 %[[BASE]], <15 x i32> <i32 1008, i32 1009, i32 1010, i32 1011, i32 1012, i32 1013, i32 1014, i32 1015, i32 1016, i32 1017, i32 1018, i32 1019, i32 1020, i32 1021, i32 1022>, <15 x i32> undef)

  ; CHECK-LSC: call <64 x i64> @llvm.vc.internal.lsc.load.bti.v64i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i64> undef)
  ; CHECK-LSC: [[ADDR512:%[^ ]+]] = add i32 [[ADDR]], 512
  ; CHECK-LSC: call <32 x i64> @llvm.vc.internal.lsc.load.bti.v32i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 7, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR512]], i16 1, i32 0, <32 x i64> undef)
  ; CHECK-LSC: [[ADDR768:%[^ ]+]] = add i32 [[ADDR]], 768
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.bti.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR768]], i16 1, i32 0, <16 x i64> undef)
  ; CHECK-LSC: [[ADDR896:%[^ ]+]] = add i32 [[ADDR]], 896
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.bti.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR896]], i16 1, i32 0, <8 x i64> undef)
  ; CHECK-LSC: [[ADDR960:%[^ ]+]] = add i32 [[ADDR]], 960
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.bti.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR960]], i16 1, i32 0, <4 x i64> undef)
  ; CHECK-LSC: [[ADDR992:%[^ ]+]] = add i32 [[ADDR]], 992
  ; CHECK-LSC: call <3 x i64> @llvm.vc.internal.lsc.load.bti.v3i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 3, <2 x i8> zeroinitializer, i32 255, i32 [[ADDR992]], i16 1, i32 0, <3 x i64> undef)
  ; CHECK-LSC: call <7 x i32> @llvm.vc.internal.lsc.load.bti.v7i32.v7i1.v2i8.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 255, <7 x i32> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <7 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8> addrspace(6)* %pi8
  ret void
}

define void @replace_load_i8_block_1023bytes_dwalign(<1023 x i8> addrspace(6)* %pi8) {
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW0:[A-Za-z0-9.]+]])
  ; CHECK: %[[ADDRDW128:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 128
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW128]])
  ; CHECK: %[[ADDRDW256:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 256
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW256]])
  ; CHECK: %[[ADDRDW384:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 384
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW384]])
  ; CHECK: %[[ADDRDW512:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 512
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW512]])
  ; CHECK: %[[ADDRDW640:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 640
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW640]])
  ; CHECK: %[[ADDRDW768:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 768
  ; CHECK: call <128 x i8> @llvm.genx.oword.ld.unaligned.v128i8(i32 0, i32 255, i32 %[[ADDRDW768]])
  ; CHECK: %[[ADDRDW896:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 896
  ; CHECK: call <64 x i8> @llvm.genx.oword.ld.unaligned.v64i8(i32 0, i32 255, i32 %[[ADDRDW896]])
  ; CHECK: %[[ADDRDW960:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 960
  ; CHECK: call <32 x i8> @llvm.genx.oword.ld.unaligned.v32i8(i32 0, i32 255, i32 %[[ADDRDW960]])
  ; CHECK: %[[ADDRDW992:[A-Za-z0-9.]+]] = add i32 %[[ADDRDW0]], 992
  ; CHECK: call <16 x i8> @llvm.genx.oword.ld.unaligned.v16i8(i32 0, i32 255, i32 %[[ADDRDW992]])
  ; CHECK: call <15 x i32> @llvm.genx.gather.scaled.v15i32.v15i1.v15i32(<15 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, i16 0, i32 255, i32 %[[ADDRDW0]], <15 x i32> <i32 1008, i32 1009, i32 1010, i32 1011, i32 1012, i32 1013, i32 1014, i32 1015, i32 1016, i32 1017, i32 1018, i32 1019, i32 1020, i32 1021, i32 1022>, <15 x i32> undef)

  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.bti.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW256:%[^ ]+]] = add i32 [[ADDRDW]], 256
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.bti.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW256]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW512:%[^ ]+]] = add i32 [[ADDRDW]], 512
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.bti.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW512]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW768:%[^ ]+]] = add i32 [[ADDRDW]], 768
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.bti.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 7, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW768]], i16 1, i32 0, <32 x i32> undef)
  ; CHECK-LSC: [[ADDRDW896:%[^ ]+]] = add i32 [[ADDRDW]], 896
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.bti.v16i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 6, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW896]], i16 1, i32 0, <16 x i32> undef)
  ; CHECK-LSC: [[ADDRDW960:%[^ ]+]] = add i32 [[ADDRDW]], 960
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.bti.v8i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW960]], i16 1, i32 0, <8 x i32> undef)
  ; CHECK-LSC: [[ADDRDW992:%[^ ]+]] = add i32 [[ADDRDW]], 992
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.bti.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW992]], i16 1, i32 0, <4 x i32> undef)
  ; CHECK-LSC: [[ADDRDW1008:%[^ ]+]] = add i32 [[ADDRDW]], 1008
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.bti.v3i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 3, <2 x i8> zeroinitializer, i32 255, i32 [[ADDRDW1008]], i16 1, i32 0, <3 x i32> undef)
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.bti.v3i32.v3i1.v2i8.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 255, <3 x i32> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <3 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8> addrspace(6)* %pi8, align 4
  ret void
}
