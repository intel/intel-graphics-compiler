;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -igc-correctly-rounded-div-sqrt -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CorrectlyRoundedDivSqrt
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_fdiv(float %src1, float %src2) {
; Testcase 1
; Check that scalar fdiv is substituted by call
;
; CHECK: [[FDIV_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32(float %src1, float %src2)

  %1 = fdiv float %src1, %src2

; Testcase 2
; Check that vector fdiv is substituted by call and insertelement value is correct
;
; CHECK: [[VSRC1_V:%[0-9]*]] = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src1, i32 0
; CHECK: [[VSRC2_V:%[0-9]*]] = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src2, i32 1
; CHECK: [[SRC1_V:%[0-9]*]] = extractelement <2 x float> [[VSRC1_V]], i64 0
; CHECK: [[SRC2_V:%[0-9]*]] = extractelement <2 x float> [[VSRC2_V]], i64 0
; CHECK: [[FDIV_VEC_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32(float [[SRC1_V]], float [[SRC2_V]])
; CHECK-NEXT: [[INSELM_V:%[0-9]*]] = insertelement <2 x float> undef, float [[FDIV_VEC_V]], i64 0
; CHECK: [[SRC1_V:%[0-9]*]] = extractelement <2 x float> [[VSRC1_V]], i64 1
; CHECK: [[SRC2_V:%[0-9]*]] = extractelement <2 x float> [[VSRC2_V]], i64 1
; CHECK: [[FDIV_VEC_V:%[0-9]*]] = call float @__builtin_spirv_divide_cr_f32_f32(float [[SRC1_V]], float [[SRC2_V]])
; CHECK-NEXT: {{%[0-9]*}} = insertelement <2 x float> [[INSELM_V]], float [[FDIV_VEC_V]], i64 1

  %2 = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src1, i32 0
  %3 = insertelement <2 x float> <float 3.000000e+00, float 2.000000e+00>, float %src2, i32 1
  %4 = fdiv <2 x float> %2, %3

; Testcase 3
; Check that sqrt calls are renamed
;
; CHECK: call float @_Z19__spirv_ocl_sqrt_cr_f32(float %src1)
; CHECK: call float @_Z7sqrt_cr_f32(float %src2)

  %5 = call float @__builtin_spirv_OpenCL_sqrt_f32(float %src1)
  %6 = call float @_Z16__spirv_ocl_sqrt_f32(float %src1)
  %7 = call float @_Z4sqrt_f32(float %src2)
  ret void
}

declare spir_func float @__builtin_spirv_OpenCL_sqrt_f32(float)

declare spir_func float @_Z16__spirv_ocl_sqrt_f32(float)

declare spir_func float @_Z4sqrt_f32(float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"CorrectlyRoundedDivSqrt", i1 true}
