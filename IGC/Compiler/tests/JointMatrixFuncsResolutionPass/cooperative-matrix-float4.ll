;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -igc-joint-matrix-resolution --platformCri 2>&1 | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
;
; The i4 element type is only storage, so the operands mask 196608 (0x30000 =
; MatrixAFP4S1E2M1ComponentsINTEL | MatrixBFP4S1E2M1ComponentsINTEL) is what
; selects E2M1 (13) over an integer precision.

; CHECK-LABEL: @mad_dpas_float4(
; CHECK:         %{{.*}} = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> {{.*}}, <8 x i16> {{.*}}, <8 x i32> {{.*}}, i32 13, i32 13, i32 8, i32 8, i1 false)
; CHECK-NEXT:    ret void
; CHECK-NOT: error:

define spir_kernel void @mad_dpas_float4() {
  %1 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z26__spirv_CompositeConstructc(i8 1)
  %2 = call spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z27__spirv_CompositeConstructc(i8 2)
  %3 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf(float 3.0)
  %4 = call spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS138__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS139__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS142__spirv_CooperativeMatrixKHR__float_3_8_16_2i(target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) %1, target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) %2, target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) %3, i32 196608)
  ret void
}

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0) @_Z26__spirv_CompositeConstructc(i8)

declare spir_func target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1) @_Z27__spirv_CompositeConstructc(i8)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z26__spirv_CompositeConstructf(float)

declare spir_func target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2) @_Z34__spirv_CooperativeMatrixMulAddKHRPU3AS138__spirv_CooperativeMatrixKHR__i4_3_8_64_0PU3AS139__spirv_CooperativeMatrixKHR__i4_3_64_16_1PU3AS142__spirv_CooperativeMatrixKHR__float_3_8_16_2i(target("spirv.CooperativeMatrixKHR", i4, 3, 8, 64, 0), target("spirv.CooperativeMatrixKHR", i4, 3, 64, 16, 1), target("spirv.CooperativeMatrixKHR", float, 3, 8, 16, 2), i32)

!igc.functions = !{!0}
!0 = !{ptr @mad_dpas_float4, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
