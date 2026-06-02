;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-low-precision-opt -S < %s | FileCheck %s
; ------------------------------------------------
; LowPrecisionOpt
; ------------------------------------------------
; In OpenCL half is a precise IEEE-754 binary16, so the lossy fpext(fptrunc(x))
; round-trip must NOT be folded away under precise math (no fast-relaxed-math).
; igc_opt defaults to the OpenCL shader type, so the round-trip is preserved.

define void @test_fptrunc_fpext(float %src1, float %src2) {
; CHECK-LABEL: @test_fptrunc_fpext(
; CHECK:    [[TMP1:%.*]] = fadd float %src1, %src2
; CHECK:    [[TMP2:%.*]] = fptrunc float [[TMP1]] to half
; CHECK:    [[TMP3:%.*]] = fpext half [[TMP2]] to float
; CHECK:    call void @use.f16(half [[TMP2]])
; CHECK:    call void @use.f32(float [[TMP3]])
; CHECK:    ret void

  %1 = fadd float %src1, %src2
  %2 = fptrunc float %1 to half
  %3 = fpext half %2 to float
  call void @use.f16(half %2)
  call void @use.f32(float %3)
  ret void
}


declare void @use.f16(half)
declare void @use.f32(float)

!igc.functions = !{!0}

!0 = !{void (float,float)* @test_fptrunc_fpext, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
