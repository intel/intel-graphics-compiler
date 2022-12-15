;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
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
; CHECK:    [[TMP1:%[A-z0-9]*]] = fadd fast float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP2:%[A-z0-9]*]] = fsub fast float [[SRC1]], [[SRC2]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = call fast float @llvm.sqrt.f32(float [[SRC1]])
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fadd float %src1, %src2
  call void @use.f32(float %1)
  %2 = fsub float %src1, %src2
  call void @use.f32(float %1)
  %3 = call float @llvm.sqrt.f32(float %src1)
  call void @use.f32(float %1)
  ret void
}

define void @test_nozero(float %src1, float %src2) #1 {
; CHECK-LABEL: @test_nozero(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fadd nsz float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP2:%[A-z0-9]*]] = fsub nsz float [[SRC1]], [[SRC2]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = call float @llvm.sqrt.f32(float [[SRC1]])
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fadd float %src1, %src2
  call void @use.f32(float %1)
  %2 = fsub float %src1, %src2
  call void @use.f32(float %1)
  %3 = call float @llvm.sqrt.f32(float %src1)
  call void @use.f32(float %1)
  ret void
}

define void @test_noinf(float %src1, float %src2) #2 {
; CHECK-LABEL: @test_noinf(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fadd ninf float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP2:%[A-z0-9]*]] = fsub ninf float [[SRC1]], [[SRC2]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = call float @llvm.sqrt.f32(float [[SRC1]])
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fadd float %src1, %src2
  call void @use.f32(float %1)
  %2 = fsub float %src1, %src2
  call void @use.f32(float %1)
  %3 = call float @llvm.sqrt.f32(float %src1)
  call void @use.f32(float %1)
  ret void
}

define void @test_nonan(float %src1, float %src2) #3 {
; CHECK-LABEL: @test_nonan(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fadd nnan float [[SRC1:%[A-z0-9]*]], [[SRC2:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP2:%[A-z0-9]*]] = fsub nnan float [[SRC1]], [[SRC2]]
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    [[TMP3:%[A-z0-9]*]] = call float @llvm.sqrt.f32(float [[SRC1]])
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fadd float %src1, %src2
  call void @use.f32(float %1)
  %2 = fsub float %src1, %src2
  call void @use.f32(float %1)
  %3 = call float @llvm.sqrt.f32(float %src1)
  call void @use.f32(float %1)
  ret void
}

attributes #0 = { "unsafe-fp-math"="true" }
attributes #1 = { "no-signed-zeros-fp-math"="true" }
attributes #2 = { "no-infs-fp-math"="true" }
attributes #3 = { "no-nans-fp-math"="true" }
declare void @use.f32(float)
declare float @llvm.sqrt.f32(float) #0

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2, !3, !4}
!2 = !{!"FastRelaxedMath", i1 true}
!3 = !{!"NoSignedZeros", i1 true}
!4 = !{!"FiniteMathOnly", i1 true}
