;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-OPT %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-OPT %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-OPT %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLPG -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-NO-OPT %s
; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S < %s | FileCheck --check-prefix=CHECK-OPT %s

; CHECK-NO-OPT-LABEL: test
; CHECK-NO-OPT: [[ADD:[^ ]+]] = fadd <16 x double> %a, %b
; CHECK-NO-OPT: [[RES:[^ ]+]] = fdiv arcp <16 x double> %a, [[ADD]]
; CHECK-NO-OPT: ret <16 x double> [[RES]]

; CHECK-OPT-LABEL: test
; CHECK-OPT: [[ADD:[^ ]+]] = fadd <16 x double> %a, %b
; CHECK-OPT: [[ADD_INV:[^ ]+]] = call <16 x double> @llvm.genx.inv.v16f64(<16 x double> [[ADD]])
; CHECK-OPT: [[RES:[^ ]+]] = fmul arcp <16 x double> %a, [[ADD_INV]]
; CHECK-OPT: ret <16 x double> [[RES]]

define <16 x double> @test(<16 x double> %a, <16 x double> %b) {
  %add = fadd <16 x double> %a, %b
  %res = fdiv arcp <16 x double> %a, %add
  ret <16 x double> %res
}
