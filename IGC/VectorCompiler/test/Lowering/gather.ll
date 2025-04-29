;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPG \
; RUN: -mtriple=spir64-unknown-unknown  -S < %s | FileCheck %s

declare <32 x float> @llvm.genx.gather.masked.scaled2.v32f32.v32i32.v32i1(i32, i16, i32, i32, <32 x i32>, <32 x i1>)
declare <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float>, <16 x float>, i32, i32, i32, i16, i32, i1)

; CHECK-LABEL: kernel
define <32 x float> @kernel(<16 x float> %arg) {
; CHECK: [[ADDR_SPLIT1:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60, i32 64, i32 68, i32 72, i32 76, i32 80, i32 84, i32 88, i32 92, i32 96, i32 100, i32 104, i32 108, i32 112, i32 116, i32 120, i32 124>, i32 0, i32 16, i32 1, i16 0, i32 undef)
; CHECK: [[GATHER_SPLIT1:%[^ ]+]] = call <16 x float> @llvm.genx.gather.masked.scaled2.v16f32.v16i32.v16i1(i32 0, i16 0, i32 0, i32 0, <16 x i32> [[ADDR_SPLIT1]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK: [[JOIN1:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> undef, <16 x float> [[GATHER_SPLIT1]], i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
; CHECK: [[ADDR_SPLIT2:%[^ ]+]] = call <16 x i32> @llvm.genx.rdregioni.v16i32.v32i32.i16(<32 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60, i32 64, i32 68, i32 72, i32 76, i32 80, i32 84, i32 88, i32 92, i32 96, i32 100, i32 104, i32 108, i32 112, i32 116, i32 120, i32 124>, i32 0, i32 16, i32 1, i16 64, i32 undef)
; CHECK: [[GATHER_SPLIT2:%[^ ]+]] = call <16 x float> @llvm.genx.gather.masked.scaled2.v16f32.v16i32.v16i1(i32 0, i16 0, i32 0, i32 0, <16 x i32> [[ADDR_SPLIT2]], <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
; CHECK: [[JOIN2:%[^ ]+]] = call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> [[JOIN1]], <16 x float> [[GATHER_SPLIT2]], i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
; CHECK: %ret = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> [[JOIN2]], <16 x float> %arg, i32 0, i32 0, i32 1, i16 0, i32 0, i1 false)
  %gather = tail call <32 x float> @llvm.genx.gather.masked.scaled2.v32f32.v32i32.v32i1(i32 0, i16 0, i32 0, i32 0, <32 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28, i32 32, i32 36, i32 40, i32 44, i32 48, i32 52, i32 56, i32 60, i32 64, i32 68, i32 72, i32 76, i32 80, i32 84, i32 88, i32 92, i32 96, i32 100, i32 104, i32 108, i32 112, i32 116, i32 120, i32 124>, <32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  %ret = tail call <32 x float> @llvm.genx.wrregionf.v32f32.v16f32.i16.i1(<32 x float> %gather, <16 x float> %arg, i32 0, i32 0, i32 1, i16 0, i32 0, i1 false)
  ret <32 x float> %ret
}
