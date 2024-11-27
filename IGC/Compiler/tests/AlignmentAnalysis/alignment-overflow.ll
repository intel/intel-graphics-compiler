;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; This test checks if we don't crash / set incorrect alignment in shl and mul cases.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-fix-alignment -S %s -o %t
; RUN: FileCheck %s --input-file=%t
target datalayout = "e-p:32:32-p1:64:64-p2:64:64-p3:32:32-p4:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @shl_overflow
define void @shl_overflow() {
entry:
  %alloc = alloca <4 x i8>
  %int = ptrtoint ptr %alloc to i32
  %shl1 = shl i32 %int, 63
  %ptr1 = inttoptr i32 %shl1 to ptr
; CHECK: load
; CHECK: align 4294967296
  %val1 = load i32, ptr %ptr1
  ret void
}

; CHECK-LABEL: @mul_overflow
define void @mul_overflow(ptr align 1 %p) {
entry:
  %int = ptrtoint ptr %p to i64
  %mul1 = mul i64 %int, u0x1000000000000000
  %mul2 = mul i64 %mul1, u0x20
  %ptr1 = inttoptr i64 %mul2 to ptr
; CHECK: load
; CHECK: align 4294967296
  %val1 = load i32, ptr %ptr1
  ret void
}


attributes #0 = { alwaysinline nounwind }