;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -igc-code-sinking --regkey CodeSinkingMinSize=0 -S %s | FileCheck %s

; Regression test: a value defined in a reachable block whose only uses live in
; a block that is unreachable from the entry (dead predecessor chain) must not
; crash code sinking. findLowestSinkTarget() used to call
; DominatorTree::findNearestCommonDominator()/getNode() on the unreachable use
; block, which is absent from the dominator tree, dereferencing a null
; DomTreeNode.
; Expected behavior after the fix: the instruction is left in place (its only
; uses are in dead code, so there is nothing live to sink towards) and the pass
; does not crash.

; CHECK-LABEL: entry:
; CHECK-NEXT:    [[V:%.*]] = add i32 %x, 1
; CHECK-NEXT:    br label %body

define void @entry(i32 %x) {
entry:
  %v = add i32 %x, 1
  br label %body

body:                                             ; preds = %entry
  ret void

dead:                                             ; No predecessors! -> unreachable
  %a = add i32 %v, 2
  %b = add i32 %v, 3
  ret void
}
