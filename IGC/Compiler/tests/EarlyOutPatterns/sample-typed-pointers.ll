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

define spir_kernel void @test_earlyout(float* %src1, i32* %src2) {
; CHECK-LABEL: @test_earlyout(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float* [[SRC1:%[A-z0-9]*]], i32* [[SRC2:%[A-z0-9]*]], i32 1, i32 2, i32 3)
; CHECK:    [[TMP1:%[A-z0-9]*]] = extractelement <4 x float> [[TMP0]], i32 0
; CHECK:    [[TMP2:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float* [[SRC1]], i32* [[SRC2]], i32 1, i32 2, i32 3)
; CHECK:    [[TMP3:%[A-z0-9]*]] = extractelement <4 x float> [[TMP2]], i32 0
; CHECK:    [[TMP4:%[A-z0-9]*]] = fadd float [[TMP1]], [[TMP3]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = fcmp oeq float [[TMP4]], 0.000000e+00
; CHECK:    br i1 [[TMP5]], label %[[EO_IF:[A-z0-9]*]], label %[[EO_ELSE:[A-z0-9]*]]
; CHECK:  [[EO_ELSE]]:
; CHECK:    [[TMP6:%[A-z0-9]*]] = bitcast float [[TMP4]] to i32
; CHECK:    [[TMP7:%[A-z0-9]*]] = fmul float [[TMP4]], 1.500000e+01
; CHECK:    [[TMP8:%[A-z0-9]*]] = fmul float [[TMP4]], 1.600000e+01
; CHECK:    [[TMP9:%[A-z0-9]*]] = fmul float [[TMP4]], 1.700000e+01
; CHECK:    [[TMP10:%[A-z0-9]*]] = fmul float [[TMP4]], 1.800000e+01
; CHECK:    [[TMP11:%[A-z0-9]*]] = fmul float [[TMP4]], 1.900000e+01
; CHECK:    [[TMP12:%[A-z0-9]*]] = fmul float [[TMP4]], 2.000000e+01
; CHECK:    [[TMP13:%[A-z0-9]*]] = bitcast i32 [[TMP6]] to float
; CHECK:    br label %[[EO_ENDIF:[A-z0-9]*]]
; CHECK:  [[EO_IF]]:
; CHECK:    [[TMP14:%[A-z0-9]*]] = bitcast float [[TMP4]] to i32
; CHECK:    [[TMP15:%[A-z0-9]*]] = fmul float [[TMP4]], 1.500000e+01
; CHECK:    [[TMP16:%[A-z0-9]*]] = fmul float [[TMP4]], 1.600000e+01
; CHECK:    [[TMP17:%[A-z0-9]*]] = fmul float [[TMP4]], 1.700000e+01
; CHECK:    [[TMP18:%[A-z0-9]*]] = fmul float [[TMP4]], 1.800000e+01
; CHECK:    [[TMP19:%[A-z0-9]*]] = fmul float [[TMP4]], 1.900000e+01
; CHECK:    [[TMP20:%[A-z0-9]*]] = fmul float [[TMP4]], 2.000000e+01
; CHECK:    [[TMP21:%[A-z0-9]*]] = bitcast i32 0 to float
; CHECK:    br label %[[EO_ENDIF]]
; CHECK:  [[EO_ENDIF]]:
; CHECK:    [[TMP22:%[A-z0-9]*]] = phi float [ [[TMP13]], %[[EO_ELSE]] ], [ 0.000000e+00, %[[EO_IF]] ]
; CHECK:    store float [[TMP22]], float* [[SRC1]], align 4
; CHECK:    ret void
;
entry:
  %0 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 1.0, float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, float* %src1, i32* %src2, i32 1, i32 2, i32 3)
  %1 = call <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float 2.0, float 3.0, float 4.0, float 5.0, float 6.0, float 7.0, float* %src1, i32* %src2, i32 1, i32 2, i32 3)
  %2 = extractelement <4 x float> %0, i32 0
  %3 = extractelement <4 x float> %1, i32 0
  %4 = fadd float %2, %3
  %5 = bitcast float %4 to i32
  %6 = fmul float %4, 15.0
  %7 = fmul float %4, 16.0
  %8 = fmul float %4, 17.0
  %9 = fmul float %4, 18.0
  %10 = fmul float %4, 19.0
  %11 = fmul float %4, 20.0
  %12 = bitcast i32 %5 to float
  store float %12, float* %src1, align 4
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleptr.v4f32.f32.f32.i32(float, float, float, float, float, float, float*, i32*, i32, i32, i32)
