;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --igc-pressure-printer -S --disable-output --regkey=RegPressureVerbocity=1 < %s 2>&1 | FileCheck %s

; Test that bestGuessSIMDSize honors FuncMD.requiredSubGroupSize via the
; ModuleMetaData-based IGC::getSIMDSize interface, even when the function
; has no matching entry in MDUtils' FunctionsInfo (igc.functions metadata).
; Previously, the metadata-based SIMD size was gated on a successful
; MDUtils->findFunctionsInfoItem lookup and would be ignored otherwise.

define spir_kernel void @testSimd16(ptr addrspace(1) %out, i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i32
  %idx = zext i32 %lid to i64
  %ptr = getelementptr inbounds float, ptr addrspace(1) %out, i64 %idx
  store float 1.000000e+00, ptr addrspace(1) %ptr, align 4
  ret void
}

; CHECK: SIMD: 16, external pressure: 0
; CHECK: function: testSimd16

!IGCMetadata = !{!0}
!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @testSimd16}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"requiredSubGroupSize", i32 16}
