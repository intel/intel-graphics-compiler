;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-madloopslice -S < %s | FileCheck %s
; ------------------------------------------------
; MadLoopSlice (block slicing / DMAD)
; ------------------------------------------------

define spir_kernel void @test(double %a0, double %b0, double %a1, double %b1) {
entry:
  ; Setup ops that must not prevent slicing.
  %p0 = fadd double %a0, 1.000000e+00
  %p1 = fadd double %a1, 2.000000e+00
  ; Two independent DMAD chains intentionally interleaved.
  %m0 = fmul double %p0, %b0
  %m1 = fmul double %p1, %b1
  %c0 = fadd double %m0, %p0
  %c1 = fadd double %m1, %p1
  %m2 = fmul double %c0, %b0
  %m3 = fmul double %c1, %b1
  %d0 = fadd double %m2, %c0
  %d1 = fadd double %m3, %c1
  ret void
}

; CHECK-LABEL: define spir_kernel void @test
; CHECK: entry:
; CHECK-NEXT: %p0 = fadd double %a0, 1.000000e+00
; CHECK-NEXT: %m0 = fmul double %p0, %b0
; CHECK-NEXT: %c0 = fadd double %m0, %p0
; CHECK-NEXT: %m2 = fmul double %c0, %b0
; CHECK-NEXT: %d0 = fadd double %m2, %c0
; CHECK-NEXT: %p1 = fadd double %a1, 2.000000e+00
; CHECK-NEXT: %m1 = fmul double %p1, %b1
; CHECK-NEXT: %c1 = fadd double %m1, %p1
; CHECK-NEXT: %m3 = fmul double %c1, %b1
; CHECK-NEXT: %d1 = fadd double %m3, %c1
; CHECK-NEXT: ret void

!igc.functions = !{!0}

!0 = !{void (double, double, double, double)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
