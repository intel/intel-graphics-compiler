;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %not_for_vc_diag% opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s 2>&1 | FileCheck %s

define void @test(i32 %0, i32 %1) {
entry:
; CHECK: GenXLegalization failed for: < %data = call <64 x i64> @llvm.genx.lsc.load.bti.v64i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 8, i8 2, i8 0, i32 %0, i32 %1){{.*}}>: Transposed LSC instruction vector size is too large
  %data = call <64 x i64> @llvm.genx.lsc.load.bti.v64i64.i1.i32(i1 true, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 8, i8 2, i8 0, i32 %0, i32 %1)
  ret void
}

declare <64 x i64> @llvm.genx.lsc.load.bti.v64i64.i1.i32(i1, i8, i8, i8, i16, i32, i8, i8, i8, i8, i32, i32)

