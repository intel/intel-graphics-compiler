;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt --opaque-pointers -enable-subroutine --regkey SubroutineThreshold=1 \
; RUN:   --regkey SubroutineInlinerThreshold=1 --regkey FunctionControl=0 --regkey SelectiveFunctionControl=0 \
; RUN:   -call-merger-pass -S < %s | FileCheck %s

; Check that CallMerger correctly updates PHI nodes in the successor block.

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK:       bb1:
; CHECK-NEXT:    [[X1:%.*]] = fadd float %a, 1.000000e+00
; CHECK-NEXT:    br label %mergedCallsBB
; CHECK:       bb2:
; CHECK-NEXT:    [[X2:%.*]] = fmul float %b, 2.000000e+00
; CHECK-NEXT:    br label %mergedCallsBB
; CHECK:       mergedCallsBB:
; CHECK-NEXT:    [[ARG:%.*]] = phi float [ [[X1]], %bb1 ], [ [[X2]], %bb2 ]
; CHECK-NEXT:    [[VAL:%.*]] = phi float [ [[X1]], %bb1 ], [ [[X2]], %bb2 ]
; CHECK-NEXT:    call spir_func void @ext(float [[ARG]])
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    [[SUM:%.*]] = fadd float [[VAL]], %a
; CHECK-NEXT:    store float [[SUM]], ptr addrspace(1) %out, align 4
; CHECK-NEXT:    ret void

; Successor has a third predecessor; its PHI entries must be preserved.

; CHECK-LABEL: define spir_kernel void @kernel_extra_pred(
; CHECK:       mergedCallsBB:
; CHECK-NEXT:    [[ARG2:%.*]] = phi float [ %x1, %bb1 ], [ %x2, %bb2 ]
; CHECK-NEXT:    [[VAL2:%.*]] = phi float [ %x1, %bb1 ], [ %x2, %bb2 ]
; CHECK-NEXT:    call spir_func void @ext(float [[ARG2]])
; CHECK-NEXT:    br label %merge
; CHECK:       merge:
; CHECK-NEXT:    [[P2:%.*]] = phi float [ %b, %bb3 ], [ [[VAL2]], %mergedCallsBB ]
; CHECK-NEXT:    [[SAME2:%.*]] = phi float [ %b, %bb3 ], [ %a, %mergedCallsBB ]
; CHECK-NOT:     phi
; CHECK:         ret void

define spir_kernel void @kernel(float %a, float %b, i32 %sel, ptr addrspace(1) %out) {
entry:
  %cond = icmp eq i32 %sel, 0
  br i1 %cond, label %bb1, label %bb2

bb1:
  %x1 = fadd float %a, 1.000000e+00
  call spir_func void @ext(float %x1)
  br label %merge

bb2:
  %x2 = fmul float %b, 2.000000e+00
  call spir_func void @ext(float %x2)
  br label %merge

merge:
  %p = phi float [ %x1, %bb1 ], [ %x2, %bb2 ]
  %same = phi float [ %a, %bb1 ], [ %a, %bb2 ]
  %sum = fadd float %p, %same
  store float %sum, ptr addrspace(1) %out, align 4
  ret void
}

define spir_kernel void @kernel_extra_pred(float %a, float %b, i32 %sel, ptr addrspace(1) %out) {
entry:
  switch i32 %sel, label %bb3 [
    i32 0, label %bb1
    i32 1, label %bb2
  ]

bb1:
  %x1 = fadd float %a, 1.000000e+00
  call spir_func void @ext(float %x1)
  br label %merge

bb2:
  %x2 = fmul float %b, 2.000000e+00
  call spir_func void @ext(float %x2)
  br label %merge

bb3:
  br label %merge

merge:
  %p = phi float [ %x1, %bb1 ], [ %x2, %bb2 ], [ %b, %bb3 ]
  %same = phi float [ %a, %bb1 ], [ %a, %bb2 ], [ %b, %bb3 ]
  %sum = fadd float %p, %same
  store float %sum, ptr addrspace(1) %out, align 4
  ret void
}

declare spir_func void @ext(float)

!igc.functions = !{!0, !3}
!0 = !{ptr @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @kernel_extra_pred, !1}
