;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -PruneUnusedArguments -S < %s | FileCheck %s
; ------------------------------------------------
; PruneUnusedArguments
; ------------------------------------------------
;
; Verify that the pass does not replace function pointers with undef when the
; function is used as an argument in a call (not as the callee). This is a
; regression test for a bug where PruneUnusedArguments would iterate over all
; users of a function and, for any CallInst user, replace operands at the
; unused-argument indices with undef — even when the function was passed as an
; argument rather than being the callee. With opaque pointers, function pointers
; can be passed directly without a bitcast, exposing this bug.

; A helper function with one used and one unused argument.
define spir_func void @callee(ptr %used, i32 %unused) {
; CHECK-LABEL: define spir_func void @callee(
  %v = load i32, ptr %used
  %sum = add i32 %v, 1
  store i32 %sum, ptr %used
  ret void
}

; An external intrinsic that takes a function pointer as its first argument.
declare ptr addrspace(1) @llvm.genx.GenISA.GetShaderPtr(ptr, i32)

define spir_kernel void @test(ptr %src) {
; CHECK-LABEL: define spir_kernel void @test(

  ; Direct call to @callee — the unused arg should be replaced with undef.
; CHECK: call spir_func void @callee(ptr %src, i32 undef)
  call spir_func void @callee(ptr %src, i32 42)

  ; @callee used as an argument (not callee) — must NOT be replaced with undef.
; CHECK: call ptr addrspace(1) @llvm.genx.GenISA.GetShaderPtr(ptr @callee, i32 0)
  %record = call ptr addrspace(1) @llvm.genx.GenISA.GetShaderPtr(ptr @callee, i32 0)

  ret void
}
