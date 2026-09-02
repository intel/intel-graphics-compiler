;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
;
; RUN: igc_opt --opaque-pointers -simplifycfg -bonus-inst-threshold=1 -S < %s | FileCheck %s --check-prefix=T1
;
; RUN: igc_opt --opaque-pointers -simplifycfg -bonus-inst-threshold=2 -S < %s | FileCheck %s --check-prefix=T2

; Tests LLVM's BonusInstThreshold, which IGC's SimplifyCFGBonusInstThreshold
; regkey raises. From the rank-sort inner loop of a Lumen screen-probe shader:
;
;   if (key < ref || (key == ref && i > tid)) rank++;
;
; Both branches share %inc, so FoldBranchToCommonDest() should merge them. It
; refuses: cloning %tie into %loop costs two bonus instructions, %eq and %gt
; (the condition %and is not counted), against LLVM's default of 1. Both sides
; are pinned so a cost-model change cannot silently move the boundary.

define spir_kernel void @rank(float %ref, i32 %thr, ptr addrspace(3) %p, ptr addrspace(1) %out) {
entry:
  br label %loop

loop:
  %cnt = phi i32 [ 0, %entry ], [ %cnt.next, %latch ]
  %i   = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  %gep = getelementptr float, ptr addrspace(3) %p, i32 %i
  %key = load float, ptr addrspace(3) %gep, align 8
  %lt  = fcmp fast olt float %key, %ref
  br i1 %lt, label %inc, label %tie

tie:
  %eq  = fcmp fast oeq float %key, %ref
  %gt  = icmp ugt i32 %i, %thr
  %and = and i1 %gt, %eq
  br i1 %and, label %inc, label %latch

inc:
  %cnt.inc = add i32 %cnt, 1
  br label %latch

latch:
  %cnt.next = phi i32 [ %cnt.inc, %inc ], [ %cnt, %tie ]
  %i.next = add i32 %i, 1
  %done = icmp slt i32 %i.next, 32
  br i1 %done, label %loop, label %exit

exit:
  store i32 %cnt.next, ptr addrspace(1) %out, align 4
  ret void
}

; Two bonus instructions against a budget of one: no fold.
;
; T1-LABEL: define spir_kernel void @rank
; T1:         br i1 %lt, label %inc, label %tie
; T1:         br i1 %and, label %inc, label %latch

; Folded, then speculated: the loop body is one block, branching only on the
; backedge.
;
; T2-LABEL: define spir_kernel void @rank
; T2:         %[[OR:.*]] = select i1 %lt, i1 true, i1 %and
; T2-NEXT:    %cnt.inc = add i32 %cnt, 1
; T2-NEXT:    %cnt.next = select i1 %[[OR]], i32 %cnt.inc, i32 %cnt
; T2-NEXT:    %i.next = add i32 %i, 1
; T2-NEXT:    %done = icmp slt i32 %i.next, 32
; T2-NEXT:    br i1 %done, label %loop, label %exit

!igc.functions = !{!0}

!0 = !{ptr @rank, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
