;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Regression test for a crash in SymbolicPointer::checkTerms().
;
; When both operands of the top-level binary instruction are identical across
; the two pointer chains, checkInstructions() sets the sentinel OpNum = 3.
; Without the early-exit guard added immediately after the first
; checkInstructions() call, the code would proceed to call getOperand(3) on a
; two-operand BinaryOperator, triggering an LLVM assertion / abort.
;
; Pattern exercised (both top-level sub nsw have the same two operands):
;
;   %add0  = add nsw i32 %base, %delta
;   %idx0  = sub nsw i32 %add0, %k       <- chain 0, top-level operands: (%add0, %k)
;
;   %add1  = add nsw i32 %base, %delta
;   %idx1r = sub nsw i32 %add1, %k       <- chain 1, top-level operands: (%add1, %k)
;   %idx1  = add nsw i32 %idx1r, 1       <- one element further
;
; RUN: igc_opt --opaque-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: @check_terms_same_operands
; Verify that the pass completes without crashing and the function is present.
; CHECK: ret void
define spir_kernel void @check_terms_same_operands(
    ptr addrspace(1) %dst,
    ptr addrspace(1) %src,
    i32 %base,
    i32 %delta,
    i32 %k) {
entry:
  ; Chain 0: index = (base + delta) - k
  %add0  = add nsw i32 %base, %delta
  %idx0  = sub nsw i32 %add0, %k
  %idx0e = sext i32 %idx0 to i64
  %ptr0  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx0e
  %v0    = load float, ptr addrspace(1) %ptr0, align 4

  ; Chain 1: index = (base + delta) - k + 1
  ; The top-level sub nsw has the same two operands as chain 0's sub nsw,
  ; which is the exact condition that triggers the OpNum = 3 sentinel.
  %add1  = add nsw i32 %base, %delta
  %idx1r = sub nsw i32 %add1, %k
  %idx1  = add nsw i32 %idx1r, 1
  %idx1e = sext i32 %idx1 to i64
  %ptr1  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %idx1e
  %v1    = load float, ptr addrspace(1) %ptr1, align 4

  ; Use both values to prevent DCE.
  %sum   = fadd float %v0, %v1
  store float %sum, ptr addrspace(1) %dst, align 4
  ret void
}
