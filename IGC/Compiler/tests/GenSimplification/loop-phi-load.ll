;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-shuffle-simplification -dce -verify -S < %s | FileCheck %s --check-prefixes=CHECK,REJECT
; RUN: igc_opt --opaque-pointers -igc-shuffle-simplification -verify -S < %s | FileCheck %s --check-prefix=REJECT

; Check cleanup with DCE; check rejected cases also without DCE so it cannot
; hide speculative extracts left by a failed match.

; A conditional entry is not a canonical preheader. Extract the initial lanes
; there and replace the loop's vector PHI and packing with scalar PHIs.
; CHECK-LABEL: define float @float_loop(
; CHECK: %initial = load <2 x float>
; CHECK-NEXT: [[INIT0:%.*]] = extractelement <2 x float> %initial, i64 0
; CHECK-NEXT: [[INIT1:%.*]] = extractelement <2 x float> %initial, i64 1
; CHECK-NEXT: br i1 %enter, label %loop, label %exit
; CHECK: loop:
; CHECK-NEXT: [[X0:%.*]] = phi float [ [[INIT0]], %entry ], [ %next0, %loop ]
; CHECK-NEXT: [[X1:%.*]] = phi float [ [[INIT1]], %entry ], [ %next1, %loop ]
; CHECK-NEXT: %next0 = fadd float [[X0]], [[X1]]
; CHECK-NEXT: %next1 = fsub float [[X1]], [[X0]]
; CHECK-NEXT: br i1 %again, label %loop, label %exit
; CHECK: ret float %result
define float @float_loop(ptr %ptr, i1 %enter, i1 %again) {
entry:
  %initial = load <2 x float>, ptr %ptr, align 8
  br i1 %enter, label %loop, label %exit
loop:
  %state = phi <2 x float> [ %initial, %entry ], [ %pack1, %loop ]
  %x0 = extractelement <2 x float> %state, i32 0
  %x1 = extractelement <2 x float> %state, i32 1
  %next0 = fadd float %x0, %x1
  %next1 = fsub float %x1, %x0
  %pack0 = insertelement <2 x float> undef, float %next0, i32 0
  %pack1 = insertelement <2 x float> %pack0, float %next1, i32 1
  br i1 %again, label %loop, label %exit
exit:
  %result = phi float [ 0.0, %entry ], [ %next0, %loop ]
  ret float %result
}

; Reversed incoming order, a separate latch, and a load that dominates the entry
; predecessor without being in it. The last insertion for a lane takes priority.
; CHECK-LABEL: define i32 @integer_loop(
; CHECK: %initial = load <2 x i32>
; CHECK-NEXT: br label %guard
; CHECK: guard:
; CHECK-NEXT: [[I0:%.*]] = extractelement <2 x i32> %initial, i64 0
; CHECK-NEXT: [[I1:%.*]] = extractelement <2 x i32> %initial, i64 1
; CHECK-NEXT: br i1 %enter, label %loop, label %exit
; CHECK: loop:
; CHECK-NEXT: [[Y0:%.*]] = phi i32 [ %next0, %latch ], [ [[I0]], %guard ]
; CHECK-NEXT: [[Y1:%.*]] = phi i32 [ %next1, %latch ], [ [[I1]], %guard ]
; CHECK-NEXT: %next0 = add i32 [[Y0]], [[Y1]]
; CHECK-NEXT: %next1 = xor i32 [[Y1]], [[Y0]]
; CHECK-NEXT: br label %latch
; CHECK: latch:
; CHECK-NEXT: br i1 %again, label %loop, label %exit
; CHECK: ret i32 %result
define i32 @integer_loop(ptr %ptr, i1 %enter, i1 %again) {
entry:
  %initial = load <2 x i32>, ptr %ptr, align 8
  br label %guard
guard:
  br i1 %enter, label %loop, label %exit
loop:
  %state = phi <2 x i32> [ %pack2, %latch ], [ %initial, %guard ]
  %x0 = extractelement <2 x i32> %state, i32 0
  %x1 = extractelement <2 x i32> %state, i32 1
  %next0 = add i32 %x0, %x1
  %next1 = xor i32 %x1, %x0
  br label %latch
latch:
  %pack0 = insertelement <2 x i32> undef, i32 99, i32 0
  %pack1 = insertelement <2 x i32> %pack0, i32 %next1, i32 1
  %pack2 = insertelement <2 x i32> %pack1, i32 %next0, i32 0
  br i1 %again, label %loop, label %exit
exit:
  %result = phi i32 [ 0, %guard ], [ %next0, %latch ]
  ret i32 %result
}

; Reject incomplete backedge packing.
; REJECT-LABEL: define i32 @partial_backedge(
; REJECT: %initial = load <2 x i32>
; REJECT-NEXT: br label %loop
; REJECT: %state = phi <2 x i32>
define i32 @partial_backedge(ptr %ptr, i1 %again) {
entry:
  %initial = load <2 x i32>, ptr %ptr, align 8
  br label %loop
loop:
  %state = phi <2 x i32> [ %initial, %entry ], [ %next, %loop ]
  %x = extractelement <2 x i32> %state, i32 0
  %y = extractelement <2 x i32> %state, i32 1
  %sum = add i32 %x, %y
  %next = insertelement <2 x i32> undef, i32 %sum, i32 0
  br i1 %again, label %loop, label %exit
exit:
  ret i32 %sum
}

; Reject a backedge load: there is no packing to eliminate.
; REJECT-LABEL: define i32 @backedge_load(
; REJECT: %initial = load <2 x i32>
; REJECT-NEXT: br label %loop
; REJECT: %state = phi <2 x i32>
define i32 @backedge_load(ptr %ptr, i1 %again) {
entry:
  %initial = load <2 x i32>, ptr %ptr, align 8
  br label %loop
loop:
  %state = phi <2 x i32> [ %initial, %entry ], [ %next, %loop ]
  %x = extractelement <2 x i32> %state, i32 0
  %next = load volatile <2 x i32>, ptr %ptr, align 8
  br i1 %again, label %loop, label %exit
exit:
  ret i32 %x
}

; Restrict load handling to loop-header PHIs.
; REJECT-LABEL: define i32 @non_loop_phi(
; REJECT: %state = phi <2 x i32>
define i32 @non_loop_phi(ptr %ptr, i32 %x, i1 %cond) {
entry:
  %initial = load <2 x i32>, ptr %ptr, align 8
  br i1 %cond, label %left, label %right
left:
  br label %merge
right:
  %pack = insertelement <2 x i32> undef, i32 %x, i32 0
  %next = insertelement <2 x i32> %pack, i32 %x, i32 1
  br label %merge
merge:
  %state = phi <2 x i32> [ %initial, %left ], [ %next, %right ]
  %result = extractelement <2 x i32> %state, i32 0
  ret i32 %result
}

; Reject out-of-range indices before indexing the lane mask.
; REJECT-LABEL: define i32 @invalid_insert(
; REJECT: %initial = load <2 x i32>
; REJECT-NEXT: br label %loop
; REJECT: %state = phi <2 x i32>
define i32 @invalid_insert(ptr %ptr, i1 %again) {
entry:
  %initial = load <2 x i32>, ptr %ptr, align 8
  br label %loop
loop:
  %state = phi <2 x i32> [ %initial, %entry ], [ %next, %loop ]
  %x = extractelement <2 x i32> %state, i32 0
  %pack = insertelement <2 x i32> undef, i32 %x, i32 0
  %next = insertelement <2 x i32> %pack, i32 %x, i64 4294967296
  br i1 %again, label %loop, label %exit
exit:
  ret i32 %x
}

!igc.functions = !{!0, !1, !2, !3, !4, !5}
!0 = !{ptr @float_loop, !6}
!1 = !{ptr @integer_loop, !6}
!2 = !{ptr @partial_backedge, !6}
!3 = !{ptr @backedge_load, !6}
!4 = !{ptr @non_loop_phi, !6}
!5 = !{ptr @invalid_insert, !6}
!6 = !{!7}
!7 = !{!"function_type", i32 0}
