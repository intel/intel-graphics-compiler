;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -ocl -platformdg1 -igc-error-check -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; Test expects EnableDPEmulation and ForceDPEmulation with false default value

; CHECK: error: Double type is not supported on this platform.

define void @test_error(double %src) {
  %1 = fadd double %src, %src
  ret void
}
