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
; RUN: igc_opt --opaque-pointers -platformPtl -early-cse -redundant-ops-cse -regkey EnableRedundantOpsCSE=1 -regkey EnableRedundantOpsCrossBBCSE=1 -regkey RedundantOpsIntraBBMaxDist=0 -S %s | FileCheck %s --check-prefix=CHECK

; Distance-guarded run: MaxDist=3, distant duplicates must survive
; RUN: igc_opt --opaque-pointers -platformPtl -redundant-ops-cse -regkey EnableRedundantOpsCSE=1 -regkey EnableRedundantOpsCrossBBCSE=1 -regkey RedundantOpsIntraBBMaxDist=3 -S %s | FileCheck %s --check-prefix=DIST

; Test 1: Redundant add in the same basic block — second add should be eliminated
define i64 @test_redundant_add_same_bb(i64 %runtime_input_0) {
; CHECK-LABEL: define i64 @test_redundant_add_same_bb
; CHECK:         %sum1 = add i64 %runtime_input_0, 42
; CHECK-NEXT:    %use1 = mul i64 %sum1, 2
; CHECK-NOT:     %sum2 = add i64 %runtime_input_0, 42
; CHECK-NEXT:    %use2 = mul i64 %sum1, 3
; CHECK-NEXT:    %result = add i64 %use1, %use2
; CHECK-NEXT:    ret i64 %result
;
; DIST-LABEL: define i64 @test_redundant_add_same_bb
; DIST:         %sum1 = add i64 %runtime_input_0, 42
; DIST-NEXT:    %use1 = mul i64 %sum1, 2
; DIST-NOT:     %sum2 = add i64 %runtime_input_0, 42
; DIST-NEXT:    %use2 = mul i64 %sum1, 3
; DIST-NEXT:    %result = add i64 %use1, %use2
; DIST-NEXT:    ret i64 %result
entry:
  %sum1 = add i64 %runtime_input_0, 42
  %use1 = mul i64 %sum1, 2
  %sum2 = add i64 %runtime_input_0, 42
  %use2 = mul i64 %sum2, 3
  %result = add i64 %use1, %use2
  ret i64 %result
}

; Test 2: Redundant add across basic blocks — second add in bb2 should be
; eliminated because bb1 dominates bb2
define i64 @test_redundant_add_across_bbs(i64 %runtime_input_0, i1 %cond) {
; CHECK-LABEL: define i64 @test_redundant_add_across_bbs
; CHECK:       entry:
; CHECK-NEXT:    %sum1 = add i64 %runtime_input_0, 42
; CHECK-NEXT:    br i1 %cond, label %bb1, label %bb2
; CHECK:       bb1:
; should be removed by earlyCSE
; CHECK-NOT:     %use1 = mul i64 %sum1, 2
; CHECK-NEXT:    br label %bb2
; CHECK:       bb2:
; CHECK-NOT:     %sum2 = add i64 %runtime_input_0, 42
; CHECK:         %use2 = mul i64 %sum1, 3
; CHECK-NEXT:    %result = add i64 %use2, %sum1
; CHECK-NEXT:    ret i64 %result
entry:
  %sum1 = add i64 %runtime_input_0, 42
  br i1 %cond, label %bb1, label %bb2

bb1:
  %use1 = mul i64 %sum1, 2
  br label %bb2

bb2:
  %sum2 = add i64 %runtime_input_0, 42
  %use2 = mul i64 %sum2, 3
  %result = add i64 %use2, %sum2
  ret i64 %result
}

; Test 3: Non-redundant — different operands should NOT be eliminated
define i64 @test_different_operands_preserved(i64 %runtime_input_0) {
; CHECK-LABEL: define i64 @test_different_operands_preserved
; CHECK:         %sum1 = add i64 %runtime_input_0, 42
; CHECK-NEXT:    %sum2 = add i64 %runtime_input_0, 99
; CHECK-NEXT:    %result = add i64 %sum1, %sum2
; CHECK-NEXT:    ret i64 %result
;
; DIST-LABEL: define i64 @test_different_operands_preserved
; DIST:         %sum1 = add i64 %runtime_input_0, 42
; DIST-NEXT:    %sum2 = add i64 %runtime_input_0, 99
; DIST-NEXT:    %result = add i64 %sum1, %sum2
; DIST-NEXT:    ret i64 %result
entry:
  %sum1 = add i64 %runtime_input_0, 42
  %sum2 = add i64 %runtime_input_0, 99
  %result = add i64 %sum1, %sum2
  ret i64 %result
}

; Test 4: Non-dominating block — redundant add should NOT be eliminated
; because bb1 does not dominate bb2 (both are branches from entry)
define i64 @test_non_dominating_not_eliminated(i64 %runtime_input_0, i1 %cond) {
; CHECK-LABEL: define i64 @test_non_dominating_not_eliminated
; CHECK:       bb1:
; CHECK:         %sum1 = add i64 %runtime_input_0, 42
; CHECK:       bb2:
; CHECK:         %sum2 = add i64 %runtime_input_0, 42
;
; DIST-LABEL: define i64 @test_non_dominating_not_eliminated
; DIST:       bb1:
; DIST:         %sum1 = add i64 %runtime_input_0, 42
; DIST:       bb2:
; DIST:         %sum2 = add i64 %runtime_input_0, 42
entry:
  br i1 %cond, label %bb1, label %bb2

bb1:
  %sum1 = add i64 %runtime_input_0, 42
  %use1 = mul i64 %sum1, 2
  br label %merge

bb2:
  %sum2 = add i64 %runtime_input_0, 42
  %use2 = mul i64 %sum2, 3
  br label %merge

merge:
  %phi = phi i64 [ %use1, %bb1 ], [ %use2, %bb2 ]
  ret i64 %phi
}

; Test 5: Distance guard — invariant-operand duplicates separated by many
; instructions in the same BB.  Both operands are function-wide invariants
; (argument + constant), but the result's live range would still be extended.
;
; With MaxDist=0 (CHECK): the duplicate is eliminated (CSE fires).
; With MaxDist=3 (DIST):  the duplicate survives because the two identical
; adds are 6 instructions apart (> 3), so the distance guard blocks CSE.
define i64 @test_distance_guard_invariant_ops(i64 %a) {
; CHECK-LABEL: define i64 @test_distance_guard_invariant_ops
; CHECK:         %add1 = add i64 %a, 100
; CHECK-NOT:     %add2 = add i64 %a, 100
; CHECK:         %res = add i64
; CHECK:         ret i64
;
; DIST-LABEL: define i64 @test_distance_guard_invariant_ops
; DIST:         %add1 = add i64 %a, 100
; DIST:         %add2 = add i64 %a, 100
; DIST:         %res = add i64 {{.*}}, %add2
; DIST:         ret i64
entry:
  %add1 = add i64 %a, 100
  %f1 = mul i64 %add1, 2
  %f2 = mul i64 %f1, 3
  %f3 = mul i64 %f2, 5
  %f4 = mul i64 %f3, 7
  %f5 = mul i64 %f4, 11
  %add2 = add i64 %a, 100
  %res = add i64 %f5, %add2
  ret i64 %res
}
