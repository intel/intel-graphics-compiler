;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %not_for_vc_diag% opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s 2>&1 | FileCheck %s

define void @test(<15 x i32> %0, <15 x i32> %1, <15 x i32> %2) {
entry:
; CHECK: GenXLegalization failed for: < %data = call <15 x i32> @llvm.genx.lsc.xatomic.bti.v15i32.v15i1.v15i32(<15 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <15 x i32> %0, <15 x i32> %1, <15 x i32> %2, i32 0, <15 x i32> undef){{.*}}>: LSC atomic instruction execution width must be a power of 2
  %data = call <15 x i32> @llvm.genx.lsc.xatomic.bti.v15i32.v15i1.v15i32(<15 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 18, i8 0, i8 0, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <15 x i32> %0, <15 x i32> %1, <15 x i32> %2, i32 0, <15 x i32> undef)
  ret void
}

declare <15 x i32> @llvm.genx.lsc.xatomic.bti.v15i32.v15i1.v15i32(<15 x i1>, i8, i8, i8, i16, i32, i8, i8, i8, i8, <15 x i32>, <15 x i32>, <15 x i32>, i32, <15 x i32>)
