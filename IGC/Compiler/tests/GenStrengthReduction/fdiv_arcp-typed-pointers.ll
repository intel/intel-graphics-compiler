;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -platformpvc -debugify -GenStrengthReduction -check-debugify -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS


define spir_kernel void @test_fdiv_half(half addrspace(1)* %a, half addrspace(1)* %b) {
entry:
; CHECK-LABEL: test_fdiv_half
; CHECK:    %0 = load half, half addrspace(1)* %a, align 2
; CHECK:    %1 = load half, half addrspace(1)* %b, align 2
; CHECK:    %2 = fdiv arcp half 0xH3C00, %1
; CHECK:    %3 = fmul arcp half %0, %2
; CHECK:    store half %3, half addrspace(1)* %a, align 2
; CHECK:    ret void
  %0 = load half, half addrspace(1)* %a, align 2
  %1 = load half, half addrspace(1)* %b, align 2
  %conv1 = fdiv arcp half  %0, %1
  store half %conv1, half addrspace(1)* %a, align 2
  ret void
}

define spir_kernel void @test_fdiv_float(float addrspace(1)* %a, float addrspace(1)* %b) {
entry:
; CHECK-LABEL: test_fdiv_float
; CHECK:    %0 = load float, float addrspace(1)* %a, align 4
; CHECK:    %1 = load float, float addrspace(1)* %b, align 4
; CHECK:    %2 = fdiv arcp float 1.000000e+00, %1
; CHECK:    %3 = fmul arcp float %0, %2
; CHECK:    store float %3, float addrspace(1)* %a, align 4
; CHECK:    ret void
  %0 = load float, float addrspace(1)* %a, align 4
  %1 = load float, float addrspace(1)* %b, align 4
  %conv1 = fdiv arcp float  %0, %1
  store float %conv1, float addrspace(1)* %a, align 4
  ret void
}

define spir_kernel void @test_fdiv_double_once(double addrspace(1)* %a, double addrspace(1)* %b) {
entry:
; CHECK-LABEL: test_fdiv_double_once
; CHECK:    %0 = load double, double addrspace(1)* %a, align 8
; CHECK:    %1 = load double, double addrspace(1)* %b, align 8
; CHECK:    %conv1 = fdiv arcp double  %0, %1
; CHECK:    store double %conv1, double addrspace(1)* %a, align 8
; CHECK:    ret void
  %0 = load double, double addrspace(1)* %a, align 8
  %1 = load double, double addrspace(1)* %b, align 8
  %conv1 = fdiv arcp double  %0, %1
  store double %conv1, double addrspace(1)* %a, align 8
  ret void
}

define spir_kernel void @test_fdiv_double_twice(double addrspace(1)* %a, double addrspace(1)* %b, double addrspace(1)* %c) {
entry:
; CHECK-LABEL: test_fdiv_double_twice
; CHECK:    %0 = load double, double addrspace(1)* %a, align 8
; CHECK:    %1 = load double, double addrspace(1)* %b, align 8
; CHECK:    %2 = load double, double addrspace(1)* %c, align 8
; CHECK:    %3 = fdiv arcp double 1.000000e+00, %2
; CHECK:    %4 = fmul arcp double %0, %3
; CHECK:    %5 = fmul arcp double %1, %3
; CHECK:    %add1 = fdiv double %4, %5
; CHECK:    store double %add1, double addrspace(1)* %a, align 8
; CHECK:    ret void
  %0 = load double, double addrspace(1)* %a, align 8
  %1 = load double, double addrspace(1)* %b, align 8
  %2 = load double, double addrspace(1)* %c, align 8
  %conv1 = fdiv arcp double  %0, %2
  %conv2 = fdiv arcp double  %1, %2
  %add1 = fdiv double %conv1, %conv2
  store double %add1, double addrspace(1)* %a, align 8
  ret void
}
