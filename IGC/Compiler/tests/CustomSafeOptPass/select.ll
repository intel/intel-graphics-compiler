;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify --igc-custom-safe-opt -check-debugify -S < %s 2>&1 | FileCheck %s

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_select_and(i1 %srca, i1 %srcb) {
; CHECK-LABEL: @test_select_and(
; CHECK:    [[TMP:%[A-z0-9]*]] = and i1 %srca, %srcb
; CHECK:    call void @use.i1(i1 [[TMP]])
; CHECK:    ret void
;
  %1 = select i1 %srca, i1 %srcb, i1 false
  call void @use.i1(i1 %1)
  ret void
}

define spir_kernel void @test_select_or(i1 %srca, i1 %srcb) {
; CHECK-LABEL: @test_select_or(
; CHECK:    [[TMP:%[A-z0-9]*]] = or i1 %srca, %srcb
; CHECK:    call void @use.i1(i1 [[TMP]])
; CHECK:    ret void
;
  %1 = select i1 %srca, i1 true, i1 %srcb
  call void @use.i1(i1 %1)
  ret void
}

declare void @use.i1(i1)
