;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-split-exiting-latch-backedge -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -igc-split-exiting-latch-backedge -loop-rotate-legacy-wrapped -S < %s | FileCheck %s --check-prefix=ROT
; RUN: igc_opt --opaque-pointers -loop-rotate-legacy-wrapped -S < %s | FileCheck %s --check-prefix=NOSPLIT
; ------------------------------------------------
; SplitExitingLatchBackedge
; ------------------------------------------------

; Header and latch both exit: only the latch->header back-edge is split, the two critical exit edges are not.
define void @top_and_bottom(i32 %n, i32 %m, ptr %p) {
; CHECK-LABEL: @top_and_bottom(
; CHECK:       header:
; CHECK-NEXT:    [[I:%.*]] = phi i32 [ 0, %entry ], [ [[INC:%.*]], %latch.header_crit_edge ]
; CHECK:         br i1 {{%.*}}, label %latch, label %exit
; CHECK:       latch:
; CHECK:         br i1 {{%.*}}, label %latch.header_crit_edge, label %exit
; CHECK:       latch.header_crit_edge:
; CHECK-NEXT:    br label %header
; CHECK:       exit:
; CHECK-NEXT:    ret void
;
; After the split, LoopRotate moves the header's test into a guard and the loop gets a new preheader.
; ROT-LABEL: @top_and_bottom(
; ROT:       latch.lr.ph:
; Without the split, LoopRotate treats the loop as rotated already.
; NOSPLIT-LABEL: @top_and_bottom(
; NOSPLIT-NOT: .lr.ph:
; NOSPLIT:     ret void
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %inc, %latch ]
  %c0 = icmp slt i32 %i, %n
  br i1 %c0, label %latch, label %exit

latch:
  store i32 %i, ptr %p, align 4
  %inc = add i32 %i, 1
  %c1 = icmp slt i32 %inc, %m
  br i1 %c1, label %header, label %exit

exit:
  ret void
}

; Only the latch exits: the loop is tested at the bottom already, nothing to split.
define void @bottom_tested(i32 %n, ptr %p) {
; CHECK-LABEL: @bottom_tested(
; CHECK-NOT:   _crit_edge
; CHECK:       ret void
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %inc, %latch ]
  store i32 %i, ptr %p, align 4
  br label %latch

latch:
  %inc = add i32 %i, 1
  %c = icmp slt i32 %inc, %n
  br i1 %c, label %header, label %exit

exit:
  ret void
}

; Only the header exits and the latch is unconditional: LoopRotate handles it without a split.
define void @top_tested(i32 %n, ptr %p) {
; CHECK-LABEL: @top_tested(
; CHECK-NOT:   _crit_edge
; CHECK:       ret void
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %inc, %latch ]
  %c = icmp slt i32 %i, %n
  br i1 %c, label %latch, label %exit

latch:
  store i32 %i, ptr %p, align 4
  %inc = add i32 %i, 1
  br label %header

exit:
  ret void
}

; Single-block loop: header and latch are the same block, nothing to split.
define void @single_block(i32 %n, ptr %p) {
; CHECK-LABEL: @single_block(
; CHECK-NOT:   _crit_edge
; CHECK:       ret void
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  store i32 %i, ptr %p, align 4
  %inc = add i32 %i, 1
  %c = icmp slt i32 %inc, %n
  br i1 %c, label %loop, label %exit

exit:
  ret void
}
