;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

declare float @llvm.sqrt.f32(float)

; y*y = x if y = sqrt(x)
define float @sqrt(float %x) #0 {
entry:
  %0 = call float @llvm.sqrt.f32(float %x)
  %1 = fmul float %0, %0
  ret float %1
}

; CHECK-LABEL: define float @sqrt
; CHECK-NOT: llvm.sqrt.f32
; CHECK-NOT: fmul float %0, %0
; CHECK: ret float %x

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
