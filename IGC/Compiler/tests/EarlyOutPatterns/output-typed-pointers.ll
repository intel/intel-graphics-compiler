;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-early-out-patterns-pass -S -inputps < %s | FileCheck %s
; ------------------------------------------------
; EarlyOutPatterns
; ------------------------------------------------

define spir_kernel void @test_earlyout(<4 x float>* %src1, i32* %src2) {
; CHECK-LABEL: @test_earlyout(
; CHECK:    [[TMP1:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float* null, i32* null, i32 1, i32 2, i32 3)
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <4 x float> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = fcmp oeq float [[TMP2]], 0.000000e+00
; CHECK:    br i1 [[TMP3]], label %[[EO_IF:[A-z0-9]*]], label %[[EO_ELSE:[A-z0-9]*]]
; CHECK:  [[EO_ELSE]]:
; CHECK:    [[TMP4:%[A-z0-9]*]] = fmul float [[TMP2]], 1.000000e+00
; CHECK:    [[TMP5:%[A-z0-9]*]] = fmul float [[TMP2]], 1.000000e+00
; CHECK:    br label %[[EO_ENDIF:[A-z0-9]*]]
; CHECK:  [[EO_IF]]:
; CHECK:    [[TMP6:%[A-z0-9]*]] = fmul float [[TMP2]], 1.000000e+00
; CHECK:    [[TMP7:%[A-z0-9]*]] = fmul float [[TMP2]], 1.000000e+00
; CHECK:    br label %[[EO_ENDIF]]
; CHECK:  [[EO_ENDIF]]:
; CHECK:    store <4 x float> [[TMP1]], <4 x float>* [[SRC1:%[A-z0-9]*]]
; CHECK:    call void @llvm.genx.GenISA.OUTPUT.f32(float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float [[TMP2]], i32 1, i32 2, i32 15)
; CHECK:    ret void
;
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, float* null, i32* null, i32 1, i32 2, i32 3)
  %2 = extractelement <4 x float> %1, i32 0
  %3 = fmul float %2, 1.0
  %4 = fmul float %2, 1.0
  %5 = fadd float %2, 1.0
  %6 = fadd float %2, 1.0
  %7 = fadd float %2, 1.0
  %8 = fadd float %2, 1.0
  %9 = fadd float %2, 1.0
  %10 = fadd float %2, 1.0
  %11 = fadd float %2, 1.0
  %12 = fadd float %2, 1.0
  %13 = fadd float %2, 1.0
  %14 = fadd float %2, 1.0
  %15 = fadd float %2, 1.0
  %16 = fadd float %2, 1.0
  %17 = fadd float %2, 1.0
  %18 = fadd float %2, 1.0
  %19 = fadd float %2, 1.0
  %20 = fadd float %2, 1.0
  %21 = fadd float %2, 1.0
  %22 = fadd float %2, 1.0
  %23 = fadd float %2, 1.0
  %24 = fadd float %2, 1.0
  %25 = fadd float %2, 1.0
  %26 = fadd float %2, 1.0
  %27 = fadd float %2, 1.0
  %28 = fadd float %2, 1.0
  %29 = fadd float %2, 1.0
  %30 = fadd float %2, 1.0
  %31 = fadd float %2, 1.0
  %32 = fadd float %2, 1.0
  %33 = fadd float %2, 1.0
  %34 = fadd float %2, 1.0
  %35 = fadd float %2, 1.0
  %36 = fadd float %2, 1.0
  %37 = fadd float %2, 1.0
  %38 = fadd float %2, 1.0
  %39 = fadd float %2, 1.0
  %40 = fadd float %2, 1.0
  %41 = fadd float %2, 1.0
  %42 = fadd float %2, 1.0
  %43 = fadd float %2, 1.0
  %44 = fadd float %2, 1.0
  %45 = fadd float %2, 1.0
  %46 = fadd float %2, 1.0
  %47 = fadd float %2, 1.0
  %48 = fadd float %2, 1.0
  %49 = fadd float %2, 1.0
  %50 = fadd float %2, 1.0
  %51 = fadd float %2, 1.0
  %52 = fadd float %2, 1.0
  %53 = fadd float %2, 1.0
  store <4 x float> %1, <4 x float>* %src1
  call void @llvm.genx.GenISA.OUTPUT.f32(float 0.0, float 0.0, float 0.0, float %2, i32 1, i32 2, i32 15)
  ret void
}

declare void @llvm.genx.GenISA.OUTPUT.f32(float, float, float, float, i32, i32, i32)
declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float, float, float, float, float, float, float*, i32*, i32, i32, i32)
