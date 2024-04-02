;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 -regkey ForceEmuKind=8 -platformdg2 -igc-precompiled-import -S < %s | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------
; REQUIRES: regkeys

define void @test_fdiv(float %a, float %b) {
; CHECK-LABEL: @test_fdiv(
; CHECK:    [[TMP1:%[A-z0-9]*]] = call float @__igcbuiltin_sp_div(float %a, float %b, i32 {{[0-1]*}}, i32 {{[0-1]*}})
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fdiv float %a, %b
  call void @use.f32(float %1)
  ret void
}

define void @test_fdiv_reciprocal(float %a) {
; CHECK-LABEL: @test_fdiv_reciprocal(
; CHECK:    [[TMP1:%[A-z0-9]*]] = fdiv float 1.000000e+00, %a
; CHECK:    call void @use.f32(float [[TMP1]])
; CHECK:    ret void
;
  %1 = fdiv float 1.0, %a
  call void @use.f32(float %1)
  ret void
}

declare void @use.f32(float)
; CHECK: define internal float @__igcbuiltin_sp_div(float{{.*}}, float{{.*}}, i32{{.*}}, i32{{.*}}) #0 {