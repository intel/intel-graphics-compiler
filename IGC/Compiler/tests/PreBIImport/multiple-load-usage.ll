;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-Pre-BIImport-Analysis -S < %s | FileCheck %s
; ------------------------------------------------
; PreBIImport
; ------------------------------------------------
; This test checks that PreBIImport checks load users in functionality related
; to math functions. This pass checks if there exists a PI number inside a
; sin/cos function, and replace the value PI*x with x, and changes a call from
; a sin to sinPI. It can only be achieved when the value is used only in the
; math function. If it's used also in other places, we lose the correct value.
;

define spir_kernel void @test_div(float %src1, float* %dst, float * %dst2) {
  %1 = fmul float 0x401921FB60000000, %src1
; CHECK: fmul float 0x401921FB60000000
  %2 = alloca float, align 4
  store float %1, float* %2
  %3 = load float, float* %2
  %4 = call float @__builtin_spirv_OpenCL_sin_f32(float %3)
; CHECK-NOT: call float @__builtin_spirv_OpenCL_sinpi_f32
  store float %3, float* %dst
  store float %4, float* %dst2
  ret void
}

declare float @__builtin_spirv_OpenCL_sin_f32(float)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3}
!2 = !{!"MatchSinCosPi", i1 true}
!3 = !{!"FastRelaxedMath", i1 false}
