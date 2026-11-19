;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -licm-legacy-wrapped -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -domtree -licm-legacy-wrapped -domtree -debug-pass=Executions -disable-output < %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PM

; LICM runs on its own FunctionAnalysisManager, and its loop adaptor runs LoopSimplify first. Here the loop has no
; preheader, so the wrapper changes the CFG (inserts header.preheader) without updating the legacy pass manager's
; analyses. It must therefore not declare them preserved: a later legacy user of the DominatorTree has to get a
; freshly computed one instead of the tree built before LICM.

; CHECK-LABEL: @no_preheader(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %header.preheader, label %other
; CHECK:       header.preheader:
; CHECK-NEXT:    %inv = mul i32 %a, %b
; CHECK:       header:
; CHECK-NOT:     mul i32
; CHECK:         ret void

; PM:      Executing Pass 'Dominator Tree Construction' on Function 'no_preheader'
; PM:      Executing Pass 'LegacyWrappedLICM' on Function 'no_preheader'
; PM:      Executing Pass 'Dominator Tree Construction' on Function 'no_preheader'

define void @no_preheader(i32 %n, i32 %a, i32 %b, i1 %c, ptr addrspace(1) %p) {
entry:
  br i1 %c, label %header, label %other

other:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ 0, %other ], [ %inc, %header ]
  %inv = mul i32 %a, %b
  %g = getelementptr i32, ptr addrspace(1) %p, i32 %i
  store i32 %inv, ptr addrspace(1) %g, align 4
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %header, label %exit

exit:
  ret void
}
