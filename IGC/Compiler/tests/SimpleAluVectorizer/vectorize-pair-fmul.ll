;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
;
; Test: Two uniform float fmuls with distinct operands are coalesced into a
; single vector fmul. Both fmuls use Arguments so each operand slot is
; free (pack cost 0), making coalescing profitable.
;
; RUN: igc_opt --opaque-pointers -platformPtl --igc-simple-alu-vectorizer -S < %s | FileCheck %s
;
; CHECK: %valu = fmul <2 x float> %{{[0-9]+}}, %{{[0-9]+}}, !igc.simple.alu.vectorized !3
; CHECK: extractelement <2 x float> %valu, i32 0
; CHECK: extractelement <2 x float> %valu, i32 1
; CHECK-NOT: %r0 = fmul float
; CHECK-NOT: %r1 = fmul float

; ModuleID = 'test'
source_filename = "test"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_pair_fmul(float %a, float %b, float %c, ptr addrspace(1) %out) #0 {
entry:
  %r0 = fmul float %a, %b
  %r1 = fmul float %a, %c
  %ptr0 = getelementptr float, ptr addrspace(1) %out, i64 0
  %ptr1 = getelementptr float, ptr addrspace(1) %out, i64 1
  store float %r0, ptr addrspace(1) %ptr0, align 4
  store float %r1, ptr addrspace(1) %ptr1, align 4
  ret void
}

attributes #0 = { nounwind }

!igc.functions = !{!0}
!0 = !{ptr @test_pair_fmul, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
