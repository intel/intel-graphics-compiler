;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -igc-custom-safe-opt -dce -opaque-pointers -S < %s | FileCheck %s
; RUN: igc_opt -igc-custom-safe-opt -dce -opaque-pointers -regkey ForceHoistUDivURem=1 -S < %s | FileCheck %s --check-prefix=CHECK-HOIST
; ------------------------------------------------
;
; Test CSE for udiv and urem in CustomSafeOptPass
; ------------------------------------------------

; Test 1: Normal CSE where the first udiv/urem dominates all other uses
; The first udiv/urem should be kept and all subsequent ones should be replaced
; This test has a simple if/else before the second udiv/urem
define void @test_udiv_urem_cse_dominating(i32 %a, i32 %b, i32 %c, i1 %cond, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4) {
; CHECK-LABEL: @test_udiv_urem_cse_dominating(
; CHECK-DAG:         [[UDIV1:%.*]] = udiv i32 %a, %b
; CHECK-DAG:         [[UREM1:%.*]] = urem i32 %a, %b
; Second udiv/urem with same operands should be CSE'd (replaced by first)
; CHECK:       merge:
; CHECK-NOT:     udiv i32 %a, %b
; CHECK-NOT:     urem i32 %a, %b
; CHECK:         [[RES1:%.*]] = add i32 %merged, [[UDIV1]]
; CHECK-NEXT:    [[RES2:%.*]] = add i32 %merged, [[UREM1]]
; CHECK:         store i32 [[RES1]], ptr %dest3
; CHECK:         store i32 [[RES2]], ptr %dest4
; CHECK:         ret void
entry:
  %udiv1 = udiv i32 %a, %b
  %urem1 = urem i32 %a, %b
  store i32 %udiv1, ptr %dest1
  store i32 %urem1, ptr %dest2
  br i1 %cond, label %path1, label %path2

path1:
  %val1 = add i32 %c, 1
  br label %merge

path2:
  %val2 = add i32 %c, 2
  br label %merge

merge:
  %merged = phi i32 [ %val1, %path1 ], [ %val2, %path2 ]
  ; These should be replaced with %udiv1 and %urem1 since entry dominates merge
  %udiv2 = udiv i32 %a, %b
  %urem2 = urem i32 %a, %b
  %res1 = add i32 %merged, %udiv2
  %res2 = add i32 %merged, %urem2
  store i32 %res1, ptr %dest3
  store i32 %res2, ptr %dest4
  ret void
}

; Test 2: udiv/urem in different branches - neither dominates the other
; First if-else has udiv in one branch, second if-else has udiv in another branch
; Without ForceUDivURemCSE: no CSE should happen (hoisting would be speculative)
; With ForceUDivURemCSE: CSE should happen and udiv/urem hoisted to common ancestor
define void @test_udiv_urem_cse_no_domination(i32 %a, i32 %b, i1 %cond1, i1 %cond2, ptr %dest1, ptr %dest2) {
; CHECK-LABEL: @test_udiv_urem_cse_no_domination(
; Without hoist, both udiv/urem should remain in their respective blocks
; CHECK:       if1:
; CHECK:         [[UDIV1:%.*]] = udiv i32 %a, %b
; CHECK:         [[UREM1:%.*]] = urem i32 %a, %b
; CHECK:       else2:
; CHECK:         [[UDIV2:%.*]] = udiv i32 %a, %b
; CHECK:         [[UREM2:%.*]] = urem i32 %a, %b

; CHECK-HOIST-LABEL: @test_udiv_urem_cse_no_domination(
; With hoist, udiv/urem should be hoisted to entry block (common ancestor)
; CHECK-HOIST:       entry:
; CHECK-HOIST-DAG:         [[UDIV_HOISTED:%.*]] = udiv i32 %a, %b
; CHECK-HOIST-DAG:         [[UREM_HOISTED:%.*]] = urem i32 %a, %b
; CHECK-HOIST:       if1:
; CHECK-HOIST-NOT:     udiv
; CHECK-HOIST-NOT:     urem
; CHECK-HOIST:         [[IF1_VAL1:%.*]] = add i32 [[UDIV_HOISTED]], 10
; CHECK-HOIST:       else2:
; CHECK-HOIST-NOT:     udiv
; CHECK-HOIST-NOT:     urem
; CHECK-HOIST:         [[ELSE2_VAL1:%.*]] = mul i32 [[UDIV_HOISTED]], 2
entry:
  br i1 %cond1, label %if1, label %else1

if1:
  ; udiv/urem in first if branch
  %udiv1 = udiv i32 %a, %b
  %urem1 = urem i32 %a, %b
  %if1_val1 = add i32 %udiv1, 10
  %if1_val2 = add i32 %urem1, 20
  br label %merge1

else1:
  ; no udiv/urem here
  br label %merge1

merge1:
  %merged1_val1 = phi i32 [ %if1_val1, %if1 ], [ 0, %else1 ]
  %merged1_val2 = phi i32 [ %if1_val2, %if1 ], [ 0, %else1 ]
  br i1 %cond2, label %if2, label %else2

if2:
  ; no udiv/urem here
  br label %merge2

else2:
  ; udiv/urem in second else branch - neither dominates the other
  %udiv2 = udiv i32 %a, %b
  %urem2 = urem i32 %a, %b
  %else2_val1 = mul i32 %udiv2, 2
  %else2_val2 = mul i32 %urem2, 3
  br label %merge2

merge2:
  %result1 = phi i32 [ %merged1_val1, %if2 ], [ %else2_val1, %else2 ]
  %result2 = phi i32 [ %merged1_val2, %if2 ], [ %else2_val2, %else2 ]
  store i32 %result1, ptr %dest1
  store i32 %result2, ptr %dest2
  ret void
}
