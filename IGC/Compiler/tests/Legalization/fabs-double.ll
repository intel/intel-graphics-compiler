;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -platformdg2 -igc-legalization -S < %s | FileCheck %s --check-prefix=NOFP64
; RUN: igc_opt --opaque-pointers -platformpvc -igc-legalization -S < %s | FileCheck %s --check-prefix=FP64
; ------------------------------------------------
; Legalization: fabs of double
; ------------------------------------------------

; Without FP64 instructions a double move is emitted as two :ud moves, where an
; (abs) source modifier doesn't clear the sign bit. fabs of double is legalized
; into an and of the high dword instead.

define double @test_fabs(double %x) {
; NOFP64-LABEL: define double @test_fabs(
; NOFP64-SAME:  double [[X:%.*]]) {
; NOFP64:         [[V:%.*]] = bitcast double [[X]] to <2 x i32>
; NOFP64-NEXT:    [[HI:%.*]] = extractelement <2 x i32> [[V]], i32 1
; NOFP64-NEXT:    [[ABS:%.*]] = and i32 [[HI]], 2147483647
; NOFP64-NEXT:    [[R:%.*]] = insertelement <2 x i32> [[V]], i32 [[ABS]], i32 1
; NOFP64-NEXT:    [[D:%.*]] = bitcast <2 x i32> [[R]] to double
; NOFP64-NEXT:    ret double [[D]]
;
; FP64-LABEL: define double @test_fabs(
; FP64:         call double @llvm.fabs.f64(
;
  %r = call double @llvm.fabs.f64(double %x)
  ret double %r
}

define <2 x double> @test_fabs_vec(<2 x double> %x) {
; NOFP64-LABEL: define <2 x double> @test_fabs_vec(
; NOFP64-SAME:  <2 x double> [[X:%.*]]) {
; NOFP64:         [[E0:%.*]] = extractelement <2 x double> [[X]], i32 0
; NOFP64-NEXT:    [[V0:%.*]] = bitcast double [[E0]] to <2 x i32>
; NOFP64-NEXT:    [[HI0:%.*]] = extractelement <2 x i32> [[V0]], i32 1
; NOFP64-NEXT:    [[ABS0:%.*]] = and i32 [[HI0]], 2147483647
; NOFP64-NEXT:    [[R0:%.*]] = insertelement <2 x i32> [[V0]], i32 [[ABS0]], i32 1
; NOFP64-NEXT:    [[D0:%.*]] = bitcast <2 x i32> [[R0]] to double
; NOFP64-NEXT:    [[VEC0:%.*]] = insertelement <2 x double> poison, double [[D0]], i32 0
; NOFP64-NEXT:    [[E1:%.*]] = extractelement <2 x double> [[X]], i32 1
; NOFP64-NEXT:    [[V1:%.*]] = bitcast double [[E1]] to <2 x i32>
; NOFP64-NEXT:    [[HI1:%.*]] = extractelement <2 x i32> [[V1]], i32 1
; NOFP64-NEXT:    [[ABS1:%.*]] = and i32 [[HI1]], 2147483647
; NOFP64-NEXT:    [[R1:%.*]] = insertelement <2 x i32> [[V1]], i32 [[ABS1]], i32 1
; NOFP64-NEXT:    [[D1:%.*]] = bitcast <2 x i32> [[R1]] to double
; NOFP64-NEXT:    [[VEC1:%.*]] = insertelement <2 x double> [[VEC0]], double [[D1]], i32 1
; NOFP64-NEXT:    ret <2 x double> [[VEC1]]
;
; FP64-LABEL: define <2 x double> @test_fabs_vec(
; FP64:         call <2 x double> @llvm.fabs.v2f64(
;
  %r = call <2 x double> @llvm.fabs.v2f64(<2 x double> %x)
  ret <2 x double> %r
}

; Only double is affected.
define float @test_fabs_float(float %x) {
; NOFP64-LABEL: define float @test_fabs_float(
; NOFP64:         [[R:%.*]] = call float @llvm.fabs.f32(float %x)
; NOFP64-NEXT:    ret float [[R]]
;
  %r = call float @llvm.fabs.f32(float %x)
  ret float %r
}

declare double @llvm.fabs.f64(double)
declare <2 x double> @llvm.fabs.v2f64(<2 x double>)
declare float @llvm.fabs.f32(float)

!igc.functions = !{!0, !1, !2}
!0 = !{ptr @test_fabs, !3}
!1 = !{ptr @test_fabs_vec, !3}
!2 = !{ptr @test_fabs_float, !3}
!3 = !{}
