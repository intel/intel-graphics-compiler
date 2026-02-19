;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --enable-debugify --igc-fix-sampled-inputs -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; FixSampleDInputs
; ------------------------------------------------

; Test checks that sampleD intrinsic has its arguments corrected to
; format without 3D/Cube/CubeArray

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test(i32* %a, i32* %b, i32* %c) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleDptr.v4f32.f32.p0i32.p0i32.p0i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float 1.100000e+01, float 9.000000e+00, float 1.000000e+01, float 0.000000e+00, i32* [[A:%.*]], i32* [[B:%.*]], i32* [[C:%.*]], i32 0, i32 1, i32 2)
; CHECK:    ret void
;
  %1 = call <4 x float> @llvm.genx.GenISA.sampleDptr.v4f32.f32.p0i32.p0i32.p0i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float 8.000000e+00, float 9.000000e+00, float 1.000000e+01, float 1.100000e+01, i32* %a, i32* %b, i32* %c, i32 0, i32 1, i32 2)
  ret void
}

declare <4 x float> @llvm.genx.GenISA.sampleDptr.v4f32.f32.p0i32.p0i32.p0i32(float, float, float, float, float, float, float, float, float, float, float, i32*, i32*, i32*, i32, i32, i32)
