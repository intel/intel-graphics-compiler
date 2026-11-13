;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; 0/x => 0
define float @div_to_zero(float %x) #0 {
entry:
  %0 = fdiv fast float 0.000000e+00, %x
  ret float %0
}

; CHECK-LABEL: define float @div_to_zero
; CHECK-NOT: fdiv
; CHECK: ret float 0.000000e+00

; x/1 => x
define float @div_by_one(float %x) #0 {
entry:
  %0 = fdiv fast float %x, 1.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @div_by_one
; CHECK-NOT: fdiv
; CHECK: ret float %x

; x/3 => x * 1/3
define float @div_to_mul(float %x) #0 {
entry:
  %0 = fdiv fast float %x, 3.000000e+00
  ret float %0
}

; CHECK-LABEL: define float @div_to_mul
; CHECK-NOT: fdiv float %x, 3.000000e+00
; CHECK: %0 = fdiv fast float 1.000000e+00, 3.000000e+00
; CHECK: %1 = fmul fast float %x, %0
; CHECK: ret float %1

; 1/rsq(x) => sqrt(x), sqrt has the type of rsq
define float @rsq_to_sqrt(float %x) #0 {
entry:
  %0 = call float @llvm.genx.GenISA.rsq.f32(float %x)
  %1 = fdiv fast float 1.000000e+00, %0
  ret float %1
}

; CHECK-LABEL: define float @rsq_to_sqrt
; CHECK-NOT: fdiv
; CHECK: %0 = call float @llvm.sqrt.f32(float %x)
; CHECK: ret float %0

define half @rsq_to_sqrt_half(half %x) #0 {
entry:
  %0 = call half @llvm.genx.GenISA.rsq.f16(half %x)
  %1 = fdiv fast half 0xH3C00, %0
  ret half %1
}

; CHECK-LABEL: define half @rsq_to_sqrt_half
; CHECK-NOT: fdiv
; CHECK: %0 = call half @llvm.sqrt.f16(half %x)
; CHECK: ret half %0

; C/rsq(x) => C * sqrt(x)
define double @rsq_to_mul_sqrt_double(double %x) #0 {
entry:
  %0 = call double @llvm.genx.GenISA.rsq.f64(double %x)
  %1 = fdiv fast double 2.000000e+00, %0
  ret double %1
}

; CHECK-LABEL: define double @rsq_to_mul_sqrt_double
; CHECK-NOT: fdiv
; CHECK: %0 = call double @llvm.sqrt.f64(double %x)
; CHECK: %1 = fmul fast double 2.000000e+00, %0
; CHECK: ret double %1

declare float @llvm.genx.GenISA.rsq.f32(float)
declare half @llvm.genx.GenISA.rsq.f16(half)
declare double @llvm.genx.GenISA.rsq.f64(double)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
