;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLegalization -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S %s | FileCheck %s

define void @test() {
entry:
; CHECK-LABEL: test

; CHECK:      call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
; CHECK-NEXT: ret void

  call void @llvm.genx.lsc.fence.i1(i1 true, i8 0, i8 0, i8 0)
  ret void
}

declare void @llvm.genx.lsc.fence.i1(i1, i8, i8, i8)
