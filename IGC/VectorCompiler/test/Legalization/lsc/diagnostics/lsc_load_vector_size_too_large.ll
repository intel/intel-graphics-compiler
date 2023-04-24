;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %not_for_vc_diag% opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s  2>&1 | FileCheck %s

define void @test(<16 x i32> %0, i32 %1) {
entry:
; CHECK: GenX execution width and GRF crossing legalization failed for: < %data = call <256 x i32> @llvm.genx.lsc.load.bti.v256i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 1, i8 0, <16 x i32> %0, i32 %1){{.*}}>: Non-transposed LSC instruction vector size is too large
  %data = call <256 x i32> @llvm.genx.lsc.load.bti.v256i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 3, i8 6, i8 1, i8 0, <16 x i32> %0, i32 %1)
  ret void
}

declare <256 x i32> @llvm.genx.lsc.load.bti.v256i32.v16i1.v16i32(<16 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <16 x i32>, i32)
