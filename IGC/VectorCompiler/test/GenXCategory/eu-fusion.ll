;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s --check-prefix=CHECK-Gen9
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen11 -S < %s | FileCheck %s --check-prefix=CHECK-Gen11
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLP -S < %s | FileCheck %s --check-prefix=CHECK-XeLP
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHP -S < %s | FileCheck %s --check-prefix=CHECK-XeHP
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -S < %s | FileCheck %s --check-prefix=CHECK-XeHPG
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLPG -S < %s | FileCheck %s --check-prefix=CHECK-XeLPG
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeLPGPlus -S < %s | FileCheck %s --check-prefix=CHECK-XeLPG
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefix=CHECK-XeHPC
; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe2 -S < %s | FileCheck %s --check-prefix=CHECK-Xe2


; CHECK-LABEL: constant
; CHECK-Gen9: call <32 x i16> @llvm.genx.rdregioni.v32i16.v1i16.i16
; CHECK-Gen11: call <32 x i16> @llvm.genx.rdregioni.v32i16.v1i16.i16
; CHECK-XeHPC: call <32 x i16> @llvm.genx.rdregioni.v32i16.v1i16.i16
; CHECK-Xe2: call <32 x i16> @llvm.genx.rdregioni.v32i16.v1i16.i16

; CHECK-XeHP: call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16
; CHECK-XeHPG: call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16
; CHECK-XeLP: call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16
; CHECK-XeLPG: call <16 x i16> @llvm.genx.rdregioni.v16i16.v1i16.i16

define <32 x i16> @test(i1 %cond, <32 x i16> %arg) #1 {
entry:
  br i1 %cond, label %constant, label %end

constant:
  br label %end

end:
  %ret = phi <32 x i16> [ zeroinitializer, %constant ], [ %arg,  %entry ]
  ret <32 x i16> %ret
}

attributes #1 = { "CMStackCall" }
