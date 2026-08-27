;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=XeHPC \
; RUN: -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; A bool-vector select whose condition is an EM value (extractvalue of a
; simdcf.goto/join) must not feed the raw EM into the and/xor of the bit
; twiddling: the late SIMD-CF conformance pass only accepts an EM used by
; goto/join/phi/rdpredregion/predicated-select. Route it through
; genx.simdcf.get.em so the boolean algebra uses a plain predicate copy.

declare { <16 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v16i1.v16i1(<16 x i1>, <16 x i1>, <16 x i1>)

; Condition is EM, "true" value is not a cmp: lower via get.em + and/xor.
; CHECK-LABEL: @test_em_select_general(
; CHECK: %[[EM:[^ ]+]] = extractvalue { <16 x i1>, <16 x i1>, i1 } %{{[^,]+}}, 0
; CHECK: %[[GETEM:[^ ]+]] = call <16 x i1> @llvm.genx.simdcf.get.em.v16i1(<16 x i1> %[[EM]])
; CHECK: %[[AND1:[^ ]+]] = and <16 x i1> %[[GETEM]], %a
; CHECK: %[[XOR:[^ ]+]] = xor <16 x i1> %[[GETEM]], {{.*}}
; CHECK: %[[AND2:[^ ]+]] = and <16 x i1> %b, %[[XOR]]
; CHECK: %[[OR:[^ ]+]] = or <16 x i1> %[[AND1]], %[[AND2]]
; CHECK: ret <16 x i1> %[[OR]]
define <16 x i1> @test_em_select_general(<16 x i1> %em.in, <16 x i1> %cond, <16 x i1> %a, <16 x i1> %b) {
  %g = call { <16 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v16i1.v16i1(<16 x i1> %em.in, <16 x i1> zeroinitializer, <16 x i1> %cond)
  %em = extractvalue { <16 x i1>, <16 x i1>, i1 } %g, 0
  %sel = select <16 x i1> %em, <16 x i1> %a, <16 x i1> %b
  ret <16 x i1> %sel
}

; Condition is EM, "true" value is a cmp: keep the wrpredpredregion path (a
; masked cmp), which is already conformant. No get.em, no raw and/xor.
; CHECK-LABEL: @test_em_select_cmp(
; CHECK: %[[EM:[^ ]+]] = extractvalue { <16 x i1>, <16 x i1>, i1 } %{{[^,]+}}, 0
; CHECK: %[[CMP:[^ ]+]] = icmp ult <16 x i32> %x, %y
; CHECK: %[[WR:[^ ]+]] = call <16 x i1> @llvm.genx.wrpredpredregion.v16i1.v16i1(<16 x i1> %b, <16 x i1> %[[CMP]], i32 0, <16 x i1> %[[EM]])
; CHECK-NOT: @llvm.genx.simdcf.get.em
; CHECK-NOT: and <16 x i1>
; CHECK: ret <16 x i1> %[[WR]]
define <16 x i1> @test_em_select_cmp(<16 x i1> %em.in, <16 x i1> %cond, <16 x i32> %x, <16 x i32> %y, <16 x i1> %b) {
  %g = call { <16 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto.v16i1.v16i1(<16 x i1> %em.in, <16 x i1> zeroinitializer, <16 x i1> %cond)
  %em = extractvalue { <16 x i1>, <16 x i1>, i1 } %g, 0
  %cmp = icmp ult <16 x i32> %x, %y
  %sel = select <16 x i1> %em, <16 x i1> %cmp, <16 x i1> %b
  ret <16 x i1> %sel
}

; Condition is a plain (non-EM) predicate: unchanged bit twiddling, no get.em.
; CHECK-LABEL: @test_nonem_select(
; CHECK-NOT: @llvm.genx.simdcf.get.em
; CHECK: %[[AND1:[^ ]+]] = and <16 x i1> %c, %a
; CHECK: %[[XOR:[^ ]+]] = xor <16 x i1> %c, {{.*}}
; CHECK: %[[AND2:[^ ]+]] = and <16 x i1> %b, %[[XOR]]
; CHECK: %[[OR:[^ ]+]] = or <16 x i1> %[[AND1]], %[[AND2]]
; CHECK: ret <16 x i1> %[[OR]]
define <16 x i1> @test_nonem_select(<16 x i1> %c, <16 x i1> %a, <16 x i1> %b) {
  %sel = select <16 x i1> %c, <16 x i1> %a, <16 x i1> %b
  ret <16 x i1> %sel
}
