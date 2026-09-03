;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputocl -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=0 < %s | FileCheck %s --check-prefix=NOWIDEN
;
; RUN: igc_opt --opaque-pointers --platformbmg --inputocl -igc-memopt -S \
; RUN:   --regkey=EnableUniformSLMLoadWiden=1 < %s | FileCheck %s --check-prefix=WIDEN

; Test: rounding a uniform SLM load up to a legal LSC transposed vector length.
;
; The four loads at element offsets 0/2/4/6 merge through the uniform hull path
; into a 7-element load. LSC transposed d32 accepts only 1/2/3/4/8/16/32/64 -- no
; 7 -- so VectorPreProcess splits the <7 x i32> back into 4 + 3, undoing most of
; the merge.
;
; EnableUniformSLMLoadWiden bumps the hull to the next legal length first. Only
; 7/15/31/63 are both illegal and one short of legal, so this rounds up by at
; most one element, and only on LSC platforms in SLM, where the overread is safe.
;
; Both arms are OpenCL, which reaches the uniform hull path with the key off. That
; isolates the round-up: the bare 7-element hull is observable in the NOWIDEN arm.
; The key's other role - allowing that hull merge outside OpenCL - is covered by
; uniform-memopt-all-shader-types.ll.

define spir_kernel void @widen(ptr addrspace(3) %src, ptr addrspace(1) %dst) {
entry:
  %g0 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 0
  %l0 = load i32, ptr addrspace(3) %g0, align 4
  %g2 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 2
  %l2 = load i32, ptr addrspace(3) %g2, align 4
  %g4 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 4
  %l4 = load i32, ptr addrspace(3) %g4, align 4
  %g6 = getelementptr inbounds i32, ptr addrspace(3) %src, i64 6
  %l6 = load i32, ptr addrspace(3) %g6, align 4
  %a = add i32 %l0, %l2
  %b = add i32 %l4, %l6
  %c = add i32 %a, %b
  store i32 %c, ptr addrspace(1) %dst, align 4
  ret void
}

; Bare hull, not a legal transposed length.
;
; NOWIDEN-LABEL: define spir_kernel void @widen
; NOWIDEN:         %[[V:[0-9]+]] = load <7 x i32>, ptr addrspace(3) %g0, align 4
; NOWIDEN:         extractelement <7 x i32> %[[V]], i32 0
; NOWIDEN:         extractelement <7 x i32> %[[V]], i32 2
; NOWIDEN:         extractelement <7 x i32> %[[V]], i32 4
; NOWIDEN:         extractelement <7 x i32> %[[V]], i32 6

; Rounded 7 -> 8; the extract indices are unchanged, the tail element is dead.
;
; WIDEN-LABEL: define spir_kernel void @widen
; WIDEN:         %[[V:[0-9]+]] = load <8 x i32>, ptr addrspace(3) %g0, align 4
; WIDEN:         extractelement <8 x i32> %[[V]], i32 0
; WIDEN:         extractelement <8 x i32> %[[V]], i32 2
; WIDEN:         extractelement <8 x i32> %[[V]], i32 4
; WIDEN:         extractelement <8 x i32> %[[V]], i32 6

!igc.functions = !{!0}

!0 = !{ptr @widen, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
