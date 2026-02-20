;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-custom-unsafe-opt-pass -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-custom-unsafe-opt-pass -S %s -regkey AllowNonMulMemOpMadChainReassoc=1 | FileCheck %s --check-prefix=CHECK-MEMOP

; Test reassociateMulAdd optimization that reassociates a non-multiply operand
; from the end of a MAD chain to pair with the first multiply.
;
; Before:
;   t0 = A * B
;   t1 = C * D
;   sum = t0 + t1 + Candidate  (where Candidate is not from a multiply)
;
; After:
;   t0 = A * B
;   t0n = t0 + Candidate   // combines to MAD
;   sum = t0n + t1 // Chain of MADs


; Basic test: depth 1 chain (A*B + C*D + Candidate)
; Candidate should be reassociated to pair with A*B
define float @test_basic_depth1(float %a, float %b, float %c, float %d, float %Candidate) #0 {
entry:
  %mul0 = fmul fast float %a, %b
  %mul1 = fmul fast float %c, %d
  %add0 = fadd fast float %mul0, %mul1
  %add1 = fadd fast float %add0, %Candidate
  ret float %add1
}

; CHECK-LABEL: define float @test_basic_depth1
; CHECK: %mul0 = fmul fast float %a, %b
; CHECK: %mul1 = fmul fast float %c, %d
; CHECK: %[[ADD1:[a-z0-9]+]] = fadd fast float %mul0, %Candidate
; CHECK: %[[ADD0:[a-z0-9]+]] = fadd fast float %[[ADD1]], %mul1
; CHECK: ret float %[[ADD0]]


; Test case with mixed fadd/fsub operations in the chain
; t0 = A * B
; t1 = C * D
; t2 = E * F
; r0 = t0 + t1       ; fadd at root
; r1 = r0 - t2       ; fsub intermediate (tests sign propagation)
; r2 = r1 + Candidate ; Candidate is not a multiply
;
; Should become:
; newT0 = t0 + Candidate     (MAD)
; r0' = newT0 + t1   (MAD)
; r1' = r0' - t2     (preserve fsub)
; r2 replaced with r1'

define float @test_mixed_addsub(float %A, float %B, float %C, float %D, float %E, float %F, float %Candidate) {
; CHECK-LABEL: @test_mixed_addsub
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[T2:%.*]] = fmul fast float %E, %F
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], %Candidate
; CHECK: [[R0:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: [[R1:%.*]] = fsub fast float [[R0]], [[T2]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %t2 = fmul fast float %E, %F
  %r0 = fadd fast float %t0, %t1
  %r1 = fsub fast float %r0, %t2
  %r2 = fadd fast float %r1, %Candidate
  ret float %r2
}

; Test with fsub at the end: (A*B + C*D) - Candidate
define float @test_fsub_at_end(float %a, float %b, float %c, float %d, float %Candidate) #0 {
entry:
  %mul0 = fmul fast float %a, %b
  %mul1 = fmul fast float %c, %d
  %add0 = fadd fast float %mul0, %mul1
  %sub0 = fsub fast float %add0, %Candidate
  ret float %sub0
}

; CHECK-LABEL: define float @test_fsub_at_end
; CHECK: %mul0 = fmul fast float %a, %b
; CHECK: %mul1 = fmul fast float %c, %d
; CHECK: %[[SUB0:[a-z0-9]+]] = fsub fast float %mul0, %Candidate
; CHECK: %[[ADD0:[a-z0-9]+]] = fadd fast float %[[SUB0]], %mul1
; CHECK: ret float %[[ADD0]]


; Test where Candidate comes from a load - should NOT be reassociated by default
; but SHOULD be reassociated when AllowNonMulMemOpMadChainReassoc is enabled
define float @test_load_operand(float %a, float %b, float %c, float %d, ptr %ptr) #0 {
entry:
  %Candidate = load float, ptr %ptr, align 4
  %mul0 = fmul fast float %a, %b
  %mul1 = fmul fast float %c, %d
  %add0 = fadd fast float %mul0, %mul1
  %add1 = fadd fast float %add0, %Candidate
  ret float %add1
}

; CHECK-LABEL: define float @test_load_operand
; With the regkey disabled, load operand should NOT be reassociated (original order preserved)
; CHECK: %Candidate = load float, ptr %ptr, align 4
; CHECK: %mul0 = fmul fast float %a, %b
; CHECK: %mul1 = fmul fast float %c, %d
; CHECK: %add0 = fadd fast float %mul0, %mul1
; CHECK: %add1 = fadd fast float %add0, %Candidate
; CHECK: ret float %add1

; CHECK-MEMOP-LABEL: define float @test_load_operand
; With the regkey enabled, load operand should be reassociated
; CHECK-MEMOP: %Candidate = load float, ptr %ptr, align 4
; CHECK-MEMOP: %mul0 = fmul fast float %a, %b
; CHECK-MEMOP: %mul1 = fmul fast float %c, %d
; CHECK-MEMOP: %[[ADD1:[a-z0-9]+]] = fadd fast float %mul0, %Candidate
; CHECK-MEMOP: %[[ADD0:[a-z0-9]+]] = fadd fast float %[[ADD1]], %mul1
; CHECK-MEMOP: ret float %[[ADD0]]


; Negative test: chain ends with a multiply, not a non-mul operand
; Should NOT be transformed, since it already generates a MAD for the last instruction
define float @test_all_muls(float %a, float %b, float %c, float %d, float %e, float %f) #0 {
entry:
  %mul0 = fmul fast float %a, %b
  %mul1 = fmul fast float %c, %d
  %mul2 = fmul fast float %e, %f
  %add0 = fadd fast float %mul0, %mul1
  %add1 = fadd fast float %add0, %mul2
  ret float %add1
}

; CHECK-LABEL: define float @test_all_muls
; The chain doesn't end with a non-multiply operand, so no reassociation
; CHECK: %mul0 = fmul fast float %a, %b
; CHECK: %mul1 = fmul fast float %c, %d
; CHECK: %mul2 = fmul fast float %e, %f
; CHECK: %add0 = fadd fast float %mul0, %mul1
; CHECK: %add1 = fadd fast float %add0, %mul2
; CHECK: ret float %add1


; Negative test: root fadd has multiple uses - should NOT be optimized
define float @test_multiple_uses(float %a, float %b, float %c, float %d, float %Candidate) #0 {
entry:
  %mul0 = fmul fast float %a, %b
  %mul1 = fmul fast float %c, %d
  %add0 = fadd fast float %mul0, %mul1
  %add1 = fadd fast float %add0, %Candidate
  %use = fadd fast float %add0, 1.0  ; extra use of add0
  %result = fadd fast float %add1, %use
  ret float %result
}

; CHECK-LABEL: define float @test_multiple_uses
; add0 has multiple uses, so we can't reassociate
; CHECK: %mul0 = fmul fast float %a, %b
; CHECK: %mul1 = fmul fast float %c, %d
; CHECK: %add0 = fadd fast float %mul0, %mul1
; CHECK: %add1 = fadd fast float %add0, %Candidate
; CHECK: %use = fadd fast float %add0, 1.000000e+00
; CHECK: %result = fadd fast float %add1, %use
; CHECK: ret float %result

; Test case with fsub at the root
; t0 = A * B
; t1 = C * D
; r0 = t0 - t1   ; fsub at root
; r1 = r0 + Candidate   ; Candidate is not a multiply
;
; Should become:
; newT0 = t0 + Candidate  (MAD)
; r0' = newT0 - t1  ; preserve fsub from root
; r1 replaced with r0'

define float @test_reassociate_with_fsub_root(float %A, float %B, float %C, float %D, float %Candidate) {
; CHECK-LABEL: @test_reassociate_with_fsub_root
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], %Candidate
; CHECK: [[R0:%.*]] = fsub fast float [[NEW]], [[T1]]
; CHECK: ret float [[R0]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %r0 = fsub fast float %t0, %t1
  %r1 = fadd fast float %r0, %Candidate
  ret float %r1
}

;===============================================================================
; Tests for chain in operand 1 of fsub ("OtherOp - Chain" pattern)
;===============================================================================

; Test: Candidate - (A*B + C*D) where chain is in operand 1 of fsub
; Original: Candidate - (A*B + C*D) = Candidate - A*B - C*D
; Expected: fsub(Candidate, T0) for the MAD pairing, then fsub T1
;
; t0 = A * B
; t1 = C * D
; r0 = t0 + t1
; r1 = Candidate - r0      ; chain is operand 1!
;
; Should become:
; newT0 = Candidate - t0   (forms MAD by applying negative source modifier for A or B)
; r1 = newT0 - t1

define float @test_chain_in_fsub_op1_basic(float %A, float %B, float %C, float %D, float %Candidate) {
; CHECK-LABEL: @test_chain_in_fsub_op1_basic
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[NEW:%.*]] = fsub fast float %Candidate, [[T0]]
; CHECK: [[R1:%.*]] = fsub fast float [[NEW]], [[T1]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %r0 = fadd fast float %t0, %t1
  %r1 = fsub fast float %Candidate, %r0
  ret float %r1
}

; Test: Candidate - (A*B - C*D) where chain is in operand 1 of fsub, root is fsub
; Original: Candidate - (A*B - C*D) = Candidate - A*B + C*D
; T0 negated, T1 positive, Candidate positive
;
; t0 = A * B
; t1 = C * D
; r0 = t0 - t1     ; fsub at root
; r1 = Candidate - r0      ; chain is operand 1!
;
; Should become:
; newT0 = Candidate - t0   (forms MAD by applying negative source modifier for A or B)
; r1 = newT0 + t1  ; T1 sign flipped from original root fsub

define float @test_chain_in_fsub_op1_with_fsub_root(float %A, float %B, float %C, float %D, float %Candidate) {
; CHECK-LABEL: @test_chain_in_fsub_op1_with_fsub_root
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[NEW:%.*]] = fsub fast float %Candidate, [[T0]]
; CHECK: [[R1:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %r0 = fsub fast float %t0, %t1
  %r1 = fsub fast float %Candidate, %r0
  ret float %r1
}

; Test: T0 and Candidate both negative - produces fneg + fsub pattern
; t0 = A * B
; t1 = C * D
; t2 = E * F
; r0 = t0 + t1
; r1 = t2 - r0    ; t2 - t0 - t1
; r2 = r1 - Candidate     ; t2 - t0 - t1 - Candidate
;
; T0 negated, T1 negated, t2 positive, Candidate subtracted
; Result: -T0 - Candidate requires fneg(Candidate) then fsub

define float @test_t0_neg_candidate_neg(float %A, float %B, float %C, float %D, float %E, float %F, float %Candidate) {
; CHECK-LABEL: @test_t0_neg_candidate_neg
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[T2:%.*]] = fmul fast float %E, %F
; CHECK: [[FNEG:%.*]] = fneg fast float %Candidate
; CHECK: [[NEW:%.*]] = fsub fast float [[FNEG]], [[T0]]
; CHECK: [[R0:%.*]] = fsub fast float [[NEW]], [[T1]]
; CHECK: [[R1:%.*]] = fadd fast float [[R0]], [[T2]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %t2 = fmul fast float %E, %F
  %r0 = fadd fast float %t0, %t1
  %r1 = fsub fast float %t2, %r0
  %r2 = fsub fast float %r1, %Candidate
  ret float %r2
}

; Test: Chain in op1 with intermediate FMuls
; Candidate - ((A*B + C*D) + E*F) = Candidate - A*B - C*D - E*F
; All terms negated by "Candidate - chain" pattern
;
; t0 = A * B
; t1 = C * D
; t2 = E * F
; r0 = t0 + t1
; r1 = r0 + t2
; r2 = Candidate - r1

define float @test_chain_op1_with_intermediate(float %A, float %B, float %C, float %D, float %E, float %F, float %Candidate) {
; CHECK-LABEL: @test_chain_op1_with_intermediate
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[T2:%.*]] = fmul fast float %E, %F
; CHECK: [[NEW:%.*]] = fsub fast float %Candidate, [[T0]]
; CHECK: [[R0:%.*]] = fsub fast float [[NEW]], [[T1]]
; CHECK: [[R1:%.*]] = fsub fast float [[R0]], [[T2]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %t2 = fmul fast float %E, %F
  %r0 = fadd fast float %t0, %t1
  %r1 = fadd fast float %r0, %t2
  %r2 = fsub fast float %Candidate, %r1
  ret float %r2
}

; Test: Chain negation at intermediate level
; t0 = A * B
; t1 = C * D
; t2 = E * F
; r0 = t0 + t1
; r1 = t2 - r0    ; t2 - t0 - t1 (chain in op1)
; r2 = r1 + Candidate     ; t2 - t0 - t1 + Candidate
;
; T0 negated, T1 negated, t2 positive, Candidate positive

define float @test_intermediate_chain_negation(float %A, float %B, float %C, float %D, float %E, float %F, float %Candidate) {
; CHECK-LABEL: @test_intermediate_chain_negation
; CHECK: [[T0:%.*]] = fmul fast float %A, %B
; CHECK: [[T1:%.*]] = fmul fast float %C, %D
; CHECK: [[T2:%.*]] = fmul fast float %E, %F
; CHECK: [[NEW:%.*]] = fsub fast float %Candidate, [[T0]]
; CHECK: [[R0:%.*]] = fsub fast float [[NEW]], [[T1]]
; CHECK: [[R1:%.*]] = fadd fast float [[R0]], [[T2]]
; CHECK: ret float [[R1]]
  %t0 = fmul fast float %A, %B
  %t1 = fmul fast float %C, %D
  %t2 = fmul fast float %E, %F
  %r0 = fadd fast float %t0, %t1
  %r1 = fsub fast float %t2, %r0
  %r2 = fadd fast float %r1, %Candidate
  ret float %r2
}

;===============================================================================
; Tests for parallel chains - should NOT be serialized
;===============================================================================

; Test: Three parallel MAD chains merged via reduction
; This tests that we don't accidentally serialize parallel chains, which would
; hurt performance. Each chain's Candidate is another chain's intermediate result,
; so all chains should be filtered out.
;
; Chain 1: t0*t1 + t2 -> s1
; Chain 2: t3*t4 + t5 -> s3
; Chain 3: t6*t7 + t8 -> s5
; Reduction: s1 + s3 + s5 -> s7
;
; Should NOT transform - parallel structure must be preserved

define float @test_parallel_chains_no_serialize(float %a0, float %a1, float %b0, float %b1, float %c0, float %c1,
                                                 float %d0, float %d1, float %e0, float %e1, float %f0, float %f1,
                                                 float %g0, float %g1, float %h0, float %h1, float %i0, float %i1) {
; CHECK-LABEL: @test_parallel_chains_no_serialize
; All three chains should remain parallel - no serialization
; CHECK: [[T0:%.*]] = fmul fast float %a0, %a1
; CHECK: [[T1:%.*]] = fmul fast float %b0, %b1
; CHECK: [[S0:%.*]] = fadd fast float [[T0]], [[T1]]
; CHECK: [[T2:%.*]] = fmul fast float %c0, %c1
; CHECK: [[S1:%.*]] = fadd fast float [[S0]], [[T2]]
; CHECK: [[T3:%.*]] = fmul fast float %d0, %d1
; CHECK: [[T4:%.*]] = fmul fast float %e0, %e1
; CHECK: [[S2:%.*]] = fadd fast float [[T3]], [[T4]]
; CHECK: [[T5:%.*]] = fmul fast float %f0, %f1
; CHECK: [[S3:%.*]] = fadd fast float [[S2]], [[T5]]
; CHECK: [[T6:%.*]] = fmul fast float %g0, %g1
; CHECK: [[T7:%.*]] = fmul fast float %h0, %h1
; CHECK: [[S4:%.*]] = fadd fast float [[T6]], [[T7]]
; CHECK: [[T8:%.*]] = fmul fast float %i0, %i1
; CHECK: [[S5:%.*]] = fadd fast float [[S4]], [[T8]]
; CHECK: [[S6:%.*]] = fadd fast float [[S1]], [[S3]]
; CHECK: [[S7:%.*]] = fadd fast float [[S6]], [[S5]]
; CHECK: ret float [[S7]]
entry:
  ; Chain 1
  %t0 = fmul fast float %a0, %a1
  %t1 = fmul fast float %b0, %b1
  %s0 = fadd fast float %t0, %t1
  %t2 = fmul fast float %c0, %c1
  %s1 = fadd fast float %s0, %t2

  ; Chain 2
  %t3 = fmul fast float %d0, %d1
  %t4 = fmul fast float %e0, %e1
  %s2 = fadd fast float %t3, %t4
  %t5 = fmul fast float %f0, %f1
  %s3 = fadd fast float %s2, %t5

  ; Chain 3
  %t6 = fmul fast float %g0, %g1
  %t7 = fmul fast float %h0, %h1
  %s4 = fadd fast float %t6, %t7
  %t8 = fmul fast float %i0, %i1
  %s5 = fadd fast float %s4, %t8

  ; Reduction - merges all chains
  %s6 = fadd fast float %s1, %s3
  %s7 = fadd fast float %s6, %s5

  ret float %s7
}

; Test: Two parallel chains converging at same fadd
; Both chains terminate at s2, each using the other's intermediate as Candidate.
; Neither chain should be transformed.

define float @test_two_chains_converge(float %a, float %b, float %c, float %d,
                                        float %e, float %f, float %g, float %h) {
; CHECK-LABEL: @test_two_chains_converge
; CHECK: [[T0:%.*]] = fmul fast float %a, %b
; CHECK: [[T1:%.*]] = fmul fast float %c, %d
; CHECK: [[S0:%.*]] = fadd fast float [[T0]], [[T1]]
; CHECK: [[T2:%.*]] = fmul fast float %e, %f
; CHECK: [[T3:%.*]] = fmul fast float %g, %h
; CHECK: [[S1:%.*]] = fadd fast float [[T2]], [[T3]]
; CHECK: [[S2:%.*]] = fadd fast float [[S0]], [[S1]]
; CHECK: ret float [[S2]]
entry:
  ; Chain A
  %t0 = fmul fast float %a, %b
  %t1 = fmul fast float %c, %d
  %s0 = fadd fast float %t0, %t1

  ; Chain B
  %t2 = fmul fast float %e, %f
  %t3 = fmul fast float %g, %h
  %s1 = fadd fast float %t2, %t3

  ; Convergence - would make each chain's Candidate the other chain's output
  %s2 = fadd fast float %s0, %s1

  ret float %s2
}

;===============================================================================
; Tests for memory load dependency check enhancement
;===============================================================================

; Test: Two independent chains with non-load Candidates
; Both should be transformed independently

define float @test_two_independent_chains(float %a, float %b, float %c, float %d,
                                           float %e, float %f, float %g, float %h,
                                           float %x, float %y) {
; CHECK-LABEL: @test_two_independent_chains
; Chain A transformed
; CHECK: [[T0:%.*]] = fmul fast float %a, %b
; CHECK: [[T1:%.*]] = fmul fast float %c, %d
; CHECK: [[NEW_A:%.*]] = fadd fast float [[T0]], %x
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW_A]], [[T1]]
; Chain B transformed
; CHECK: [[T2:%.*]] = fmul fast float %e, %f
; CHECK: [[T3:%.*]] = fmul fast float %g, %h
; CHECK: [[NEW_B:%.*]] = fadd fast float [[T2]], %y
; CHECK: [[ADD1:%.*]] = fadd fast float [[NEW_B]], [[T3]]
; Results combined
; CHECK: [[RESULT:%.*]] = fadd fast float [[ADD0]], [[ADD1]]
; CHECK: ret float [[RESULT]]
entry:
  ; Chain A with Candidate x
  %t0 = fmul fast float %a, %b
  %t1 = fmul fast float %c, %d
  %s0 = fadd fast float %t0, %t1
  %s1 = fadd fast float %s0, %x

  ; Chain B with Candidate y
  %t2 = fmul fast float %e, %f
  %t3 = fmul fast float %g, %h
  %s2 = fadd fast float %t2, %t3
  %s3 = fadd fast float %s2, %y

  ; Independent combination
  %result = fadd fast float %s1, %s3
  ret float %result
}

; Test: Candidate has other external uses - should still work
; The Candidate value is used elsewhere, but this should not prevent the transform

define float @test_candidate_has_other_uses(float %a, float %b, float %c, float %d, float %Candidate) {
; CHECK-LABEL: @test_candidate_has_other_uses
; CHECK: [[T0:%.*]] = fmul fast float %a, %b
; CHECK: [[T1:%.*]] = fmul fast float %c, %d
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], %Candidate
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: [[OTHER:%.*]] = fadd fast float %Candidate, 1.000000e+00
; CHECK: [[RESULT:%.*]] = fadd fast float [[ADD0]], [[OTHER]]
; CHECK: ret float [[RESULT]]
entry:
  %t0 = fmul fast float %a, %b
  %t1 = fmul fast float %c, %d
  %s0 = fadd fast float %t0, %t1
  %s1 = fadd fast float %s0, %Candidate

  ; Another use of Candidate
  %other = fadd fast float %Candidate, 1.0

  %result = fadd fast float %s1, %other
  ret float %result
}

; Test: Constant Candidate - should work (constants can't be erased)

define float @test_constant_candidate(float %a, float %b, float %c, float %d) {
; CHECK-LABEL: @test_constant_candidate
; CHECK: [[T0:%.*]] = fmul fast float %a, %b
; CHECK: [[T1:%.*]] = fmul fast float %c, %d
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], 4.200000e+01
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: ret float [[ADD0]]
entry:
  %t0 = fmul fast float %a, %b
  %t1 = fmul fast float %c, %d
  %s0 = fadd fast float %t0, %t1
  %s1 = fadd fast float %s0, 42.0
  ret float %s1
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"MadEnable", i1 true}
