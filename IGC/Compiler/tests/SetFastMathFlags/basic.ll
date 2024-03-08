;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-set-fast-math-flags -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SetFastMathFlags
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_fast(float %src1, float %src2) #0 {
; CHECK-LABEL: @test_fast(
; CHECK: fadd fast float
; CHECK: fsub fast float
; CHECK: call fast float @llvm.sqrt.f32
;
  %1 = fadd float %src1, %src2
  %2 = fsub float %src1, %src2
  %3 = call float @llvm.sqrt.f32(float %src1)
  ret void
}

define void @test_nozero(float %src1, float %src2) #1 {
; CHECK-LABEL: @test_nozero(
; CHECK: fadd nsz float
; CHECK: fsub nsz float
; CHECK: call float @llvm.sqrt.f32
;
  %1 = fadd float %src1, %src2
  %2 = fsub float %src1, %src2
  %3 = call float @llvm.sqrt.f32(float %src1)
  ret void
}

define void @test_noinf(float %src1, float %src2) #2 {
; CHECK-LABEL: @test_noinf(
; CHECK: fadd ninf float
; CHECK: fsub ninf float
; CHECK: call float @llvm.sqrt.f32
;
  %1 = fadd float %src1, %src2
  %2 = fsub float %src1, %src2
  %3 = call float @llvm.sqrt.f32(float %src1)
  ret void
}

define void @test_nonan(float %src1, float %src2) #3 {
; CHECK-LABEL: @test_nonan(
; CHECK: fadd nnan float
; CHECK: fsub nnan float
; CHECK: call float @llvm.sqrt.f32
;
  %1 = fadd float %src1, %src2
  %2 = fsub float %src1, %src2
  %3 = call float @llvm.sqrt.f32(float %src1)
  ret void
}

attributes #0 = { "unsafe-fp-math"="true" }
attributes #1 = { "no-signed-zeros-fp-math"="true" }
attributes #2 = { "no-infs-fp-math"="true" }
attributes #3 = { "no-nans-fp-math"="true" }
declare float @llvm.sqrt.f32(float) #0

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3, !4}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"NoSignedZeros", i1 true}
!4 = !{!"FiniteMathOnly", i1 true}
