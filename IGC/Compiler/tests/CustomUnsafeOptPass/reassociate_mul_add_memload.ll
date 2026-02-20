;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers -igc-custom-unsafe-opt-pass -S %s | FileCheck %s

; Tests for the memory load dependency enhancement in reassociateMulAdd.
; When Candidate comes from a memory load, we allow reassociation if T0 or T1
; also depends on the same load (since the critical path doesn't increase).

declare <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904), i32, i32, i1)

; Test: T0 and Candidate both depend on the same ldrawvector load
; This should be transformed because pairing them doesn't increase latency
;
; Pattern from real shader:
;   %load = ldrawvector 32xfloat
;   %165 = extractelement %load, 14   (used as Candidate)
;   %168 = extractelement %load, 2
;   %id382-139 = fmul %154, %168      (T0 depends on same load as Candidate)
;   %id382-140 = fmul %159, %167      (T1)
;   %id382-141 = fadd %id382-139, %id382-140
;   %id382-142 = fmul %id378-, %166
;   %id382-143 = fadd %id382-141, %id382-142
;   %id385- = fadd %id382-143, %165   (Candidate)
;
; T0 operand %168 comes from same load as Candidate %165, so allow reassociation

define float @test_same_load_t0_and_candidate(ptr addrspace(2555904) %ptr, i32 %idx, float %x, float %y, float %z, float %w) {
; CHECK-LABEL: @test_same_load_t0_and_candidate
; CHECK: [[LOAD:%.*]] = call <32 x float> @llvm.genx.GenISA.ldrawvector
; CHECK: [[E165:%.*]] = extractelement <32 x float> [[LOAD]], i32 14
; CHECK: [[E166:%.*]] = extractelement <32 x float> [[LOAD]], i32 10
; CHECK: [[E167:%.*]] = extractelement <32 x float> [[LOAD]], i32 6
; CHECK: [[E168:%.*]] = extractelement <32 x float> [[LOAD]], i32 2
; Chain should be reassociated - T0 paired with Candidate
; CHECK: [[T0:%.*]] = fmul fast float %x, [[E168]]
; CHECK: [[T1:%.*]] = fmul fast float %y, [[E167]]
; CHECK: [[T2:%.*]] = fmul fast float %z, [[E166]]
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], [[E165]]
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: [[ADD1:%.*]] = fadd fast float [[ADD0]], [[T2]]
; CHECK: ret float [[ADD1]]
entry:
  %load = call <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904) %ptr, i32 %idx, i32 4, i1 false)
  %e165 = extractelement <32 x float> %load, i32 14  ; Candidate
  %e166 = extractelement <32 x float> %load, i32 10
  %e167 = extractelement <32 x float> %load, i32 6
  %e168 = extractelement <32 x float> %load, i32 2   ; T0's operand, same load

  %t0 = fmul fast float %x, %e168
  %t1 = fmul fast float %y, %e167
  %t2 = fmul fast float %z, %e166

  %r0 = fadd fast float %t0, %t1
  %r1 = fadd fast float %r0, %t2
  %r2 = fadd fast float %r1, %e165  ; Candidate from same load as T0's operand

  ret float %r2
}

; Test: T1 depends on same load as Candidate, but T0 does not
; Should swap T0 and T1, then transform

define float @test_same_load_t1_and_candidate(ptr addrspace(2555904) %ptr, i32 %idx, float %x, float %y, float %other_val) {
; CHECK-LABEL: @test_same_load_t1_and_candidate
; CHECK: [[LOAD:%.*]] = call <32 x float> @llvm.genx.GenISA.ldrawvector
; CHECK: [[E0:%.*]] = extractelement <32 x float> [[LOAD]], i32 0
; CHECK: [[E1:%.*]] = extractelement <32 x float> [[LOAD]], i32 1
; T0 (fmul %x, %other_val) does NOT share load with Candidate
; T1 (fmul %y, %e0) DOES share load with Candidate
; Should swap: pair Candidate with what was T1
; CHECK: [[T0:%.*]] = fmul fast float %x, %other_val
; CHECK: [[T1:%.*]] = fmul fast float %y, [[E0]]
; CHECK: [[NEW:%.*]] = fadd fast float [[T1]], [[E1]]
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW]], [[T0]]
; CHECK: ret float [[ADD0]]
entry:
  %load = call <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904) %ptr, i32 %idx, i32 4, i1 false)
  %e0 = extractelement <32 x float> %load, i32 0  ; T1's operand
  %e1 = extractelement <32 x float> %load, i32 1  ; Candidate, same load

  %t0 = fmul fast float %x, %other_val  ; Does NOT depend on the load
  %t1 = fmul fast float %y, %e0         ; Depends on same load as Candidate

  %r0 = fadd fast float %t0, %t1
  %r1 = fadd fast float %r0, %e1        ; Candidate

  ret float %r1
}

; Test: Neither T0 nor T1 depends on Candidate's load
; Should NOT transform (different load sources)

define float @test_different_loads_no_transform(ptr addrspace(2555904) %ptr1, ptr addrspace(2555904) %ptr2, i32 %idx, float %x, float %y) {
; CHECK-LABEL: @test_different_loads_no_transform
; CHECK: [[LOAD1:%.*]] = call <32 x float> @llvm.genx.GenISA.ldrawvector{{.*}}%ptr1
; CHECK: [[LOAD2:%.*]] = call <32 x float> @llvm.genx.GenISA.ldrawvector{{.*}}%ptr2
; CHECK: [[E0:%.*]] = extractelement <32 x float> [[LOAD1]], i32 0
; CHECK: [[E1:%.*]] = extractelement <32 x float> [[LOAD1]], i32 1
; CHECK: [[CAND:%.*]] = extractelement <32 x float> [[LOAD2]], i32 0
; No transformation - Candidate is from different load
; CHECK: [[T0:%.*]] = fmul fast float %x, [[E0]]
; CHECK: [[T1:%.*]] = fmul fast float %y, [[E1]]
; CHECK: [[R0:%.*]] = fadd fast float [[T0]], [[T1]]
; CHECK: [[R1:%.*]] = fadd fast float [[R0]], [[CAND]]
; CHECK: ret float [[R1]]
entry:
  %load1 = call <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904) %ptr1, i32 %idx, i32 4, i1 false)
  %load2 = call <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904) %ptr2, i32 %idx, i32 4, i1 false)

  %e0 = extractelement <32 x float> %load1, i32 0  ; T0's operand
  %e1 = extractelement <32 x float> %load1, i32 1  ; T1's operand
  %cand = extractelement <32 x float> %load2, i32 0  ; Candidate from DIFFERENT load

  %t0 = fmul fast float %x, %e0
  %t1 = fmul fast float %y, %e1

  %r0 = fadd fast float %t0, %t1
  %r1 = fadd fast float %r0, %cand  ; Candidate from different load - no transform

  ret float %r1
}

; Test: Candidate is NOT from a load - should always transform

define float @test_candidate_not_from_load(ptr addrspace(2555904) %ptr, i32 %idx, float %x, float %y, float %Candidate) {
; CHECK-LABEL: @test_candidate_not_from_load
; CHECK: [[LOAD:%.*]] = call <32 x float> @llvm.genx.GenISA.ldrawvector
; CHECK: [[E0:%.*]] = extractelement <32 x float> [[LOAD]], i32 0
; CHECK: [[E1:%.*]] = extractelement <32 x float> [[LOAD]], i32 1
; Candidate is a function argument, not from load - always transform
; CHECK: [[T0:%.*]] = fmul fast float %x, [[E0]]
; CHECK: [[T1:%.*]] = fmul fast float %y, [[E1]]
; CHECK: [[NEW:%.*]] = fadd fast float [[T0]], %Candidate
; CHECK: [[ADD0:%.*]] = fadd fast float [[NEW]], [[T1]]
; CHECK: ret float [[ADD0]]
entry:
  %load = call <32 x float> @llvm.genx.GenISA.ldrawvector.indexed.v32f32.p2555904i8(ptr addrspace(2555904) %ptr, i32 %idx, i32 4, i1 false)
  %e0 = extractelement <32 x float> %load, i32 0
  %e1 = extractelement <32 x float> %load, i32 1

  %t0 = fmul fast float %x, %e0
  %t1 = fmul fast float %y, %e1

  %r0 = fadd fast float %t0, %t1
  %r1 = fadd fast float %r0, %Candidate  ; Not from a load

  ret float %r1
}

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"MadEnable", i1 true}
