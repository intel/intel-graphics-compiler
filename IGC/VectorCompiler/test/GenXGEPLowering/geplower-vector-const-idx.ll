;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXGEPLowering -march=genx64 -mcpu=Xe2 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; A vector-of-pointers base gives a vector pointer-math value, so a constant
; index must be lowered to a matching vector offset (not a scalar), otherwise
; the add is built with operands of differing type.

; CHECK-LABEL: @const_idx_vector_ptr
define <8 x i32*> @const_idx_vector_ptr(<8 x i32*> %base) {
  %gep = getelementptr i32, <8 x i32*> %base, i64 2
; CHECK-TYPED-PTRS: [[V1:%.*]] = ptrtoint <8 x i32*> %base to <8 x i64>
; CHECK-OPAQUE-PTRS: [[V1:%.*]] = ptrtoint <8 x ptr> %base to <8 x i64>
; CHECK-NEXT: [[V2:%.*]] = add <8 x i64> [[V1]], {{(splat \(i64 8\)|<i64 8(, i64 8)*>)}}
; CHECK-TYPED-PTRS-NEXT: [[V3:%.*]] = inttoptr <8 x i64> [[V2]] to <8 x i32*>
; CHECK-OPAQUE-PTRS-NEXT: [[V3:%.*]] = inttoptr <8 x i64> [[V2]] to <8 x ptr>
  ret <8 x i32*> %gep
}
