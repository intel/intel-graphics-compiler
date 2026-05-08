;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-gen-specific-pattern -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; The (icmp eq X, -1) -> (zext to i32) -> (add X, 1) folding into uaddc is only
; valid when the matched `add X, 1` dominates the `zext`. The rewrite hoists
; the carry-flag definition to the add's location and RAUWs the zext, so a use
; that crossed the conditional branch into the false successor (where PIX shader
; instrumentation typically materializes the +1 offset for the next-element load)
; would land in a block that the add does not dominate.
;
; X is sourced from a load in every test so the m_Instruction matcher inside
; addcPattern1 reaches the dominance check; using a function argument here
; would short-circuit the match and make the test pass for the wrong reason.

; Negative case 1: the add lives in the false successor of the icmp's branch.
; The zext is consumed in the predecessor block (typical PIX dump pattern), so
; the rewrite must not fire. Two things must hold: no uaddc is emitted, AND the
; predecessor-block dump store of the converted compare must survive (this is
; what got silently lost in the original miscompile and produced the
; "icmp eq X, -1 stored as 0 even though X = -1" PIX symptom).

define spir_kernel void @cross_block_negative(ptr addrspace(1) %src, ptr addrspace(1) %dump) {
entry:
  %x = load i32, ptr addrspace(1) %src, align 4
  %cmp = icmp eq i32 %x, -1
  %conv = zext i1 %cmp to i32
  store i32 %conv, ptr addrspace(1) %dump, align 4
  br i1 %cmp, label %skip, label %use_add

use_add:
  %add = add i32 %x, 1
  %gep = getelementptr i32, ptr addrspace(1) %dump, i32 %add
  store i32 %add, ptr addrspace(1) %gep, align 4
  br label %skip

skip:
  ret void
}

; CHECK-LABEL: @cross_block_negative
; CHECK-NOT: call <2 x i32> @llvm.genx.GenISA.uaddc
; CHECK: store i32 %{{[a-zA-Z0-9_.]+}}, ptr addrspace(1) %dump
; CHECK: ret void

; Negative case 2: same block, but the add is textually after the zext.
; The current rewrite would still anchor at the add, so the carry replacing
; the zext would not dominate users above the add.

define spir_kernel void @same_block_add_after_negative(ptr addrspace(1) %src, ptr addrspace(1) %dump) {
entry:
  %x = load i32, ptr addrspace(1) %src, align 4
  %cmp = icmp eq i32 %x, -1
  %conv = zext i1 %cmp to i32
  store i32 %conv, ptr addrspace(1) %dump, align 4
  %add = add i32 %x, 1
  %gep = getelementptr i32, ptr addrspace(1) %dump, i32 %add
  store i32 %add, ptr addrspace(1) %gep, align 4
  ret void
}

; CHECK-LABEL: @same_block_add_after_negative
; CHECK-NOT: call <2 x i32> @llvm.genx.GenISA.uaddc
; CHECK: store i32 %{{[a-zA-Z0-9_.]+}}, ptr addrspace(1) %dump
; CHECK: ret void

; Positive control: same block, add precedes the zext - the rewrite is valid
; and must still fire (the original optimization is preserved).

define spir_kernel void @positive_control(ptr addrspace(1) %src, ptr addrspace(1) %dump) {
entry:
  %x = load i32, ptr addrspace(1) %src, align 4
  %add = add i32 %x, 1
  %cmp = icmp eq i32 %x, -1
  %conv = zext i1 %cmp to i32
  store i32 %conv, ptr addrspace(1) %dump, align 4
  %gep = getelementptr i32, ptr addrspace(1) %dump, i32 %add
  store i32 %add, ptr addrspace(1) %gep, align 4
  ret void
}

; CHECK-LABEL: @positive_control
; CHECK: call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %x, i32 1)
