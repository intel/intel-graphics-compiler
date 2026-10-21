;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt --opaque-pointers %s -igc-constant-coalescing -print-wia-check -disable-output -verify -regkey PrintToConsole=1 2>&1 | FileCheck %s --implicit-check-not="phi <" --implicit-check-not="phi {"
; RUN: igc_opt --opaque-pointers %s -igc-constant-coalescing -print-wia-check -disable-output -verify -regkey PrintToConsole=1,EnableScalarLCSSAInConstantCoalescing=0 2>&1 | FileCheck %s --check-prefix=DISABLED --implicit-check-not=".lcssa"

; Integer, floating-point and pointer live-outs get exit PHIs. The vector and
; aggregate retain their direct outside users. The WIAnalysis dump observes the
; scalar LCSSA output before coalescing removes redundant PHIs.
define float @mixed_liveouts(ptr %buffer, i32 %limit, float %input, <4 x float> %vector) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %next = add i32 %i, 1
  %fp = fadd float %input, 1.000000e+00
  %p = getelementptr float, ptr %buffer, i32 %i
  %v = insertelement <4 x float> %vector, float %fp, i32 1
  %aggregate = insertvalue { i32, float } undef, i32 %next, 0
  store float %fp, ptr %p
  %again = icmp ult i32 %next, %limit
  br i1 %again, label %loop, label %exit

exit:
  %loaded = load float, ptr %p
  %e = extractelement <4 x float> %v, i32 1
  %field = extractvalue { i32, float } %aggregate, 0
  %count = add i32 %next, %field
  %countf = uitofp i32 %count to float
  %a = fadd float %loaded, %fp
  %b = fadd float %a, %e
  %result = fadd float %b, %countf
  ret float %result
}
; CHECK-LABEL: define float @mixed_liveouts(
; CHECK: exit:
; CHECK-DAG: %next.lcssa = phi i32 [ %next, %loop ]
; CHECK-DAG: %p.lcssa = phi ptr [ %p, %loop ]
; CHECK-DAG: %fp.lcssa = phi float [ %fp, %loop ]
; CHECK: %loaded = load float, ptr %p.lcssa
; CHECK-NEXT: %e = extractelement <4 x float> %v, i32 1
; CHECK-NEXT: %field = extractvalue { i32, float } %aggregate, 0
; CHECK-NEXT: %count = add i32 %next.lcssa, %field
; CHECK: %a = fadd float %loaded, %fp.lcssa
; CHECK: ret float %result

; Close the scalar at both loop boundaries, leaving the vector untouched.
define i32 @nested_loops(i32 %limit, <2 x i32> %input) {
entry:
  br label %outer

outer:
  %i = phi i32 [ 0, %entry ], [ %i.next, %outer.latch ]
  br label %inner

inner:
  %j = phi i32 [ 0, %outer ], [ %j.next, %inner ]
  %j.next = add i32 %j, 1
  %scalar = add i32 %i, %j.next
  %vector = insertelement <2 x i32> %input, i32 %scalar, i32 0
  %inner.more = icmp ult i32 %j.next, %limit
  br i1 %inner.more, label %inner, label %inner.exit

inner.exit:
  %i.next = add i32 %i, 1
  %outer.more = icmp ult i32 %i.next, %limit
  br i1 %outer.more, label %outer.latch, label %exit

outer.latch:
  br label %outer

exit:
  %element = extractelement <2 x i32> %vector, i32 0
  %result = add i32 %scalar, %element
  ret i32 %result
}
; CHECK-LABEL: define i32 @nested_loops(
; CHECK: inner.exit:
; CHECK-NEXT: BB:
; CHECK-NEXT: %scalar.lcssa = phi i32 [ %scalar, %inner ]
; CHECK: exit:
; CHECK-NEXT: BB:
; CHECK-NEXT: %scalar.lcssa.lcssa = phi i32 [ %scalar.lcssa, %inner.exit ]
; CHECK-NEXT: %element = extractelement <2 x i32> %vector, i32 0
; CHECK-NEXT: %result = add i32 %scalar.lcssa.lcssa, %element
; CHECK-NEXT: ret i32 %result

; Two distinct exits need PHIs and SSA repair at their later join.
define i32 @multiple_exits(i32 %limit, i32 %stop) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %latch ]
  %value = add i32 %i, 10
  %done = icmp eq i32 %i, %stop
  br i1 %done, label %exit.left, label %latch

latch:
  %next = add i32 %i, 1
  %again = icmp ult i32 %next, %limit
  br i1 %again, label %loop, label %exit.right

exit.left:
  br label %join

exit.right:
  br label %join

join:
  %result = add i32 %value, 1
  ret i32 %result
}
; CHECK-LABEL: define i32 @multiple_exits(
; CHECK: exit.left:
; CHECK-NEXT: BB:
; CHECK-NEXT: [[LEFT:%[^ ]+]] = phi i32 [ %value, %loop ]
; CHECK-NEXT: br label %join
; CHECK: exit.right:
; CHECK-NEXT: BB:
; CHECK-NEXT: [[RIGHT:%[^ ]+]] = phi i32 [ %value, %latch ]
; CHECK-NEXT: br label %join
; CHECK: join:
; CHECK-NEXT: BB:
; CHECK-NEXT: [[MERGED:%[^ ]+]] = phi i32 [ [[RIGHT]], %exit.right ], [ [[LEFT]], %exit.left ]
; CHECK-NEXT: %result = add i32 [[MERGED]], 1
; CHECK-NEXT: ret i32 %result

; A PHI incoming use on the exiting edge is already closed.
define i32 @existing_exit_phi(i32 %limit) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %next = add i32 %i, 1
  %again = icmp ult i32 %next, %limit
  br i1 %again, label %loop, label %exit

exit:
  %closed = phi i32 [ %next, %loop ]
  ret i32 %closed
}
; CHECK-LABEL: define i32 @existing_exit_phi(
; CHECK-NOT: .lcssa
; CHECK: exit:
; CHECK-NEXT: BB:
; CHECK-NEXT: %closed = phi i32 [ %next, %loop ]
; CHECK-NEXT: ret i32 %closed

; DISABLED-LABEL: define float @mixed_liveouts(
; DISABLED: %loaded = load float, ptr %p
; DISABLED: %count = add i32 %next, %field
; DISABLED-LABEL: define i32 @nested_loops(
; DISABLED: %result = add i32 %scalar, %element
; DISABLED-LABEL: define i32 @multiple_exits(
; DISABLED: %result = add i32 %value, 1
; DISABLED-LABEL: define i32 @existing_exit_phi(
; DISABLED: %closed = phi i32 [ %next, %loop ]

!igc.functions = !{!0, !1, !2, !3}
!0 = !{ptr @mixed_liveouts, !5}
!1 = !{ptr @nested_loops, !5}
!2 = !{ptr @multiple_exits, !5}
!3 = !{ptr @existing_exit_phi, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
