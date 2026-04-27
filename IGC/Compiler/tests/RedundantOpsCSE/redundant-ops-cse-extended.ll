;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

; REQUIRES: regkeys, llvm-14-plus

; Default run: unlimited distance (MaxDist=0), CSE should fire everywhere
; RUN: igc_opt --opaque-pointers -platformPtl -redundant-ops-cse -regkey EnableRedundantOpsCSE=1 -regkey EnableRedundantOpsCrossBBCSE=1 -regkey RedundantOpsIntraBBMaxDist=0 -S %s | FileCheck %s --check-prefix=CHECK

; Distance-guarded run: MaxDist=3, distant duplicates must survive
; RUN: igc_opt --opaque-pointers -platformPtl -redundant-ops-cse -regkey EnableRedundantOpsCSE=1 -regkey EnableRedundantOpsCrossBBCSE=1 -regkey RedundantOpsIntraBBMaxDist=3 -S %s | FileCheck %s --check-prefix=DIST

; Test 1: Three identical adds in one BB — only the first should survive
define i64 @test_triple_redundant(i64 %x) {
; CHECK-LABEL: define i64 @test_triple_redundant
; CHECK:         %sum1 = add i64 %x, 42
; CHECK-NOT:     %sum2 = add i64 %x, 42
; CHECK-NOT:     %sum3 = add i64 %x, 42
; CHECK:         %r1 = mul i64 %sum1, %sum1
; CHECK-NEXT:    %r2 = mul i64 %r1, %sum1
; CHECK-NEXT:    ret i64 %r2
;
; DIST-LABEL: define i64 @test_triple_redundant
; DIST:         %sum1 = add i64 %x, 42
; DIST-NOT:     %sum2 = add i64 %x, 42
; DIST-NOT:     %sum3 = add i64 %x, 42
; DIST:         %r1 = mul i64 %sum1, %sum1
; DIST-NEXT:    %r2 = mul i64 %r1, %sum1
; DIST-NEXT:    ret i64 %r2
entry:
  %sum1 = add i64 %x, 42
  %sum2 = add i64 %x, 42
  %sum3 = add i64 %x, 42
  %r1 = mul i64 %sum1, %sum2
  %r2 = mul i64 %r1, %sum3
  ret i64 %r2
}

; Test 2: Different binary operators — each should be CSE'd independently
define i64 @test_different_binops(i64 %x) {
; CHECK-LABEL: define i64 @test_different_binops
; CHECK:         %a1 = mul i64 %x, 7
; CHECK-NOT:     %a2 = mul i64 %x, 7
; CHECK:         %b1 = shl i64 %x, 3
; CHECK-NOT:     %b2 = shl i64 %x, 3
; CHECK:         %t1 = add i64 %a1, %b1
; CHECK:         %t2 = sub i64 %a1, %b1
;
; DIST-LABEL: define i64 @test_different_binops
; DIST:         %a1 = mul i64 %x, 7
; DIST-NOT:     %a2 = mul i64 %x, 7
; DIST:         %b1 = shl i64 %x, 3
; DIST-NOT:     %b2 = shl i64 %x, 3
; DIST:         %t1 = add i64 %a1, %b1
; DIST:         %t2 = sub i64 %a1, %b1
entry:
  %a1 = mul i64 %x, 7
  %b1 = shl i64 %x, 3
  %a2 = mul i64 %x, 7
  %b2 = shl i64 %x, 3
  %t1 = add i64 %a1, %b1
  %t2= sub i64 %a2, %b2
  %result = mul i64 %t1, %t2
  ret i64 %result
}

; Test 3: Commutative operand order — should NOT be eliminated
define i64 @test_commutative_preserved(i64 %a, i64 %b) {
; CHECK-LABEL: define i64 @test_commutative_preserved
; CHECK:         %sum1 = add i64 %a, %b
; CHECK-NEXT:    %sum2 = add i64 %b, %a
; CHECK-NEXT:    %result = mul i64 %sum1, %sum2
; CHECK-NEXT:    ret i64 %result
;
; DIST-LABEL: define i64 @test_commutative_preserved
; DIST:         %sum1 = add i64 %a, %b
; DIST-NEXT:    %sum2 = add i64 %b, %a
; DIST-NEXT:    %result = mul i64 %sum1, %sum2
; DIST-NEXT:    ret i64 %result
entry:
  %sum1 = add i64 %a, %b
  %sum2 = add i64 %b, %a
  %result = mul i64 %sum1, %sum2
  ret i64 %result
}

; Test 4: Same opcode, different types — should NOT be eliminated
define i64 @test_different_types(i32 %x32, i64 %x64) {
; CHECK-LABEL: define i64 @test_different_types
; CHECK:         %sum32 = add i32 %x32, 42
; CHECK-NEXT:    %sum64 = add i64 %x64, 42
;
; DIST-LABEL: define i64 @test_different_types
; DIST:         %sum32 = add i32 %x32, 42
; DIST-NEXT:    %sum64 = add i64 %x64, 42
entry:
  %sum32 = add i32 %x32, 42
  %sum64 = add i64 %x64, 42
  %ext = zext i32 %sum32 to i64
  %result = add i64 %ext, %sum64
  ret i64 %result
}

; Test 5: Diamond CFG — entry dominates merge, so merge's add is redundant
define i64 @test_diamond_cfg(i64 %x, i1 %cond) {
; CHECK-LABEL: define i64 @test_diamond_cfg
; CHECK:       entry:
; CHECK-NEXT:    %sum1 = add i64 %x, 42
; CHECK:       merge:
; CHECK-NOT:     %sum2 = add i64 %x, 42
; CHECK:         %result = mul i64 %sum1, 2
;
; DIST-LABEL: define i64 @test_diamond_cfg
; DIST:       entry:
; DIST-NEXT:    %sum1 = add i64 %x, 42
; DIST:       merge:
; DIST-NOT:     %sum2 = add i64 %x, 42
; DIST:         %result = mul i64 %sum1, 2
entry:
  %sum1 = add i64 %x, 42
  br i1 %cond, label %left, label %right

left:
  br label %merge

right:
  br label %merge

merge:
  %sum2 = add i64 %x, 42
  %result = mul i64 %sum2, 2
  ret i64 %result
}

; Test 6: Chain of dominating BBs (A dom B dom C) — B and C should be eliminated
define i64 @test_chain_dominance(i64 %x, i1 %c1, i1 %c2) {
; CHECK-LABEL: define i64 @test_chain_dominance
; CHECK:       entry:
; CHECK-NEXT:    %sum1 = add i64 %x, 42
; CHECK:       bb1:
; CHECK-NOT:     %sum2 = add i64 %x, 42
; CHECK:       bb2:
; CHECK-NOT:     %sum3 = add i64 %x, 42
;
; DIST-LABEL: define i64 @test_chain_dominance
; DIST:       entry:
; DIST-NEXT:    %sum1 = add i64 %x, 42
; DIST:       bb1:
; DIST-NOT:     %sum2 = add i64 %x, 42
; DIST:       bb2:
; DIST-NOT:     %sum3 = add i64 %x, 42
entry:
  %sum1 = add i64 %x, 42
  br label %bb1

bb1:
  %sum2 = add i64 %x, 42
  %use1 = mul i64 %sum2, 2
  br label %bb2

bb2:
  %sum3 = add i64 %x, 42
  %use2 = mul i64 %sum3, 3
  %result = add i64 %use1, %use2
  ret i64 %result
}

; Test 7: Side-effect instruction between redundant adds — CSE should still work
define i64 @test_side_effect_between(i64 %x, i64* %ptr) {
; CHECK-LABEL: define i64 @test_side_effect_between
; CHECK:         %sum1 = add i64 %x, 42
; CHECK:         store i64 %sum1,
; CHECK-NOT:     %sum2 = add i64 %x, 42
; CHECK:         %result = mul i64 %sum1, 2
;
; DIST-LABEL: define i64 @test_side_effect_between
; DIST:         %sum1 = add i64 %x, 42
; DIST:         store i64 %sum1,
; DIST-NOT:     %sum2 = add i64 %x, 42
; DIST:         %result = mul i64 %sum1, 2
entry:
  %sum1 = add i64 %x, 42
  store i64 %sum1, i64* %ptr
  %sum2 = add i64 %x, 42
  %result = mul i64 %sum2, 2
  ret i64 %result
}

; Test 8: nsw flag difference — should NOT be eliminated
define i64 @test_nsw_flag_difference(i64 %x) {
; CHECK-LABEL: define i64 @test_nsw_flag_difference
; CHECK:         %sum1 = add nsw i64 %x, 42
; CHECK-NEXT:    %sum2 = add i64 %x, 42
; CHECK-NEXT:    %result = mul i64 %sum1, %sum2
; CHECK-NEXT:    ret i64 %result
;
; DIST-LABEL: define i64 @test_nsw_flag_difference
; DIST:         %sum1 = add nsw i64 %x, 42
; DIST-NEXT:    %sum2 = add i64 %x, 42
; DIST-NEXT:    %result = mul i64 %sum1, %sum2
; DIST-NEXT:    ret i64 %result
entry:
  %sum1 = add nsw i64 %x, 42
  %sum2 = add i64 %x, 42
  %result = mul i64 %sum1, %sum2
  ret i64 %result
}

; Test 9: Single instruction in BB — no crash, no modification
define i64 @test_single_instruction(i64 %x) {
; CHECK-LABEL: define i64 @test_single_instruction
; CHECK:         %sum = add i64 %x, 42
; CHECK-NEXT:    ret i64 %sum
;
; DIST-LABEL: define i64 @test_single_instruction
; DIST:         %sum = add i64 %x, 42
; DIST-NEXT:    ret i64 %sum
entry:
  %sum = add i64 %x, 42
  ret i64 %sum
}

; Test 10: Bitwise ops — and/or/xor redundancy
define i64 @test_bitwise_ops(i64 %x) {
; CHECK-LABEL: define i64 @test_bitwise_ops
; CHECK:         %a1 = and i64 %x, 255
; CHECK-NEXT:    %b1 = or i64 %x, 15
; CHECK-NEXT:    %c1 = xor i64 %x, 128
; CHECK-NOT:     %a2 = and i64 %x, 255
; CHECK-NOT:     %b2 = or i64 %x, 15
; CHECK-NOT:     %c2 = xor i64 %x, 128
;
; DIST-LABEL: define i64 @test_bitwise_ops
; DIST:         %a1 = and i64 %x, 255
; DIST-NEXT:    %b1 = or i64 %x, 15
; DIST-NEXT:    %c1 = xor i64 %x, 128
; DIST-NOT:     %a2 = and i64 %x, 255
; DIST-NOT:     %b2 = or i64 %x, 15
; DIST-NOT:     %c2 = xor i64 %x, 128
entry:
  %a1 = and i64 %x, 255
  %b1 = or i64 %x, 15
  %c1 = xor i64 %x, 128
  %a2 = and i64 %x, 255
  %b2 = or i64 %x, 15
  %c2 = xor i64 %x, 128
  %r1 = add i64 %a2, %b2
  %r2 = add i64 %r1, %c2
  ret i64 %r2
}

; Test 11: Distance guard — identical adds separated by many instructions.
; With MaxDist=0 (CHECK): CSE eliminates the duplicate.
; With MaxDist=3 (DIST): distance of 6 exceeds the limit, duplicate survives.
define i64 @test_distance_guard_intra_bb(i64 %x) {
; CHECK-LABEL: define i64 @test_distance_guard_intra_bb
; CHECK:         %add1 = add i64 %x, 77
; CHECK-NOT:     %add2 = add i64 %x, 77
; CHECK:         %res = add i64
; CHECK:         ret i64
;
; DIST-LABEL: define i64 @test_distance_guard_intra_bb
; DIST:         %add1 = add i64 %x, 77
; DIST:         %add2 = add i64 %x, 77
; DIST:         %res = add i64
; DIST:         ret i64
entry:
  %add1 = add i64 %x, 77
  %f1 = mul i64 %add1, 2
  %f2 = mul i64 %f1, 3
  %f3 = mul i64 %f2, 5
  %f4 = mul i64 %f3, 7
  %f5 = mul i64 %f4, 11
  %add2 = add i64 %x, 77
  %res = add i64 %f5, %add2
  ret i64 %res
}

; Test 12: Distance guard with close duplicates — even with MaxDist=3,
; duplicates within the distance limit should still be eliminated.
define i64 @test_distance_guard_close_duplicates(i64 %x) {
; CHECK-LABEL: define i64 @test_distance_guard_close_duplicates
; CHECK:         %add1 = add i64 %x, 55
; CHECK-NOT:     %add2 = add i64 %x, 55
; CHECK:         %res = add i64
; CHECK:         ret i64
;
; DIST-LABEL: define i64 @test_distance_guard_close_duplicates
; DIST:         %add1 = add i64 %x, 55
; DIST-NOT:     %add2 = add i64 %x, 55
; DIST:         %res = add i64
; DIST:         ret i64
entry:
  %add1 = add i64 %x, 55
  %f1 = mul i64 %add1, 2
  %add2 = add i64 %x, 55
  %res = add i64 %f1, %add2
  ret i64 %res
}
