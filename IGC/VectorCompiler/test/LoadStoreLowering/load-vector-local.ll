;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-SLM-BLOCK %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen11 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-SLM-BLOCK %s
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-SLM-BLOCK-NOT: WARNING
; CHECK-SLM-BLOCK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

; COM: Basic test on load lowering pass
; COM: simplest vector load from addrspace(3)

target datalayout = "e-p:64:64-p3:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Address space 3 (local) operations are lowered into bti/slm intrinsics

define void @replace_load_i8_block(<16 x i8> addrspace(3)* %pi8) {
  ; CHECK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SAME: i1 true>, i32 0, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SAME: <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>,
  ; CHECK-SAME: <16 x i32> undef)
  ; CHECK-SLM-BLOCK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.slm.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(3)* %pi8
  ret void
}

define void @replace_load_i16_block(<16 x i16> addrspace(3)* %pi16) {
  ; CHECK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SAME: i1 true>, i32 1, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SAME: <16 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14, i32 16, i32 18, i32 20, i32 22, i32 24, i32 26, i32 28, i32 30>,
  ; CHECK-SAME: <16 x i32> undef)
  ; CHECK-SLM-BLOCK: call <16 x i16> @llvm.genx.oword.ld.v16i16(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(3)* %pi16
  ret void
}

define void @replace_load_i32_block(<16 x i32> addrspace(3)* %pi32) {
  ; CHECK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SAME: i1 true>, i32 2, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SAME: <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>,
  ; CHECK-SAME: <16 x i32> undef)
  ; CHECK-SLM-BLOCK: call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(3)* %pi32
  ret void
}

define void @replace_load_i64_block(<16 x i64> addrspace(3)* %pi64) {
  ; CHECK: call <32 x i32> @llvm.genx.gather.scaled.v32i32.v32i1.v32i32(<32 x i1> <i1 true
  ; CHECK-SAME: i1 true>, i32 2, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SAME: <32 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60,
  ; CHECK-SAME: i32 64, i32 68, i32 72, i32 76, i32 80, i32 84, i32 88, i32 92, i32 96, i32 100, i32 104, i32 108, i32 112, i32 116, i32 120, i32 124>,
  ; CHECK-SAME: <32 x i32> undef)
  ; CHECK-SLM-BLOCK: call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(3)* %pi64
  ret void
}

define void @replace_load_i8_block_dwalign(<16 x i8> addrspace(3)* %pi8) {
  ; CHECK-SLM-BLOCK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SLM-BLOCK-SAME: i1 true>, i32 0, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>,
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> undef)
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.slm.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i32> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(3)* %pi8, align 4
  ret void
}

define void @replace_load_i16_block_dwalign(<16 x i16> addrspace(3)* %pi16) {
  ; CHECK-SLM-BLOCK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SLM-BLOCK-SAME: i1 true>, i32 1, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14, i32 16, i32 18, i32 20, i32 22, i32 24, i32 26, i32 28, i32 30>,
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> undef)
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i32> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(3)* %pi16, align 4
  ret void
}

define void @replace_load_i32_block_dwalign(<16 x i32> addrspace(3)* %pi32) {
  ; CHECK-SLM-BLOCK: call <16 x i32> @llvm.genx.gather.scaled.v16i32.v16i1.v16i32(<16 x i1> <i1 true
  ; CHECK-SLM-BLOCK-SAME: i1 true>, i32 2, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60>,
  ; CHECK-SLM-BLOCK-SAME: <16 x i32> undef)
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i32> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(3)* %pi32, align 4
  ret void
}

define void @replace_load_i64_block_dwalign(<16 x i64> addrspace(3)* %pi64) {
  ; CHECK-SLM-BLOCK: call <32 x i32> @llvm.genx.gather.scaled.v32i32.v32i1.v32i32(<32 x i1> <i1 true
  ; CHECK-SLM-BLOCK-SAME: i1 true>, i32 2, i16 0, i32 254, i32 %{{[^ ,]+}},
  ; CHECK-SLM-BLOCK-SAME: <32 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60,
  ; CHECK-SLM-BLOCK-SAME: i32 64, i32 68, i32 72, i32 76, i32 80, i32 84, i32 88, i32 92, i32 96, i32 100, i32 104, i32 108, i32 112, i32 116, i32 120, i32 124>,
  ; CHECK-SLM-BLOCK-SAME: <32 x i32> undef)
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 7, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <32 x i32> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(3)* %pi64, align 4
  ret void
}

define void @replace_load_i8_block_qwalign(<16 x i8> addrspace(3)* %pi8) {
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.slm.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(3)* %pi8, align 8
  ret void
}

define void @replace_load_i16_block_qwalign(<16 x i16> addrspace(3)* %pi16) {
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(3)* %pi16, align 8
  ret void
}

define void @replace_load_i32_block_qwalign(<16 x i32> addrspace(3)* %pi32) {
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(3)* %pi32, align 8
  ret void
}

define void @replace_load_i64_block_qwalign(<16 x i64> addrspace(3)* %pi64) {
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(3)* %pi64, align 8
  ret void
}

define void @replace_load_i8_block_owalign(<16 x i8> addrspace(3)* %pi8) {
  ; CHECK-SLM-BLOCK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <2 x i64> @llvm.vc.internal.lsc.load.slm.v2i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 2, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <2 x i64> undef)
  %loaded = load <16 x i8>, <16 x i8> addrspace(3)* %pi8, align 16
  ret void
}

define void @replace_load_i16_block_owalign(<16 x i16> addrspace(3)* %pi16) {
  ; CHECK-SLM-BLOCK: call <16 x i16> @llvm.genx.oword.ld.v16i16(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <4 x i64> undef)
  %loaded = load <16 x i16>, <16 x i16> addrspace(3)* %pi16, align 16
  ret void
}

define void @replace_load_i32_block_owalign(<16 x i32> addrspace(3)* %pi32) {
  ; CHECK-SLM-BLOCK: call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <8 x i64> undef)
  %loaded = load <16 x i32>, <16 x i32> addrspace(3)* %pi32, align 16
  ret void
}

define void @replace_load_i64_block_owalign(<16 x i64> addrspace(3)* %pi64) {
  ; CHECK-SLM-BLOCK: call <16 x i64> @llvm.genx.oword.ld.v16i64(i32 0, i32 254, i32 %{{[A-Za-z0-9.]+}})
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 0, i32 %{{[0-9a-zA-Z.]+}}, i16 1, i32 0, <16 x i64> undef)
  %loaded = load <16 x i64>, <16 x i64> addrspace(3)* %pi64, align 16
  ret void
}

define void @replace_load_i8_block_1023bytes(<1023 x i8> addrspace(3)* %pi8) {
  ; CHECK: call <1023 x i32> @llvm.genx.gather.scaled.v1023i32.v1023i1.v1023i32

  ; CHECK-SLM-BLOCK: %[[ADDR0:[^ ]+]] = lshr i32 %[[BASE:[^,]+]], 4
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR0]])
  ; CHECK-SLM-BLOCK: %[[ADDR128:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 8
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR128]])
  ; CHECK-SLM-BLOCK: %[[ADDR256:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 16
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR256]])
  ; CHECK-SLM-BLOCK: %[[ADDR384:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 24
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR384]])
  ; CHECK-SLM-BLOCK: %[[ADDR512:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 32
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR512]])
  ; CHECK-SLM-BLOCK: %[[ADDR640:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 40
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR640]])
  ; CHECK-SLM-BLOCK: %[[ADDR768:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 48
  ; CHECK-SLM-BLOCK: call <128 x i8> @llvm.genx.oword.ld.v128i8(i32 0, i32 254, i32 %[[ADDR768]])
  ; CHECK-SLM-BLOCK: %[[ADDR896:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 56
  ; CHECK-SLM-BLOCK: call <64 x i8> @llvm.genx.oword.ld.v64i8(i32 0, i32 254, i32 %[[ADDR896]])
  ; CHECK-SLM-BLOCK: %[[ADDR960:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 60
  ; CHECK-SLM-BLOCK: call <32 x i8> @llvm.genx.oword.ld.v32i8(i32 0, i32 254, i32 %[[ADDR960]])
  ; CHECK-SLM-BLOCK: %[[ADDR992:[A-Za-z0-9.]+]] = add i32 %[[ADDR0]], 62
  ; CHECK-SLM-BLOCK: call <16 x i8> @llvm.genx.oword.ld.v16i8(i32 0, i32 254, i32 %[[ADDR992]])
  ; CHECK-SLM-BLOCK: call <15 x i32> @llvm.genx.gather.scaled.v15i32.v15i1.v15i32(<15 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, i16 0, i32 254, i32 %[[BASE]], <15 x i32> <i32 1008, i32 1009, i32 1010, i32 1011, i32 1012, i32 1013, i32 1014, i32 1015, i32 1016, i32 1017, i32 1018, i32 1019, i32 1020, i32 1021, i32 1022>, <15 x i32> undef)

  ; CHECK-LSC: call <64 x i64> @llvm.vc.internal.lsc.load.slm.v64i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 8, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i64> undef)
  ; CHECK-LSC: [[ADDR512:%[^ ]+]] = add i32 [[ADDR]], 512
  ; CHECK-LSC: call <32 x i64> @llvm.vc.internal.lsc.load.slm.v32i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 7, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR512]], i16 1, i32 0, <32 x i64> undef)
  ; CHECK-LSC: [[ADDR768:%[^ ]+]] = add i32 [[ADDR]], 768
  ; CHECK-LSC: call <16 x i64> @llvm.vc.internal.lsc.load.slm.v16i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 6, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR768]], i16 1, i32 0, <16 x i64> undef)
  ; CHECK-LSC: [[ADDR896:%[^ ]+]] = add i32 [[ADDR]], 896
  ; CHECK-LSC: call <8 x i64> @llvm.vc.internal.lsc.load.slm.v8i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 5, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR896]], i16 1, i32 0, <8 x i64> undef)
  ; CHECK-LSC: [[ADDR960:%[^ ]+]] = add i32 [[ADDR]], 960
  ; CHECK-LSC: call <4 x i64> @llvm.vc.internal.lsc.load.slm.v4i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 4, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR960]], i16 1, i32 0, <4 x i64> undef)
  ; CHECK-LSC: [[ADDR992:%[^ ]+]] = add i32 [[ADDR]], 992
  ; CHECK-LSC: call <3 x i64> @llvm.vc.internal.lsc.load.slm.v3i64.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 4, i8 3, <2 x i8> zeroinitializer, i32 0, i32 [[ADDR992]], i16 1, i32 0, <3 x i64> undef)
  ; CHECK-LSC: call <7 x i32> @llvm.vc.internal.lsc.load.slm.v7i32.v7i1.v2i8.v7i32(<7 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <7 x i32> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <7 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8> addrspace(3)* %pi8
  ret void
}

define void @replace_load_i8_block_1023bytes_dwalign(<1023 x i8> addrspace(3)* %pi8) {
  ; CHECK: call <1023 x i32> @llvm.genx.gather.scaled.v1023i32.v1023i1.v1023i32
  ; CHECK-SLM-BLOCK: call <1023 x i32> @llvm.genx.gather.scaled.v1023i32.v1023i1.v1023i32

  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.slm.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW:%[0-9a-zA-Z.]+]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW256:%[^ ]+]] = add i32 [[ADDRDW]], 256
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.slm.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW256]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW512:%[^ ]+]] = add i32 [[ADDRDW]], 512
  ; CHECK-LSC: call <64 x i32> @llvm.vc.internal.lsc.load.slm.v64i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 8, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW512]], i16 1, i32 0, <64 x i32> undef)
  ; CHECK-LSC: [[ADDRDW768:%[^ ]+]] = add i32 [[ADDRDW]], 768
  ; CHECK-LSC: call <32 x i32> @llvm.vc.internal.lsc.load.slm.v32i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 7, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW768]], i16 1, i32 0, <32 x i32> undef)
  ; CHECK-LSC: [[ADDRDW896:%[^ ]+]] = add i32 [[ADDRDW]], 896
  ; CHECK-LSC: call <16 x i32> @llvm.vc.internal.lsc.load.slm.v16i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 6, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW896]], i16 1, i32 0, <16 x i32> undef)
  ; CHECK-LSC: [[ADDRDW960:%[^ ]+]] = add i32 [[ADDRDW]], 960
  ; CHECK-LSC: call <8 x i32> @llvm.vc.internal.lsc.load.slm.v8i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 5, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW960]], i16 1, i32 0, <8 x i32> undef)
  ; CHECK-LSC: [[ADDRDW992:%[^ ]+]] = add i32 [[ADDRDW]], 992
  ; CHECK-LSC: call <4 x i32> @llvm.vc.internal.lsc.load.slm.v4i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 4, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW992]], i16 1, i32 0, <4 x i32> undef)
  ; CHECK-LSC: [[ADDRDW1008:%[^ ]+]] = add i32 [[ADDRDW]], 1008
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.slm.v3i32.v1i1.v2i8.i32(<1 x i1> <i1 true>, i8 2, i8 3, i8 3, <2 x i8> zeroinitializer, i32 0, i32 [[ADDRDW1008]], i16 1, i32 0, <3 x i32> undef)
  ; CHECK-LSC: call <3 x i32> @llvm.vc.internal.lsc.load.slm.v3i32.v3i1.v2i8.v3i32(<3 x i1> <i1 true, i1 true, i1 true>, i8 2, i8 5, i8 1, <2 x i8> zeroinitializer, i32 0, <3 x i32> %{{[0-9a-zA-Z]+}}, i16 1, i32 0, <3 x i32> undef)
  %loaded = load <1023 x i8>, <1023 x i8> addrspace(3)* %pi8, align 4
  ret void
}
