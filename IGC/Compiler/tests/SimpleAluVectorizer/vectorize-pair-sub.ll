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
; Test: Two uniform i32 subs sharing the same operands are coalesced into a
; single vector sub. Both subs use the same pair of runtime-value operands
; so each operand slot is AllSame (pack cost 0), making coalescing profitable.
;
; RUN: igc_opt --opaque-pointers -platformPtl -inputcs --igc-simple-alu-vectorizer -S < %s | FileCheck %s
;
; CHECK: %valu = sub <2 x i32> %{{[0-9]+}}, %{{[0-9]+}}, !igc.simple.alu.vectorized !3
; CHECK: extractelement <2 x i32> %valu, i32 0
; CHECK: extractelement <2 x i32> %valu, i32 1
; CHECK-NOT: %r0 = sub i32
; CHECK-NOT: %r1 = sub i32

; ModuleID = 'test'
source_filename = "test"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_pair_sub(i32 %a, i32 %b, i32 %c, ptr addrspace(1) %out) #0 {
entry:
  %r0 = sub i32 %a, %b
  %r1 = sub i32 %a, %c
  %ptr0 = getelementptr i32, ptr addrspace(1) %out, i64 0
  %ptr1 = getelementptr i32, ptr addrspace(1) %out, i64 1
  store i32 %r0, ptr addrspace(1) %ptr0, align 4
  store i32 %r1, ptr addrspace(1) %ptr1, align 4
  ret void
}

attributes #0 = { nounwind }

!igc.functions = !{!0}
!0 = !{ptr @test_pair_sub, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
