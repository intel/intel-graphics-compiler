;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -debugify -GenFDIVEmulation -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenFDIVEmulation
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_fdiv(float %a, float %b) {
; CHECK-LABEL: @test_fdiv(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fcmp oeq float [[A:%[A-z0-9]*]], [[B:%[A-z0-9]*]]
; CHECK:    br i1 [[TMP1]], label %[[BB3:[A-z0-9]*]], label %[[BB2:[A-z0-9]*]]
; CHECK:  [[BB2]]:
; CHECK:    [[TMP2:%[A-z0-9]*]] = bitcast float [[B]] to i32
; CHECK:    [[TMP3:%[A-z0-9]*]] = and i32 [[TMP2]], 2139095040
; CHECK:    [[TMP4:%[A-z0-9]*]] = icmp eq i32 [[TMP3]], 0
; CHECK:    [[TMP5:%[A-z0-9]*]] = select i1 [[TMP4]], float 0x41F0000000000000, float 1.000000e+00
; CHECK:    [[TMP6:%[A-z0-9]*]] = icmp uge i32 [[TMP3]], 1677721600
; CHECK:    [[TMP7:%[A-z0-9]*]] = select i1 [[TMP6]], float 0x3DF0000000000000, float [[TMP5]]
; CHECK:    [[TMP8:%[A-z0-9]*]] = fmul float [[B]], [[TMP7]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = fdiv float 1.000000e+00, [[TMP8]]
; CHECK:    [[TMP10:%[A-z0-9]*]] = fmul float [[TMP9]], [[A]]
; CHECK:    [[TMP11:%[A-z0-9]*]] = fmul float [[TMP10]], [[TMP7]]
; CHECK:  [[BB3]]:
; CHECK:    [[TMP12:%[A-z0-9]*]] = phi float [ 1.000000e+00, %[[BB1:[A-z0-9]*]] ], [ [[TMP11]], %[[BB2]] ]
; CHECK:    call void @use.f32(float [[TMP12]])
; CHECK:    ret void
;
  %1 = fdiv float %a, %b
  call void @use.f32(float %1)
  ret void
}

define void @test_fdiv_arcp(float %a, float %b) {
; CHECK-LABEL: @test_fdiv_arcp(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fcmp arcp oeq float [[A:%[A-z0-9]*]], [[B:%[A-z0-9]*]]
; CHECK:    br i1 [[TMP1]], label %[[BB3:[A-z0-9]*]], label %[[BB2:[A-z0-9]*]]
; CHECK:  [[BB2]]:
; CHECK:    [[TMP2:%[A-z0-9]*]] = fdiv arcp float 1.000000e+00, [[B]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = fmul arcp float [[TMP2]], [[A]]
; CHECK:  [[BB3]]:
; CHECK:    [[TMP4:%[A-z0-9]*]] = phi {{(arcp )?}}float [ 1.000000e+00, %[[BB1:[A-z0-9]*]] ], [ [[TMP3]], %[[BB2]] ]
; CHECK:    call void @use.f32(float [[TMP4]])
; CHECK:    ret void
;
  %1 = fdiv arcp float %a, %b
  call void @use.f32(float %1)
  ret void
}

define void @test_fdiv_half(half %a, half %b) {
; CHECK-LABEL: @test_fdiv_half(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fcmp oeq half [[A:%[A-z0-9]*]], [[B:%[A-z0-9]*]]
; CHECK:    br i1 [[TMP1]], label %[[BB3:[A-z0-9]*]], label %[[BB2:[A-z0-9]*]]
; CHECK:  [[BB2]]:
; CHECK:    [[TMP2:%[A-z0-9]*]] = fpext half [[B]] to float
; CHECK:    [[TMP3:%[A-z0-9]*]] = fdiv float 1.000000e+00, [[TMP2]]
; CHECK:    [[TMP4:%[A-z0-9]*]] = fpext half [[A]] to float
; CHECK:    [[TMP5:%[A-z0-9]*]] = fmul float [[TMP3]], [[TMP4]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = fptrunc float [[TMP5]] to half
; CHECK:  [[BB3]]:
; CHECK:    [[TMP7:%[A-z0-9]*]] = phi half [ 0xH3C00, %[[BB1:[A-z0-9]*]] ], [ [[TMP6]], %[[BB2]] ]
; CHECK:    call void @use.f16(half [[TMP7]])
; CHECK:    ret void
;
  %1 = fdiv half %a, %b
  call void @use.f16(half %1)
  ret void
}

declare void @use.f32(float)
declare void @use.f16(half)
