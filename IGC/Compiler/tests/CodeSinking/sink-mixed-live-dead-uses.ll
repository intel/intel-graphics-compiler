;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -igc-code-sinking --regkey CodeSinkingMinSize=0 -S %s | FileCheck %s

; Mixed case: %v is defined in the reachable entry block and used by BOTH a live
; block (live.use, reachable) and a dead block (dead.use, unreachable from the
; entry). Code sinking must not crash on the unreachable use, and it must still
; sink %v towards the remaining live use (into live.use).

; CHECK-LABEL: entry:
; CHECK-NEXT:    br i1 %cond, label %live.use, label %other

; CHECK-LABEL: live.use:
; CHECK-NEXT:    [[V:%.*]] = add i32 %x, 1
; CHECK-NEXT:    add i32 [[V]], 2
; CHECK-NEXT:    ret void

define void @entry(i1 %cond, i32 %x) {
entry:
  %v = add i32 %x, 1
  br i1 %cond, label %live.use, label %other

live.use:                                         ; preds = %entry
  %u = add i32 %v, 2
  ret void

other:                                            ; preds = %entry
  ret void

dead.exit:                                        ; No predecessors! -> unreachable
  br label %dead.use

dead.use:                                         ; preds = %dead.exit
  %d = add i32 %v, 3
  ret void
}
