;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Test that the pass leaves subgroup shuffle down unchanged: its in-range
; index depends on the lane id, so the emitter bounds it, as checked in
; ocloc_tests/optimizations/shuffle-down-and-up-bounds-checks.ll.
; RUN: igc_opt %s -S  --igc-register-access-bounds-checks | FileCheck %s
;

define i32 @f0(i32 %data, i32 %next, i32 %delta) #1 {
Label-0:
  %ret = call i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32 %data, i32 %next, i32 %delta)
  ret i32 %ret
}
; CHECK-LABEL: define i32 @f0
; CHECK-NEXT:  Label-0:
; CHECK-NEXT:    %ret = call i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32 %data, i32 %next, i32 %delta)
; CHECK-NEXT:    ret i32 %ret
declare i32 @llvm.genx.GenISA.simdShuffleDown.i32(i32, i32, i32)



