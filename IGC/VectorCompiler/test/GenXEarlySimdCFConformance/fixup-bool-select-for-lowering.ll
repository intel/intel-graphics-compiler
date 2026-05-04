;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXEarlySimdCFConformance -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPG -S < %s | FileCheck %s
;
; LLVM-17 InstCombine folds icmp(select(pred, 1, 0), 0) back to pred, which
; may be xor(cmp, all_true) — not a CmpInst. GenXLowering needs a CmpInst
; to create wrpredpredregion. The conformance pass must replace xor(cmp, true)
; with the inverse CmpInst.
;
; CHECK-LABEL: @test_xor_to_inv_cmp
; CHECK: [[CMP:%[a-z0-9.]+]] = icmp ne <32 x i8>
; CHECK: select <32 x i1> %join.extractem, <32 x i1> [[CMP]], <32 x i1> zeroinitializer

define spir_kernel void @test_xor_to_inv_cmp(<32 x i8> %arg) {
entry:
  br label %body

body:
  %join = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, <32 x i1> zeroinitializer)
  %join.extractem = extractvalue { <32 x i1>, i1 } %join, 0
  %cmp.eq = icmp eq <32 x i8> %arg, zeroinitializer
  ; LLVM-17 InstCombine produces this xor instead of icmp ne:
  %inv = xor <32 x i1> %cmp.eq, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  %sel = select <32 x i1> %join.extractem, <32 x i1> %inv, <32 x i1> zeroinitializer
  ret void
}

declare { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>)
