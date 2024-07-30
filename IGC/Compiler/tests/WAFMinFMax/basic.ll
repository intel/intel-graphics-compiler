;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-wa-fminmax  -ocl -enable-debugify -enable-fmax-fmin-plus-zero -retain-denormals -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; WAFMinFMax
; ------------------------------------------------

; Test checks WA to properly treat input sNaNs for fmin

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_fmin(float %src1, float %src2) {
; CHECK-LABEL: @test_fmin(
; CHECK:    [[TMP1:%.*]] = call float @llvm.minnum.f32(float %src1, float 0x7FF8000000000000)
; CHECK:    [[TMP2:%.*]] = call float @llvm.minnum.f32(float %src2, float 0x7FF8000000000000)
; CHECK:    [[TMP3:%.*]] = call float @llvm.minnum.f32(float [[TMP1]], float [[TMP2]])
; CHECK:    ret void

  %1 = call float @llvm.minnum.f32(float %src1, float %src2)
  ret void
}

declare float @llvm.minnum.f32(float, float)

