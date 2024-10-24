;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

%struct.st = type { i32, <4 x float>, [2 x i64] }
define void @test_lowergep(<2 x %struct.st*> %arg) {
  %1 = getelementptr %struct.st, <2 x %struct.st*> %arg, <2 x i32> <i32 1, i32 1>, <2 x i32> <i32 1, i32 1>
; CHECK-TYPED-PTRS: [[V1:%.*]] = ptrtoint <2 x %struct.st*> %arg to <2 x i64>
; CHECK-OPAQUE-PTRS: [[V1:%.*]] = ptrtoint <2 x ptr> %arg to <2 x i64>
; CHECK-NEXT: [[V2:%.*]] = add <2 x i64> [[V1]], <i64 48, i64 48>
; CHECK-NEXT: [[V3:%.*]] = add <2 x i64> [[V2]], <i64 16, i64 16>
; CHECK-TYPED-PTRS-NEXT: [[V4:%.*]] = inttoptr <2 x i64> [[V3]] to <2 x <4 x float>*>
; CHECK-OPAQUE-PTRS-NEXT: [[V4:%.*]] = inttoptr <2 x i64> [[V3]] to <2 x ptr>
  ret void
}
