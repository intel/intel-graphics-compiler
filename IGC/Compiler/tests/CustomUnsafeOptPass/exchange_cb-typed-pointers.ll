;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-unsafe-opt-pass -S %s -o %t.ll
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

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
