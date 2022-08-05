;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests CustomUnsafeOptPass::visitExchangeCB

; (x * cb1) + (x * cb2) = x * (cb1 + cb2)
; where "cb" is load from constant buffer
define float @test1(float %x, float addrspace(65536)* %cb1, float addrspace(65536)* %cb2) #0 {
entry:
  %0 = load float, float addrspace(65536)* %cb1
  %1 = fmul float %x, %0
  %2 = load float, float addrspace(65536)* %cb2
  %3 = fmul float %x, %2
  %4 = fadd float %1, %3
  ret float %4
}

; CHECK-LABEL: define float @test1
; CHECK: [[CB1:%[A-z0-9]*]] = load float, float addrspace(65536)* %cb1
; CHECK-NOT: fmul float %x, [[CB1]]
; CHECK: [[CB2:%[A-z0-9]*]] = load float, float addrspace(65536)* %cb2
; CHECK-NOT: fmul float %x, [[CB2]]
; CHECK: [[ADD:%[A-z0-9]*]] = fadd float [[CB1]], [[CB2]]
; CHECK: [[MUL:%[A-z0-9]*]] = fmul float %x, [[ADD]]
; CHECK: ret float [[MUL]]

; (x * 2) + (x * 3) = x * (2 + 3)
define float @test2(float %x) #0 {
entry:
  %0 = fmul float %x, 2.000000e+00
  %1 = fmul float %x, 3.000000e+00
  %2 = fadd float %0, %1
  ret float %2
}

; CHECK-LABEL: define float @test2
; CHECK-NOT: fmul float %x, 2.000000e+00
; CHECK-NOT: fmul float %x, 3.000000e+00
; CHECK: %0 = fadd float 2.000000e+00, 3.000000e+00
; CHECK: %1 = fmul float %x, %0
; CHECK: ret float %1

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
