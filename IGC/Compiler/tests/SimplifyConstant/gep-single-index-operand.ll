;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -SimplifyConstant -S < %s 2>&1 | FileCheck %s

; This test ensures SimplifyConstant pass supports flat GEPs with a single index operand.

@g = dso_local unnamed_addr addrspace(2) constant [2 x i32] [i32 1065353216, i32 -1082130432], align 4

define spir_kernel void @test(i64 %index, ptr %output) {
; CHECK-LABEL: @test(
; CHECK:    [[TRUNC:%.*]] = trunc i64 [[INDEX:%.*]] to i1
; CHECK:    [[SELECT_I32:%.*]] = select i1 [[TRUNC]], i32 -1082130432, i32 1065353216
; CHECK:    [[BITCAST:%.*]] = bitcast i32 [[SELECT_I32]] to float
; CHECK:    store float [[BITCAST]], ptr [[OUTPUT:%.*]], align 4
; CHECK:    ret void
  %a = getelementptr inbounds float, ptr addrspace(2) @g, i64 %index
  %b = load float, ptr addrspace(2) %a, align 4
  store float %b, ptr %output, align 4
  ret void
}
