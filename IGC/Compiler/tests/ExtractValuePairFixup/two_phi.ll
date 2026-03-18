;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-extractvalue-pair-fixup -S < %s | FileCheck %s
; ------------------------------------------------
; ExtractValuePairFixup
; ------------------------------------------------
; Check extractvalue instruction is not inserted between phi instructions

%ty = type { i32, i32 }

define spir_kernel void @test_phi_extr(i32 %flag) {
entry:
  %0 = insertvalue %ty { i32 0, i32 0 }, i32 1, 0
  %1 = insertvalue %ty %0, i32 1, 1
  %switch = icmp eq i32 %flag, 0
  br i1 %switch, label %lb1, label %lb2

lb1:
  %2 = insertvalue %ty %1, i32 2, 0
  %3 = insertvalue %ty %2, i32 2, 1
  %4 = insertvalue %ty %3, i32 5, 1
  br label %lb2

; CHECK-LABEL: lb2:
; CHECK: [[PHI1:%[A-z0-9]*]] = phi %ty [ %1, %entry ], [ %3, %lb1 ]
; CHECK-NEXT: [[PHI2:%[A-z0-9]*]] = phi %ty [ %1, %entry ], [ %4, %lb1 ]
; CHECK-NEXT: [[EXV1:%[A-z0-9]*]] = extractvalue %ty [[PHI1]], 0
; CHECK-NEXT: [[EXV1:%[A-z0-9]*]] = extractvalue %ty [[PHI2]], 1
lb2:
  %5 = phi %ty [%1, %entry], [%3, %lb1]
  %6 = phi %ty [%1, %entry], [%4, %lb1]
  call void @use_i32(i32 %flag)
  %7 = extractvalue %ty %5, 0
  call void @use_i32(i32 %7)
  %8 = extractvalue %ty %6, 1
  call void @use_i32(i32 %8)
  ret void
}

declare void @use_i32(i32)

!igc.functions = !{!0}
!0 = !{void (i32, i32)* @test_phi_extr, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
