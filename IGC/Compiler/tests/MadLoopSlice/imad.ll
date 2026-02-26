;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --platformpvc --opaque-pointers -igc-madloopslice -S < %s | FileCheck %s
; ------------------------------------------------
; MadLoopSlice (block slicing)
; ------------------------------------------------

define spir_kernel void @test(i32 %a0, i32 %b0, i32 %a1, i32 %b1) {
entry:
  ; Two independent IMAD chains intentionally interleaved.
  %m0 = mul i32 %a0, %b0
  %m1 = mul i32 %a1, %b1
  %c0 = add i32 %m0, %b0
  %c1 = add i32 %m1, %b1
  %m2 = mul i32 %b0, %c0
  %m3 = mul i32 %b1, %c1
  %d0 = add i32 %m2, %b0
  %d1 = add i32 %m3, %b1
  ret void
}

; CHECK-LABEL: define spir_kernel void @test
; CHECK: entry:
; CHECK-NEXT: %m0 = mul i32 %a0, %b0
; CHECK-NEXT: %c0 = add i32 %m0, %b0
; CHECK-NEXT: %m2 = mul i32 %b0, %c0
; CHECK-NEXT: %d0 = add i32 %m2, %b0
; CHECK-NEXT: %m1 = mul i32 %a1, %b1
; CHECK-NEXT: %c1 = add i32 %m1, %b1
; CHECK-NEXT: %m3 = mul i32 %b1, %c1
; CHECK-NEXT: %d1 = add i32 %m3, %b1
; CHECK-NEXT: ret void

!igc.functions = !{!0}

!0 = !{void (i32, i32, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
