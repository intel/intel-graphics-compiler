;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Regression tests for the MemOpt fix in SymbolicPointer::checkTerms():
; when OpNum == 3 (both operands of the compared instructions are identical),
; the merge must be REJECTED if the two terms carry different Scale values,
; because the pointer difference is then runtime-dependent.
;
; Two patterns are covered, one per guard:
;
;   Test 1 (first guard):
;     The differing terms' own top-level sub nsw instructions share both
;     operands (%add0 and %k), so the FIRST checkInstructions call sets
;     OpNum = 3.  Scale(chain0)=4, Scale(chain1)=8 → reject.
;
;   Test 2 (second guard):
;     The differing terms' top-level sub nsw instructions share only operand 1
;     (%k); the FIRST checkInstructions call drills into operand 0, which is
;     an add nsw with identical operands (%base, %delta), so the SECOND
;     checkInstructions call sets OpNum = 3.
;     Scale(chain0)=4, Scale(chain1)=8 → reject.
;
; In both cases the loads must remain as two separate float loads rather than
; being coalesced into a <2 x float> load.
;
; RUN: igc_opt --opaque-pointers %s -S -o - --basic-aa -igc-memopt -instcombine | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; ---------------------------------------------------------------------------
; Test 1: first-guard path.
;
; Both differing terms use a top-level sub nsw whose two operands are the
; same IR values (%add0 and %k), so the very first checkInstructions call
; sets OpNum = 3.  The GEP index for chain 1 is multiplied by 2, giving
; Scale=8 vs Scale=4 for chain 0.  The guard must return true (reject).
;
; Chain 0: ptr = src[sub0]       sub0 = add0 - k     scale 4
; Chain 1: ptr = src[sub1 * 2]   sub1 = add0 - k     scale 8
;
; CHECK-LABEL: @check_terms_first_guard_different_scales
; CHECK: load float
; CHECK: load float
; CHECK-NOT: load <2 x float>
define spir_kernel void @check_terms_first_guard_different_scales(
    ptr addrspace(1) %dst,
    ptr addrspace(1) %src,
    i32 %base,
    i32 %delta,
    i32 %k) {
entry:
  %add0  = add nsw i32 %base, %delta

  ; Chain 0: GEP scale = 4 (float)
  %sub0  = sub nsw i32 %add0, %k
  %sub0e = sext i32 %sub0 to i64
  %ptr0  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %sub0e
  %v0    = load float, ptr addrspace(1) %ptr0, align 4

  ; Chain 1: same operands (%add0, %k) as sub0 → first checkInstructions
  ; sets OpNum = 3.  Index doubled → net GEP scale = 8.
  %sub1  = sub nsw i32 %add0, %k
  %sub1e = sext i32 %sub1 to i64
  %sub1s = mul i64 %sub1e, 2
  %ptr1  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %sub1s
  %v1    = load float, ptr addrspace(1) %ptr1, align 4

  %sum   = fadd float %v0, %v1
  store float %sum, ptr addrspace(1) %dst, align 4
  ret void
}

; ---------------------------------------------------------------------------
; Test 2: second-guard path.
;
; The differing terms' top-level sub nsw instructions differ in operand 0
; (%add0 vs %add1), so the first checkInstructions call chooses OpNum = 0
; and drills into that operand.  The nested add nsw instructions (%add0 and
; %add1) share both operands (%base, %delta), so the SECOND
; checkInstructions call sets OpNum = 3.  The GEP index for chain 1 is
; multiplied by 2, giving Scale=8 vs Scale=4 for chain 0.  The guard must
; return true (reject).
;
; Chain 0: ptr = src[sub0]       sub0 = add0 - k,  add0 = base + delta   scale 4
; Chain 1: ptr = src[sub1 * 2]   sub1 = add1 - k,  add1 = base + delta   scale 8
;
; CHECK-LABEL: @check_terms_second_guard_different_scales
; CHECK: load float
; CHECK: load float
; CHECK-NOT: load <2 x float>
define spir_kernel void @check_terms_second_guard_different_scales(
    ptr addrspace(1) %dst,
    ptr addrspace(1) %src,
    i32 %base,
    i32 %delta,
    i32 %k) {
entry:
  ; Chain 0: GEP scale = 4 (float)
  %add0  = add nsw i32 %base, %delta
  %sub0  = sub nsw i32 %add0, %k
  %sub0e = sext i32 %sub0 to i64
  %ptr0  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %sub0e
  %v0    = load float, ptr addrspace(1) %ptr0, align 4

  ; Chain 1: add1 has same operands as add0 → second checkInstructions
  ; sets OpNum = 3.  Index doubled → net GEP scale = 8.
  %add1  = add nsw i32 %base, %delta
  %sub1  = sub nsw i32 %add1, %k
  %sub1e = sext i32 %sub1 to i64
  %sub1s = mul i64 %sub1e, 2
  %ptr1  = getelementptr inbounds float, ptr addrspace(1) %src, i64 %sub1s
  %v1    = load float, ptr addrspace(1) %ptr1, align 4

  %sum   = fadd float %v0, %v1
  store float %sum, ptr addrspace(1) %dst, align 4
  ret void
}
