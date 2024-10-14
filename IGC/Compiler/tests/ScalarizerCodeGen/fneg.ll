;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt -enable-debugify %s -S -o - --igc-scalarizer-in-codegen 2>&1 | FileCheck %s
; ------------------------------------------------
; ScalarizerCodeGen
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Test checks fneg scalarization

define <1 x half> @test_fneg_scalarization_hf(<1 x half> %src) {
; CHECK-LABEL: define <1 x half> @test_fneg_scalarization_hf(
; CHECK-SAME: <1 x half> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <1 x half> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = fneg half [[TMP1]]
; CHECK:    [[TMP3:%.*]] = insertelement <1 x half> undef, half [[TMP2]], i32 0
; CHECK:    ret <1 x half> [[SRC]]
;
  %1 = fneg <1 x half> %src
  ret <1 x half> %src
}

define <2 x bfloat> @test_fneg_scalarization_bf(<2 x bfloat> %src) {
; CHECK-LABEL: define <2 x bfloat> @test_fneg_scalarization_bf(
; CHECK-SAME: <2 x bfloat> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <2 x bfloat> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = fneg bfloat [[TMP1]]
; CHECK:    [[TMP3:%.*]] = insertelement <2 x bfloat> undef, bfloat [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <2 x bfloat> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = fneg bfloat [[TMP4]]
; CHECK:    [[TMP6:%.*]] = insertelement <2 x bfloat> [[TMP3]], bfloat [[TMP5]], i32 1
; CHECK:    ret <2 x bfloat> [[SRC]]
;
  %1 = fneg <2 x bfloat> %src
  ret <2 x bfloat> %src
}

define <3 x float> @test_fneg_scalarization_f(<3 x float> %src) {
; CHECK-LABEL: define <3 x float> @test_fneg_scalarization_f(
; CHECK-SAME: <3 x float> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <3 x float> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = fneg float [[TMP1]]
; CHECK:    [[TMP3:%.*]] = insertelement <3 x float> undef, float [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <3 x float> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = fneg float [[TMP4]]
; CHECK:    [[TMP6:%.*]] = insertelement <3 x float> [[TMP3]], float [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <3 x float> [[SRC]], i32 2
; CHECK:    [[TMP8:%.*]] = fneg float [[TMP7]]
; CHECK:    [[TMP9:%.*]] = insertelement <3 x float> [[TMP6]], float [[TMP8]], i32 2
; CHECK:    ret <3 x float> [[TMP9]]
;
  %1 = fneg <3 x float> %src
  ret <3 x float> %1
}

define <4 x double> @test_fneg_scalarization_d(<4 x double> %src) {
; CHECK-LABEL: define <4 x double> @test_fneg_scalarization_d(
; CHECK-SAME: <4 x double> [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = extractelement <4 x double> [[SRC]], i32 0
; CHECK:    [[TMP2:%.*]] = fneg double [[TMP1]]
; CHECK:    [[TMP3:%.*]] = insertelement <4 x double> undef, double [[TMP2]], i32 0
; CHECK:    [[TMP4:%.*]] = extractelement <4 x double> [[SRC]], i32 1
; CHECK:    [[TMP5:%.*]] = fneg double [[TMP4]]
; CHECK:    [[TMP6:%.*]] = insertelement <4 x double> [[TMP3]], double [[TMP5]], i32 1
; CHECK:    [[TMP7:%.*]] = extractelement <4 x double> [[SRC]], i32 2
; CHECK:    [[TMP8:%.*]] = fneg double [[TMP7]]
; CHECK:    [[TMP9:%.*]] = insertelement <4 x double> [[TMP6]], double [[TMP8]], i32 2
; CHECK:    [[TMP10:%.*]] = extractelement <4 x double> [[SRC]], i32 3
; CHECK:    [[TMP11:%.*]] = fneg double [[TMP10]]
; CHECK:    [[TMP12:%.*]] = insertelement <4 x double> [[TMP9]], double [[TMP11]], i32 3
; CHECK:    ret <4 x double> [[SRC]]
;
  %1 = fneg <4 x double> %src
  ret <4 x double> %src
}

; sanity - do nothing for scalar type

define double @test_fneg_sanity(double %src) {
; CHECK-LABEL: define double @test_fneg_sanity(
; CHECK-SAME: double [[SRC:%.*]])
; CHECK:    [[TMP1:%.*]] = fneg fast double [[SRC]]
; CHECK:    ret double [[SRC]]
;
  %1 = fneg fast double %src
  ret double %src
}
