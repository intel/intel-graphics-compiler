;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -breakdown-intrinsics -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; BreakdownIntrinsic
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_fmuladd(float %a, float %b, float %c) {
; CHECK-LABEL: @test_fmuladd(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fmul float [[A:%[A-z0-9]*]], [[B:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = fadd float [[TMP1]], [[C:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP2]])
; CHECK:    ret void
;
  %1 = call fast float @llvm.fmuladd.f32(float %a, float %b, float %c)
  call void @use.f32(float %1)
  ret void
}

define spir_kernel void @test_fma(float %a, float %b, float %c) {
; CHECK-LABEL: @test_fma(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fmul float [[A:%[A-z0-9]*]], [[B:%[A-z0-9]*]]
; CHECK:    [[TMP2:%[A-z0-9]*]] = fadd float [[TMP1]], [[C:%[A-z0-9]*]]
; CHECK:    call void @use.f32(float [[TMP2]])
; CHECK:    ret void
;
  %1 = call fast float @llvm.fma.f32(float %a, float %b, float %c)
  call void @use.f32(float %1)
  ret void
}

declare float @llvm.fma.f32(float, float, float)
declare float @llvm.fmuladd.f32(float, float, float)
declare void @use.f32(float)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"UnsafeMathOptimizations", i1 true}
