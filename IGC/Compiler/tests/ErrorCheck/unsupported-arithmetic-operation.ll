;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -ocl -platformdg2 -igc-error-check -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; The test emits an error when the platform can emulate DP conv operations,
; but DP arithmetic operations are used in the kernel and poisonFP64Kernels is disabled

; CHECK: error: Double arithmetic operation is not supported on this platform with FP64 conversion emulation mode (poison FP64 kernels is disabled).

define void @test_error_arithmetic_operation(double %src, double %src2) {
  %1 = fadd double %src, %src2
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{void (double, double)* @test_error_arithmetic_operation, !1}
!1 = !{!3}
!3 = !{!"function_type", i32 0}
!4 = !{!"ModuleMD", !5}
!5 = !{!"compOpt", !77, !78}

!76 = !{!"EnableUnsupportedFP64Poisoning", i1 false}
!77 = !{!"FP64GenEmulationEnabled", i1 false}
!78 = !{!"FP64GenConvEmulationEnabled", i1 true}
