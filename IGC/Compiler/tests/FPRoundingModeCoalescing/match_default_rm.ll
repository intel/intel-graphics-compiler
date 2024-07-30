;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify --igc-fp-rounding-mode-coalescing -check-debugify -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; CHECK: CheckModuleDebugify: PASS

; Test verifies if pass doesn't change order of instructions in case explicit rounding mode
; already matches default one in kernel.
define spir_kernel void @test(double %src1, double %src2, double %src3, double addrspace(1)* %dst) {
entry:
; CHECK-LABEL: entry:
; CHECK:         %fma.rtz.1.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
; CHECK:         %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result
; CHECK:         %fma.rtz.2.arg = fsub double %src1, %src3
; CHECK:         %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.1.result, double %fma.rtz.2.arg)
; CHECK:         %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result
; CHECK:         ret void

  %fma.rtz.1.arg = fsub double %src1, %src3
  %fma.rtz.1.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %src2, double %fma.rtz.1.arg)
  %tmp.rtz.1 = fadd double %src1, %fma.rtz.1.result

  %fma.rtz.2.arg = fsub double %src1, %src3
  %fma.rtz.2.result = call double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double %src1, double %fma.rtz.1.result, double %fma.rtz.2.arg)
  %tmp.rtz.2 = fadd double %src1, %fma.rtz.2.result

  ret void
}

declare double @llvm.genx.GenISA.fma.rtz.f64.f64.f64.f64(double, double, double)

!igc.functions = !{!0}
!IGCMetadata = !{!200}

!0 = !{void (double, double, double, double addrspace(1)*)* @test, !100}
!100 = !{!101}
!101 = !{!"function_type", i32 0}
!200 = !{!"ModuleMD", !201}
!201 = !{!"compOpt", !202}
!202 = !{!"FloatRoundingMode", i32 3}
