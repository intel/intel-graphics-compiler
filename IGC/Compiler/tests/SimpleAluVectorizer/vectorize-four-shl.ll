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
; Test: Four uniform i32 shls coalesced into a single <4 x i32> shl.
;
; RUN: igc_opt --opaque-pointers -platformPtl --igc-simple-alu-vectorizer -S < %s | FileCheck %s
;
; CHECK: %valu = shl <4 x i32> %{{[0-9]+}}, %{{[0-9]+}}, !igc.simple.alu.vectorized !3
; CHECK-NOT: %r0 = shl i32
; CHECK-NOT: %r1 = shl i32
; CHECK-NOT: %r2 = shl i32
; CHECK-NOT: %r3 = shl i32

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_four_shl(i32 %a, i32 %b, i32 %c, i32 %d, i32 %e, ptr addrspace(1) %out) #0 {
entry:
  %r0 = shl i32 %a, %b
  %r1 = shl i32 %a, %c
  %r2 = shl i32 %a, %d
  %r3 = shl i32 %a, %e
  %ptr0 = getelementptr i32, ptr addrspace(1) %out, i64 0
  %ptr1 = getelementptr i32, ptr addrspace(1) %out, i64 1
  %ptr2 = getelementptr i32, ptr addrspace(1) %out, i64 2
  %ptr3 = getelementptr i32, ptr addrspace(1) %out, i64 3
  store i32 %r0, ptr addrspace(1) %ptr0, align 4
  store i32 %r1, ptr addrspace(1) %ptr1, align 4
  store i32 %r2, ptr addrspace(1) %ptr2, align 4
  store i32 %r3, ptr addrspace(1) %ptr3, align 4
  ret void
}

attributes #0 = { nounwind }

!igc.functions = !{!0}
!0 = !{ptr @test_four_shl, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
