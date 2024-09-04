;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; tests removeHftoFCast

define float @doNothing(float %a, float %b) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fadd half %1, %2
    %4 = fpext half %3 to float
    ret float %4
}

; CHECK-LABEL: define float @doNothing
; CHECK: %1 = fptrunc float %a to half
; CHECK: %2 = fptrunc float %b to half
; CHECK: %3 = fadd half %1, %2
; CHECK: %4 = fpext half %3 to float
; CHECK: ret float %4

define float @fastAttr(float %a, float %b) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fadd fast half %1, %2
    %4 = fpext half %3 to float
    ret float %4
}

; CHECK-LABEL: define float @fastAttr
; CHECK-NOT: fptrunc
; CHECK-NOT: fpext
; CHECK: %1 = fadd fast float %a, %b
; CHECK: ret float %1

define float @afnAttr(float %a, float %b) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fadd afn half %1, %2
    %4 = fpext half %3 to float
    ret float %4
}

; CHECK-LABEL: define float @afnAttr
; CHECK-NOT: fptrunc
; CHECK-NOT: fpext
; CHECK: %1 = fadd afn float %a, %b
; CHECK: ret float %1

define float @reassocAttr(float %a, float %b) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fadd reassoc half %1, %2
    %4 = fpext half %3 to float
    ret float %4
}

; CHECK-LABEL: define float @reassocAttr
; CHECK-NOT: fptrunc
; CHECK-NOT: fpext
; CHECK: %1 = fadd reassoc float %a, %b
; CHECK: ret float %1

define float @doNothingMulAdd(float %a, float %b, half %c) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fmul half %1, %c
    %4 = fadd half %2, %3
    %5 = fpext half %4 to float
    ret float %5
}

; CHECK-LABEL: define float @doNothingMulAdd
; CHECK: %1 = fptrunc float %a to half
; CHECK: %2 = fptrunc float %b to half
; CHECK: %3 = fmul half %1, %c
; CHECK: %4 = fadd half %2, %3
; CHECK: %5 = fpext half %4 to float
; CHECK: ret float %5

define float @thirdOperandIsInstruction(float %a, float %b, i32 %c) {
    %dummy_half = sitofp i32 %c to half
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fmul fast half %1, %dummy_half
    %4 = fadd fast half %2, %3
    %5 = fpext half %4 to float
    ret float %5
}

; CHECK-LABEL: define float @thirdOperandIsInstruction
; CHECK-NOT: fptrunc
; CHECK: %1 = fpext half %dummy_half to float
; CHECK: %2 = fmul float %a, %1
; CHECK: %3 = fadd fast float %2, %b
; CHECK: ret float %3

define float @thirdOperandIsValue(float %a, float %b, half %c) {
    %1 = fptrunc float %a to half
    %2 = fptrunc float %b to half
    %3 = fmul fast half %1, %c
    %4 = fadd fast half %2, %3
    %5 = fpext half %4 to float
    ret float %5
}

; CHECK-LABEL: define float @thirdOperandIsValue
; CHECK-NOT: fptrunc
; CHECK: %1 = fpext half %c to float
; CHECK: %2 = fmul float %a, %1
; CHECK: %3 = fadd fast float %2, %b
; CHECK: ret float %3


!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
