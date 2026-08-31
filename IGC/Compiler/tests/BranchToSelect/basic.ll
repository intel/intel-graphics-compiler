;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --typed-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-branch-to-select -S < %s 2>&1 | FileCheck %s

; Triangle: one private speculated successor, one direct edge to the merge. The
; successor's speculatable instruction is hoisted and the PHI becomes a select on
; the branch condition.
define i32 @test_triangle(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_triangle(
; CHECK:         %v = add i32 %a, %b
; CHECK:         select i1 %c, i32 %v, i32 %a
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %v = add i32 %a, %b
  br label %merge

merge:                                            ; preds = %entry, %then
  %r = phi i32 [ %v, %then ], [ %a, %entry ]
  ret i32 %r
}

; Diamond: both successors are private and meet at a common merge.
define i32 @test_diamond(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_diamond(
; CHECK:         %vt = add i32 %a, 1
; CHECK:         %vf = add i32 %b, 2
; CHECK:         select i1 %c, i32 %vt, i32 %vf
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %t, label %f

t:                                                ; preds = %entry
  %vt = add i32 %a, 1
  br label %merge

f:                                                ; preds = %entry
  %vf = add i32 %b, 2
  br label %merge

merge:                                            ; preds = %t, %f
  %r = phi i32 [ %vt, %t ], [ %vf, %f ]
  ret i32 %r
}

; Degenerate PHI in a private successor. A single-predecessor block can only
; carry single-incoming ("LCSSA-style") PHIs, which are semantically just their
; incoming value. The pass folds such a PHI to that value (here %p -> %a, so %v
; becomes add %a, %b) instead of bailing, and still linearizes the triangle.
define i32 @test_lcssa_phi(i1 %c, i32 %a, i32 %b) {
; CHECK-LABEL: define i32 @test_lcssa_phi(
; The degenerate PHI is folded: %v reads %a directly, not %p.
; CHECK:         %v = add i32 %a, %b
; CHECK-NOT:     %p
; CHECK:         select i1 %c, i32 %v, i32 %a
; CHECK-NOT:     br i1
entry:
  br i1 %c, label %then, label %merge

then:                                             ; preds = %entry
  %p = phi i32 [ %a, %entry ]
  %v = add i32 %p, %b
  br label %merge

merge:                                            ; preds = %entry, %then
  %r = phi i32 [ %v, %then ], [ %a, %entry ]
  ret i32 %r
}

; Shared landing pad: a short-circuit chain whose early-out edges have been
; funneled by SimplifyCFG into one empty pad block reached from several
; predecessors (%entry and %cont). The pass must "peel" each predecessor off the
; pad -- routing it straight to the merge via a select -- rather than bail. The
; region fully linearizes: no conditional branches survive and the pad is gone.
define i32 @test_shared_pad(i1 %c0, i1 %c1, i32 %val) {
; CHECK-LABEL: define i32 @test_shared_pad(
; CHECK:         select i1 %c1, i32 0, i32 %val
; CHECK:         select i1 %c0,
; CHECK-NOT:     br i1
; CHECK-NOT:   pad:
entry:
  br i1 %c0, label %cont, label %pad

cont:                                             ; preds = %entry
  br i1 %c1, label %pad, label %inb

inb:                                              ; preds = %cont
  br label %merge

pad:                                              ; preds = %entry, %cont
  br label %merge

merge:                                            ; preds = %pad, %inb
  %r = phi i32 [ 0, %pad ], [ %val, %inb ]
  ret i32 %r
}

; Negative: a shared block with a real instruction is NOT a pass-through. Hoisting
; it into one predecessor would wrongly run it for the pad's other predecessors,
; so the pass must leave the region untouched.
define i32 @test_shared_pad_not_empty(i1 %c0, i1 %c1, i32 %val, i32 %x) {
; CHECK-LABEL: define i32 @test_shared_pad_not_empty(
; CHECK:         br i1 %c1
; CHECK:         %pv = add i32 %x, 7
; CHECK:         phi i32
entry:
  br i1 %c0, label %cont, label %pad

cont:                                             ; preds = %entry
  br i1 %c1, label %pad, label %inb

inb:                                              ; preds = %cont
  br label %merge

pad:                                              ; preds = %entry, %cont
  %pv = add i32 %x, 7
  br label %merge

merge:                                            ; preds = %pad, %inb
  %r = phi i32 [ %pv, %pad ], [ %val, %inb ]
  ret i32 %r
}

; Shared landing pad holding only PHIs. The value the pad feeds the merge is one of
; its own PHIs, so peeling %cont off the pad must select the value that PHI carries
; along %cont's edge (%v) -- not %pv, which is not available in %cont. Peeling drops
; the pad to a single predecessor, after which it folds as a private successor too,
; so the region fully linearizes.
define i32 @test_shared_pad_phi_only(i1 %c0, i1 %c1, i32 %val, i32 %x) {
; CHECK-LABEL: define i32 @test_shared_pad_phi_only(
; CHECK:         %v = add i32 %x, 7
; CHECK:         select i1 %c1, i32 %val, i32 %v
; CHECK:         select i1 %c0,
; CHECK-NOT:     br i1
; CHECK-NOT:   pad:
entry:
  br i1 %c0, label %cont, label %pad

cont:                                             ; preds = %entry
  %v = add i32 %x, 7
  br i1 %c1, label %inb, label %pad

inb:                                              ; preds = %cont
  br label %merge

pad:                                              ; preds = %entry, %cont
  %pv = phi i32 [ 0, %entry ], [ %v, %cont ]
  br label %merge

merge:                                            ; preds = %pad, %inb
  %r = phi i32 [ %pv, %pad ], [ %val, %inb ]
  ret i32 %r
}

; Negative: PHIs being allowed in a shared pad must not make the pad's other
; instructions acceptable. %pw still has to run for the pad's remaining predecessor,
; so it can only be cloned into %cont, not moved -- the pass leaves the region alone.
define i32 @test_shared_pad_phi_and_inst(i1 %c0, i1 %c1, i32 %val, i32 %x) {
; CHECK-LABEL: define i32 @test_shared_pad_phi_and_inst(
; CHECK:         br i1 %c1
; CHECK:         %pw = add i32 %pv, 3
; CHECK:         phi i32
entry:
  br i1 %c0, label %cont, label %pad

cont:                                             ; preds = %entry
  %v = add i32 %x, 7
  br i1 %c1, label %inb, label %pad

inb:                                              ; preds = %cont
  br label %merge

pad:                                              ; preds = %entry, %cont
  %pv = phi i32 [ 0, %entry ], [ %v, %cont ]
  %pw = add i32 %pv, 3
  br label %merge

merge:                                            ; preds = %pad, %inb
  %r = phi i32 [ %pw, %pad ], [ %val, %inb ]
  ret i32 %r
}
