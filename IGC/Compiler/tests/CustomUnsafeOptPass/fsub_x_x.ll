;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-unsafe-opt-pass -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @sample_test(float %x, float %y, float addrspace(1)* nocapture %res) nounwind {
entry:
  %sub = fsub float %x, %x
  store float %sub, float addrspace(1)* %res
  ret void
}

; CHECK-NOT:     fsub float %x, %x
; CHECK:         store float 0.000000e+00

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
