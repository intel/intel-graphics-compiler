;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -CMLowerVLoadVStore -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

declare <16 x float> @llvm.genx.vload.v16f32.p0v16f32(<16 x float>*)
declare void @llvm.genx.vstore.v16f32.p0v16f32(<16 x float>, <16 x float>*)

define void @test(<16 x float>* %addr) {
; CHECK: [[VAL:[^ ]+]] = load <16 x float>, <16 x float>* %addr, align 4
  %val = call <16 x float> @llvm.genx.vload.v16f32.p0v16f32(<16 x float>* %addr)
; CHECK-NEXT: store <16 x float> [[VAL]], <16 x float>* %addr, align 4
  call void @llvm.genx.vstore.v16f32.p0v16f32(<16 x float> %val, <16 x float>* %addr)
  ret void
}
