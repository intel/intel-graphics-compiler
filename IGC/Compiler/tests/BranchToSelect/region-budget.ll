;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A chain of two diamonds where the first's merge block is the second's header.
; After the first diamond folds, the pass merges the merge block into the
; predecessor (an in-pass SimplifyCFG step), which hands the predecessor the second
; diamond's branch and exposes it as a fold rooted at the same block. Each fold
; accumulates a cumulative cost into that linearized region, bounded by
; BranchToSelectMaxRegionCost. Each arm here costs 1, so each diamond costs 2.
;
; LOOSE budget: both diamonds fold -- two selects, no conditional branch, and the
; whole region collapses to one block (the in-pass merge exposed the second fold).
; TIGHT budget (=2): the first diamond fits and folds, but adding the second would
; push the region's cumulative cost to 4 > 2, so it is refused -- its branch and
; merge PHI survive. Confirms the cumulative region gate bounds a cascade even when
; each individual fold is well within the per-arm budget.

; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedCost=40,BranchToSelectMaxRegionCost=100 -S < %s 2>&1 | FileCheck %s --check-prefix=LOOSE
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedCost=40,BranchToSelectMaxRegionCost=100 -S < %s 2>&1 | FileCheck %s --check-prefix=LOOSE
; RUN: igc_opt --typed-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedCost=40,BranchToSelectMaxRegionCost=2 -S < %s 2>&1 | FileCheck %s --check-prefix=TIGHT
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -regkey BranchToSelectMaxSpeculatedCost=40,BranchToSelectMaxRegionCost=2 -S < %s 2>&1 | FileCheck %s --check-prefix=TIGHT

define i32 @test_region_budget(i1 %c1, i1 %c2, i32 %a, i32 %b) {
; LOOSE-LABEL: define i32 @test_region_budget(
; LOOSE:         select i1 %c1
; LOOSE:         select i1 %c2
; LOOSE-NOT:     br i1
;
; TIGHT-LABEL: define i32 @test_region_budget(
; TIGHT:         select i1 %c1
; TIGHT:         br i1 %c2
; TIGHT-NOT:     select i1 %c2
entry:
  br i1 %c1, label %t1, label %f1

t1:                                               ; preds = %entry
  %v1t = add i32 %a, 1
  br label %mid

f1:                                               ; preds = %entry
  %v1f = add i32 %b, 2
  br label %mid

mid:                                              ; preds = %t1, %f1
  %m = phi i32 [ %v1t, %t1 ], [ %v1f, %f1 ]
  br i1 %c2, label %t2, label %f2

t2:                                               ; preds = %mid
  %v2t = add i32 %m, 3
  br label %exit

f2:                                               ; preds = %mid
  %v2f = add i32 %m, 4
  br label %exit

exit:                                             ; preds = %t2, %f2
  %r = phi i32 [ %v2t, %t2 ], [ %v2f, %f2 ]
  ret i32 %r
}
