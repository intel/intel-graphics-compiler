;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; Test: Three stacked groups of 4 isomorphic uniform i32 binary ops.
;
;   Group A (4 x add) -> Group B (4 x mul) -> Group C (4 x sub) -> scalar stores
;
; The new "vector-friendly consumer" gate works as follows for each group:
;   - Group A's users are muls in Group B, which are themselves vectorization
;     candidates -> gate passes -> adds become <4 x i32> add.
;   - Group B's users are subs in Group C, also candidates -> gate passes ->
;     muls become <4 x i32> mul.
;   - Group C's users are plain scalar stores (not candidates, not vector
;     operands) -> gate rejects -> subs stay scalar.
;
; This simultaneously demonstrates:
;   (a) the gate ALLOWS legitimate chained vectorization (A and B), and
;
; RUN: igc_opt --opaque-pointers -platformPtl -igc-simple-alu-vectorizer -S < %s | FileCheck %s
;
; A chain of 4 uniform i32 adds whose results feed 4 uniform i32 muls.
; Both groups are isomorphic and uniform, so:
;   - the add group has a vector-friendly consumer (the muls are themselves
;     candidates), so the consumer gate allows vectorization.
;   - the mul group's results feed a store / further vector use.
;
; CHECK:     %valu{{.*}} = add <4 x i32> %{{[0-9]+}}, %{{[0-9]+}}, !igc.simple.alu.vectorized
; CHECK:     %valu{{.*}} = mul <4 x i32> %{{[0-9]+}}, %{{[0-9]+}}, !igc.simple.alu.vectorized
; CHECK-NOT: sub <4 x i32>
; CHECK:     sub i32
; CHECK:     sub i32
; CHECK:     sub i32
; CHECK:     sub i32

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @chain_add_mul_sub(i32 %a,
                                           i32 %b0, i32 %b1, i32 %b2, i32 %b3,
                                           i32 %c0, i32 %c1, i32 %c2, i32 %c3,
                                           i32 %d0, i32 %d1, i32 %d2, i32 %d3,
                                           ptr addrspace(1) %out) #0 {
entry:
  ; Group A: 4 uniform adds.  Users (muls below) are vectorization candidates.
  %s0 = add i32 %a, %b0
  %s1 = add i32 %a, %b1
  %s2 = add i32 %a, %b2
  %s3 = add i32 %a, %b3

  ; Group B: 4 uniform muls.  Users (subs below) are vectorization candidates.
  %m0 = mul i32 %s0, %c0
  %m1 = mul i32 %s1, %c1
  %m2 = mul i32 %s2, %c2
  %m3 = mul i32 %s3, %c3

  ; Group C: 4 uniform subs.  Users are scalar stores -> consumer gate
  ; rejects, so this group should stay scalar.
  %t0 = sub i32 %m0, %d0
  %t1 = sub i32 %m1, %d1
  %t2 = sub i32 %m2, %d2
  %t3 = sub i32 %m3, %d3

  %ptr0 = getelementptr i32, ptr addrspace(1) %out, i64 0
  %ptr1 = getelementptr i32, ptr addrspace(1) %out, i64 1
  %ptr2 = getelementptr i32, ptr addrspace(1) %out, i64 2
  %ptr3 = getelementptr i32, ptr addrspace(1) %out, i64 3
  store i32 %t0, ptr addrspace(1) %ptr0, align 4
  store i32 %t1, ptr addrspace(1) %ptr1, align 4
  store i32 %t2, ptr addrspace(1) %ptr2, align 4
  store i32 %t3, ptr addrspace(1) %ptr3, align 4
  ret void
}

attributes #0 = { nounwind }

!igc.functions = !{!0}
!0 = !{ptr @chain_add_mul_sub, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
