;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-special-cases-disable-licm -S < %s | FileCheck %s
; ------------------------------------------------
; SpecialCasesDisableLICM : LoopHasInvariantSwitchDispatch
; ------------------------------------------------

; A loop containing many dispatch blocks (each a single icmp against the same
; loop-invariant value + conditional branch) should have LICM disabled.

; CHECK-LABEL: @test_invariant_switch_dispatch(
; CHECK: br i1 %exit_cond, label %exit, label %header, !llvm.loop !0

; CHECK: !0 = distinct !{!0, !1}
; CHECK: !1 = !{!"llvm.licm.disable"}

define spir_kernel void @test_invariant_switch_dispatch(i32 %dispatch_val) {
preheader:
  br label %header

header:
  %counter = phi i32 [ 0, %preheader ], [ %next, %latch ]
  br label %dispatch0

dispatch0:
  %c0 = icmp eq i32 %dispatch_val, 0
  br i1 %c0, label %arm, label %dispatch1

dispatch1:
  %c1 = icmp eq i32 %dispatch_val, 1
  br i1 %c1, label %arm, label %dispatch2

dispatch2:
  %c2 = icmp eq i32 %dispatch_val, 2
  br i1 %c2, label %arm, label %dispatch3

dispatch3:
  %c3 = icmp eq i32 %dispatch_val, 3
  br i1 %c3, label %arm, label %dispatch4

dispatch4:
  %c4 = icmp eq i32 %dispatch_val, 4
  br i1 %c4, label %arm, label %dispatch5

dispatch5:
  %c5 = icmp eq i32 %dispatch_val, 5
  br i1 %c5, label %arm, label %dispatch6

dispatch6:
  %c6 = icmp eq i32 %dispatch_val, 6
  br i1 %c6, label %arm, label %dispatch7

dispatch7:
  %c7 = icmp eq i32 %dispatch_val, 7
  br i1 %c7, label %arm, label %dispatch8

dispatch8:
  %c8 = icmp eq i32 %dispatch_val, 8
  br i1 %c8, label %arm, label %dispatch9

dispatch9:
  %c9 = icmp eq i32 %dispatch_val, 9
  br i1 %c9, label %arm, label %dispatch10

dispatch10:
  %c10 = icmp eq i32 %dispatch_val, 10
  br i1 %c10, label %arm, label %dispatch11

dispatch11:
  %c11 = icmp eq i32 %dispatch_val, 11
  br i1 %c11, label %arm, label %dispatch12

dispatch12:
  %c12 = icmp eq i32 %dispatch_val, 12
  br i1 %c12, label %arm, label %dispatch13

dispatch13:
  %c13 = icmp eq i32 %dispatch_val, 13
  br i1 %c13, label %arm, label %dispatch14

dispatch14:
  %c14 = icmp eq i32 %dispatch_val, 14
  br i1 %c14, label %arm, label %latch

arm:
  br label %latch

latch:
  %next = add i32 %counter, 1
  %exit_cond = icmp eq i32 %next, 100
  br i1 %exit_cond, label %exit, label %header

exit:
  ret void
}

!igc.functions = !{}
