;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-promote-phi-to-source-width -S < %s | FileCheck %s
; ------------------------------------------------
; PromotePhiToSourceWidth - PHI result with more than one use
; ------------------------------------------------
;
; The other positive tests feed the PHI into a single store. These check that
; replaceAllUsesWith rewires *every* use of the PHI to the single sunk narrowing,
; both inside the merge block and in a dominated successor.

; Two uses in the merge block: both rewired to the one sunk trunc.
define void @phi_multiuse_same_block(i1 %c, i32 %a, ptr %p, ptr %q) {
; CHECK-LABEL: @phi_multiuse_same_block(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    store i16 [[T]], ptr %p
; CHECK-NEXT:    store i16 [[T]], ptr %q
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 0, %B ]
  store i16 %phi, ptr %p
  store i16 %phi, ptr %q
  ret void
}

; Use in a successor block: the sunk trunc in the merge block dominates it, so
; the cross-block use is rewired without a dominance violation.
define void @phi_use_in_successor(i1 %c, i32 %a, ptr %p) {
; CHECK-LABEL: @phi_use_in_successor(
; CHECK:       M:
; CHECK-NEXT:    [[W:%.*]] = phi i32 [ %a, %A ], [ 0, %B ]
; CHECK-NEXT:    [[T:%.*]] = trunc i32 [[W]] to i16
; CHECK-NEXT:    br label %N
; CHECK:       N:
; CHECK-NEXT:    store i16 [[T]], ptr %p
entry:
  br i1 %c, label %A, label %B
A:
  %t = trunc i32 %a to i16
  br label %M
B:
  br label %M
M:
  %phi = phi i16 [ %t, %A ], [ 0, %B ]
  br label %N
N:
  store i16 %phi, ptr %p
  ret void
}
