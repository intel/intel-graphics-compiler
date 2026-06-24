;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-poison-fp64-kernels -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PoisonFP64Kernels
; ------------------------------------------------

; Test checks that function that illegally uses fp64 instructions(has corresponding attr) is removed.
;

define void @test_kernel(double %src, double* %dst) {
; CHECK: @test_kernel(
; CHECK-SAME: #[[ATTR:[0-9]*]]
; CHECK-NEXT:  entry:
; CHECK-NEXT:    ret void
;
  %1 = call double @test_func(double %src)
  store double %1, double* %dst
  ret void
}

; Check that function body is removed
;
; CHECK-NOT: define double @test_func

; Check that invalid_kernel attribute is added
;
; CHECK: attributes #[[ATTR]] = { "invalid_kernel("uses-fp64-math")" }

define double @test_func(double %src) #0 {
  %1 = fadd double %src, 2.0
  ret double %1
}

attributes #0 = { "uses-fp64-math" }
declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (double, double*)* @test_kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{double (double)* @test_func, !4}
!4 = !{!5}
!5 = !{!"function_type", i32 2}

