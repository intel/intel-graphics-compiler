;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mcpu=XeLP -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; CHECK-LABEL: @match
define <2 x half> @match(<2 x half> %src) {
  ; CHECK: [[MIN:%[^ ]+]] = call <2 x half> @llvm.minnum.v2f16(<2 x half> <half 0xH3800, half 0xH3800>, <2 x half> %src)
  ; CHECK: ret <2 x half> [[MIN]]
  %1 = fcmp oge <2 x half> %src, <half 0xH3800, half 0xH3800>
  %2 = select <2 x i1> %1, <2 x half> <half 0xH3800, half 0xH3800>, <2 x half> %src
  ret <2 x half> %2
}

; CHECK-LABEL: @not_match
define <2 x half> @not_match(<2 x half> %src) {
  ; CHECK: [[CMP:%[^ ]+]] = fcmp oge <2 x half> %src, <half 0xH3800, half 0xH3800>
  ; CHECK: [[SEL:%[^ ]+]] = select <2 x i1> [[CMP]], <2 x half> <half 0xH3C00, half 0xH3C00>, <2 x half> %src
  ; CHECK: ret <2 x half> [[SEL]]
  %1 = fcmp oge <2 x half> %src, <half 0xH3800, half 0xH3800>
  %2 = select <2 x i1> %1, <2 x half> <half 0xH3C00, half 0xH3C00>, <2 x half> %src
  ret <2 x half> %2
}
