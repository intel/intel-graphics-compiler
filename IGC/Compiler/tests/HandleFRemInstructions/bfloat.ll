;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
;
; RUN: igc_opt -debugify --igc-handle-frem-inst -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; HandleFRemInstructions
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_frem_bfloat(bfloat %src1, bfloat %src2) {
; CHECK-LABEL: @test_frem_bfloat(
; CHECK:    [[TMP1:%.*]] = fpext bfloat %src1 to float
; CHECK:    [[TMP2:%.*]] = fpext bfloat %src2 to float
; CHECK:    [[TMP3:%.*]] = call float @__builtin_spirv_OpFRem_f32_f32(float [[TMP1]], float [[TMP2]])
; CHECK:    [[TMP4:%.*]] = fptrunc float [[TMP3]] to bfloat
; CHECK:    call void @use.bf(bfloat [[TMP4]])
; CHECK:    ret void
  %1 = frem bfloat %src1, %src2
  call void @use.bf(bfloat %1)
  ret void
}

define spir_kernel void @test_frem_vec_bfloat(<2x bfloat> %src1, <2 x bfloat> %src2) {
; CHECK:    [[TMP1:%.*]] = fpext <2 x bfloat> %src1 to <2 x float>
; CHECK:    [[TMP2:%.*]] = fpext <2 x bfloat> %src2 to <2 x float>
; CHECK:    [[TMP3:%.*]] = call <2 x float> @__builtin_spirv_OpFRem_v2f32_v2f32(<2 x float> [[TMP1]], <2 x float> [[TMP2]])
; CHECK:    [[TMP4:%.*]] = fptrunc <2 x float> [[TMP3]] to <2 x bfloat>
; CHECK:    call void @use.v2bf(<2 x bfloat> [[TMP4]])
; CHECK:    ret void
  %1 = frem <2 x bfloat> %src1, %src2
  call void @use.v2bf(<2 x bfloat> %1)
  ret void
}

declare void @use.bf(bfloat)
declare void @use.v2bf(<2 x bfloat>)
