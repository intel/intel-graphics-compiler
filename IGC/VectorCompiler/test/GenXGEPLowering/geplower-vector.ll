;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

define void @test8.v16i64(<16 x i64*> %arg, <16 x i64> %offset) {
  %arrayidx = getelementptr i64, <16 x i64*> %arg, <16 x i64> %offset
; CHECK-TYPED-PTRS: [[V1:%.*]] = ptrtoint <16 x i64*> %arg to <16 x i64>
; CHECK-OPAQUE-PTRS: [[V1:%.*]] = ptrtoint <16 x ptr> %arg to <16 x i64>
; CHECK-NEXT: [[V2:%.*]] = shl <16 x i64> %offset, <i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3, i64 3>
; CHECK-NEXT: [[V3:%.*]] = add <16 x i64> [[V1]], [[V2]]
; CHECK-TYPED-PTRS-NEXT: [[V4:%.*]] = inttoptr <16 x i64> [[V3]] to <16 x i64*>
; CHECK-OPAQUE-PTRS-NEXT: [[V4:%.*]] = inttoptr <16 x i64> [[V3]] to <16 x ptr>
  ret void
}
