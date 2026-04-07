;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: system-windows, llvm-17-plus
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=6 < %s 2>&1 | FileCheck %s

define spir_kernel void @testMixed(float addrspace(1)* %out, float addrspace(1)* %in, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %lid   = zext i16 %localIdX to i32
  %base  = ptrtoint float addrspace(1)* %in to i64
  %idx   = zext i32 %lid to i64
  %ptr   = getelementptr inbounds float, float addrspace(1)* %in, i64 %idx
  %val   = load float, float addrspace(1)* %ptr, align 4
  %outptr = getelementptr inbounds float, float addrspace(1)* %out, i64 %idx
  store float %val, float addrspace(1)* %outptr, align 4
  ret void
}

!igc.functions = !{!0}
!0 = !{void (float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, i16)* @testMixed, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 8}

; Uniform pointer args keep UP > 0 throughout; non-uniform localId makes NP > 0.
; CHECK: block: entry function: testMixed
; CHECK: N: UP: 1 NP: 1     48 (2)         %lid = zext i16 %localIdX to i32
; CHECK: N: UP: 1 NP: 2     80 (3)         %idx = zext i32 %lid to i64
; CHECK: N: UP: 1 NP: 4     136 (5)        %ptr = getelementptr inbounds float, float addrspace(1)* %in, i64 %idx
; CHECK: N: UP: 1 NP: 3     104 (4)        %val = load float, float addrspace(1)* %ptr, align 4
; CHECK: N: UP: 0 NP: 3     96 (3)         %outptr = getelementptr inbounds float, float addrspace(1)* %out, i64 %idx
; CHECK: MaxPressure In Function: testMixed --> 5
