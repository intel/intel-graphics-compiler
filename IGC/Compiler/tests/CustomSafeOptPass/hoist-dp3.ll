;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey ForceHoistDp3=1  -igc-custom-safe-opt -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: Hoist dp3
; ------------------------------------------------

define void @test_dp3(double %dx1, double %dx2, double %dy1, double %dy2, double %dz1, double %dz2) {
; CHECK-LABEL: @test_dp3(
; CHECK:    [[X1:%.*]] = fptrunc double [[DX1:%.*]] to float
; CHECK:    [[X2:%.*]] = fptrunc double [[DX2:%.*]] to float
; CHECK:    [[Y1:%.*]] = fptrunc double [[DY1:%.*]] to float
; CHECK:    [[Y2:%.*]] = fptrunc double [[DY2:%.*]] to float
; CHECK:    [[Z1:%.*]] = fptrunc double [[DZ1:%.*]] to float
; CHECK:    [[Z2:%.*]] = fptrunc double [[DZ2:%.*]] to float
; CHECK:    [[TMP1:%.*]] = fmul float [[X1]], [[X2]]
; CHECK:    [[TMP2:%.*]] = fmul float [[Y1]], [[Y2]]
; CHECK:    [[TMP3:%.*]] = fadd float [[TMP1]], [[TMP2]]
; CHECK:    [[TMP4:%.*]] = fmul float [[Z1]], [[Z2]]
; CHECK:    [[TMP5:%.*]] = fadd float [[TMP4]], [[TMP3]]
; CHECK:    [[FILL1:%.*]] = fadd float [[X1]], [[X2]]
; CHECK:    [[FILL2:%.*]] = fadd float [[Y1]], [[Y2]]
; CHECK:    [[FILL3:%.*]] = fadd float [[Z1]], [[Z2]]
; CHECK:    call void @use.f32(float [[TMP5]])
;
  %x1 = fptrunc double %dx1 to float
  %x2 = fptrunc double %dx2 to float
  %y1 = fptrunc double %dy1 to float
  %y2 = fptrunc double %dy2 to float
  %z1 = fptrunc double %dz1 to float
  %z2 = fptrunc double %dz2 to float
  %fill1 = fadd float %x1, %x2
  %fill2 = fadd float %y1, %y2
  %fill3 = fadd float %z1, %z2
  %1 = fmul float %x1, %x2
  %2 = fmul float %y1, %y2
  %3 = fadd float %1, %2
  %4 = fmul float %z1, %z2
  %5 = fadd float %4, %3
  call void @use.f32(float %5)
  ret void
}

declare void @use.f32(float)
