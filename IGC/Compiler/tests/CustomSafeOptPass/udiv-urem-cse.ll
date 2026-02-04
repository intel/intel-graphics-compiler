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
; CHECK-LABEL: @test_udiv_urem_cse_dominating(
; CHECK-DAG: [[UDIV1:%.*]] = udiv i32 %a, %b
; CHECK-DAG: [[UREM1:%.*]] = urem i32 %a, %b
; CHECK: merge:
; CHECK-NOT: udiv i32 %a, %b
; CHECK-NOT: urem i32 %a, %b
; CHECK: [[RES1:%.*]] = add i32 %merged, [[UDIV1]]
; CHECK: [[RES2:%.*]] = add i32 %merged, [[UREM1]]
; CHECK: store i32 [[RES1]], ptr %dest3
; CHECK: store i32 [[RES2]], ptr %dest4
; CHECK: ret void
define void @test_udiv_urem_cse_dominating(i32 %a, i32 %b, i32 %c, i1 %cond, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4) {
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
; CHECK-LABEL: @test_udiv_urem_cse_no_domination(
; CHECK: if1:
; CHECK: [[UDIV1:%.*]] = udiv i32 %a, %b
; CHECK: [[UREM1:%.*]] = urem i32 %a, %b
; CHECK: else2:
; CHECK: [[UDIV2:%.*]] = udiv i32 %a, %b
; CHECK: [[UREM2:%.*]] = urem i32 %a, %b

; CHECK-HOIST-LABEL: @test_udiv_urem_cse_no_domination(
; CHECK-HOIST: entry:
; CHECK-HOIST-DAG: [[UDIV_HOISTED:%.*]] = udiv i32 %a, %b
; CHECK-HOIST-DAG: [[UREM_HOISTED:%.*]] = urem i32 %a, %b
; CHECK-HOIST: if1:
; CHECK-HOIST-NOT: udiv i32 %a, %b
; CHECK-HOIST-NOT: urem i32 %a, %b
; CHECK-HOIST: add i32 [[UDIV_HOISTED]], 10
; CHECK-HOIST: add i32 [[UREM_HOISTED]], 20
; CHECK-HOIST: else2:
; CHECK-HOIST-NOT: udiv i32 %a, %b
; CHECK-HOIST-NOT: urem i32 %a, %b
; CHECK-HOIST: mul i32 [[UDIV_HOISTED]], 2
; CHECK-HOIST: mul i32 [[UREM_HOISTED]], 3
define void @test_udiv_urem_cse_no_domination(i32 %a, i32 %c, i1 %cond1, i1 %cond2, ptr %dest1, ptr %dest2) {
entry:
  %b = shl i32 1, %c ; guarantees non-zero for divisor
  br i1 %cond1, label %if1, label %else1

if1:
  %udiv1 = udiv i32 %a, %b
  %urem1 = urem i32 %a, %b
  %if1_val1 = add i32 %udiv1, 10
  %if1_val2 = add i32 %urem1, 20
  br label %merge1

else1:
  br label %merge1

merge1:
  %merged1_val1 = phi i32 [ %if1_val1, %if1 ], [ 0, %else1 ]
  %merged1_val2 = phi i32 [ %if1_val2, %if1 ], [ 0, %else1 ]
  br i1 %cond2, label %if2, label %else2

if2:
  br label %merge2

else2:
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

; Test 3: Normal CSE where the first udiv/urem dominates all other uses, but mismatch of exact keyword results in no CSE for udiv
; However, urem can be CSE'd, and resulting second udiv implementation after PreCompiledFuncImport may have code that can be DCE'd if it was only relevant for urem calculation
; This test has a simple if/else before the second udiv/urem
; CHECK-LABEL: @test_udiv_urem_no_cse_dominating(
; CHECK: [[UDIV1:%.*]] = udiv exact i32 %a, %b
; CHECK: [[UREM1:%.*]] = urem i32 %a, %b
; CHECK: store i32 [[UDIV1]], ptr %dest1
; CHECK: store i32 [[UREM1]], ptr %dest2
; CHECK: merge:
; CHECK: [[UDIV2:%.*]] = udiv i32 %a, %b
; CHECK-NOT: urem i32 %a, %b
; CHECK: [[RES1:%.*]] = add i32 %merged, [[UDIV2]]
; CHECK: [[RES2:%.*]] = add i32 %merged, [[UREM1]]
; CHECK: store i32 [[RES1]], ptr %dest3
; CHECK: store i32 [[RES2]], ptr %dest4
; CHECK: ret void
define void @test_udiv_urem_no_cse_dominating(i32 %a, i32 %b, i32 %c, i1 %cond, ptr %dest1, ptr %dest2, ptr %dest3, ptr %dest4) {
entry:
  %udiv1 = udiv exact i32 %a, %b
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
  ; This should not be replaced with %udiv1 since it does not have exact, even if entry dominates merge
  %udiv2 = udiv i32 %a, %b
  %urem2 = urem i32 %a, %b
  %res1 = add i32 %merged, %udiv2
  %res2 = add i32 %merged, %urem2
  store i32 %res1, ptr %dest3
  store i32 %res2, ptr %dest4
  ret void
}

; Test 4: udiv/urem in different branches - neither dominates the other
; With ForceUDivURemCSE: CSE should not happen because divisor cannot be proven to be non-zero
; CHECK-HOIST-LABEL: @test_udiv_urem_cse_not_non_zero_no_domination(
; CHECK-HOIST: if1:
; CHECK-HOIST: [[UDIV1:%.*]] = udiv i32 %a, %b
; CHECK-HOIST: [[UREM1:%.*]] = urem i32 %a, %b
; CHECK-HOIST: else2:
; CHECK-HOIST: [[UDIV2:%.*]] = udiv i32 %a, %b
; CHECK-HOIST: [[UREM2:%.*]] = urem i32 %a, %b
define void @test_udiv_urem_cse_not_non_zero_no_domination(i32 %a, i32 %b, i1 %cond1, i1 %cond2, ptr %dest1, ptr %dest2) {
entry:
  br i1 %cond1, label %if1, label %else1

if1:
  %udiv1 = udiv i32 %a, %b
  %urem1 = urem i32 %a, %b
  %if1_val1 = add i32 %udiv1, 10
  %if1_val2 = add i32 %urem1, 20
  br label %merge1

else1:
  br label %merge1

merge1:
  %merged1_val1 = phi i32 [ %if1_val1, %if1 ], [ 0, %else1 ]
  %merged1_val2 = phi i32 [ %if1_val2, %if1 ], [ 0, %else1 ]
  br i1 %cond2, label %if2, label %else2

if2:
  br label %merge2

else2:
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

; Test 5: udiv/urem in different branches - neither dominates the other
; First if-else has udiv in one branch, second if-else has udiv in another branch
; With ForceUDivURemCSE: no CSE should happen udiv/urem hoisted to common ancestor
; This is even though urem is allowed to be hoisted, the hoist block will not have an accompanying udiv
; This may result in a third precompiled function import to happen, so rather keep urems together with accompanying udiv
define void @test_udiv_urem_cse_all_or_nothing_no_domination(i32 %a, i32 %c, i1 %cond1, i1 %cond2, ptr %dest1, ptr %dest2) {
; CHECK-HOIST-LABEL: @test_udiv_urem_cse_all_or_nothing_no_domination(
; CHECK-HOIST: if1:
; CHECK-HOIST: [[UDIV1:%.*]] = udiv exact i32 %a, %b
; CHECK-HOIST: [[UREM1:%.*]] = urem i32 %a, %b
; CHECK-HOIST: else2:
; CHECK-HOIST: [[UDIV2:%.*]] = udiv i32 %a, %b
; CHECK-HOIST: [[UREM2:%.*]] = urem i32 %a, %b
entry:
  %b = shl i32 1, %c
  br i1 %cond1, label %if1, label %else1

if1:
  %udiv1 = udiv exact i32 %a, %b
  %urem1 = urem i32 %a, %b
  %if1_val1 = add i32 %udiv1, 10
  %if1_val2 = add i32 %urem1, 20
  br label %merge1

else1:
  br label %merge1

merge1:
  %merged1_val1 = phi i32 [ %if1_val1, %if1 ], [ 0, %else1 ]
  %merged1_val2 = phi i32 [ %if1_val2, %if1 ], [ 0, %else1 ]
  br i1 %cond2, label %if2, label %else2

if2:
  br label %merge2

else2:
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