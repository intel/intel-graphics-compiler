;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; IGC-17192: a value defined in the accumulator's merge block (DstFalse, here
; %v in %m1) was correctly cloned into DstTrue (%t1) by
; cloneAccumulatorTriangleTailBB(), but the *terminator* clone produced by
; mergePathIntoBlock() only consulted its own local VMap, not the
; CumulativeVMap holding that clone. RemapInstruction (with
; RF_IgnoreMissingLocals) silently left the terminator's operand pointing at
; the stale, non-dominating original value, which repairSSAAfterMerge() then
; replaced with poison. @f must return %a + 1 on every path.
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

define i32 @f(i1 %c, i32 %a) {
; CHECK-LABEL: define i32 @f(
; CHECK-SAME: i1 [[C:%.*]], i32 [[A:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    br i1 [[C]], label %[[T1:.*]], label %[[M1:.*]]
; CHECK:       [[T1]]:
; CHECK-NEXT:    [[TMP0:%.*]] = add i32 [[A]], 1
; CHECK-NEXT:    ret i32 [[TMP0]]
; CHECK:       [[M1]]:
; CHECK-NEXT:    [[V:%.*]] = add i32 [[A]], 1
; CHECK-NEXT:    ret i32 [[V]]
;
entry:
  br i1 %c, label %t1, label %m1

t1:
  br label %m1

m1:                                    ; join of diamond #1
  %v = add i32 %a, 1
  br i1 %c, label %t2, label %m2

t2:
  br label %m2

m2:                                    ; join of diamond #2
  ret i32 %v
}
