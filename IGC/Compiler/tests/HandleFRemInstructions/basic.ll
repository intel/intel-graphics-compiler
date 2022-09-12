;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-handle-frem-inst -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; HandleFRemInstructions
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_frem(float %src1, float %src2) {
; CHECK-LABEL: @test_frem(
; CHECK:    [[TMP1:%.*]] = call float @__builtin_spirv_OpFRem_f32_f32(float %src1, float %src2)
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = frem float %src1, %src2
  call void @use.f32(float %1)
  ret void
}

define spir_kernel void @test_frem_vec(<2x float> %src1, <2 x float> %src2) {
; CHECK-LABEL: @test_frem_vec(
; CHECK:    [[TMP1:%.*]] = call <2 x float> @__builtin_spirv_OpFRem_v2f32_v2f32(<2 x float> %src1, <2 x float> %src2)
; CHECK:    call void @use.v2f32(<2 x float> [[TMP1]])
; CHECK:    ret void
;
  %1 = frem <2 x float> %src1, %src2
  call void @use.v2f32(<2 x float> %1)
  ret void
}

declare void @use.f32(float)
declare void @use.v2f32(<2 x float>)
