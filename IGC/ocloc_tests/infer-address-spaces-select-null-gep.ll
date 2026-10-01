;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; InferAddressSpaces must not infer a private address space for a select
; between a private pointer and a GEP whose only pointer operand is a generic
; null. Leaving that GEP uninferred made the rewrite drop the select's operand,
; and the following BreakConstantExpr crashed on it.
; https://github.com/llvm/llvm-project/issues/171890
; Fixed upstream by https://github.com/llvm/llvm-project/pull/172143

; REQUIRES: regkeys, bmg-supported

; RUN: llvm-as %OPAQUE_PTR_FLAG% < %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s

; CHECK: Build succeeded

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%mat = type { [3 x %vec] }
%vec = type { float, float, float }

define spir_kernel void @test(i1 %c) {
entry:
  %a = alloca %mat, align 4
  %a.gen = addrspacecast ptr %a to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) null, ptr addrspace(4) %a.gen
  %gep = getelementptr i8, ptr addrspace(4) %sel, i64 12
  store float 0.000000e+00, ptr addrspace(4) %gep, align 4
  ret void
}
