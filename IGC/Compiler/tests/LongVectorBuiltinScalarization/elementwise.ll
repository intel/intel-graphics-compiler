;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-long-vector-builtin-scalarization -S < %s | FileCheck %s

; A builtin on a vector width BiF doesn't define becomes one call of the scalar
; BiF variant per element, unrolled up to 16 elements.

target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define spir_func <5 x float> @elementwise(
; CHECK:         [[X0:%.*]] = extractelement <5 x float> %x, i32 0
; CHECK:         [[R0:%.*]] = call spir_func float @_Z16__spirv_ocl_fmaxff(float [[X0]], float %y)
; CHECK:         insertelement <5 x float> poison, float [[R0]], i32 0
; CHECK-COUNT-4: call spir_func float @_Z16__spirv_ocl_fmaxff(
; CHECK-NOT:     call
; CHECK:         ret <5 x float>
define spir_func <5 x float> @elementwise(<5 x float> %x, float %y) {
  %r = call spir_func <5 x float> @_Z16__spirv_ocl_fmaxDv5_ff(<5 x float> %x, float %y)
  ret <5 x float> %r
}

; CHECK-LABEL: define spir_func <1 x float> @single_element(
; CHECK:         [[X:%.*]] = extractelement <1 x float> %x, i32 0
; CHECK:         [[R:%.*]] = call spir_func float @_Z15__spirv_ocl_sinf(float [[X]])
; CHECK:         [[V:%.*]] = insertelement <1 x float> poison, float [[R]], i32 0
; CHECK:         ret <1 x float> [[V]]
define spir_func <1 x float> @single_element(<1 x float> %x) {
  %r = call spir_func <1 x float> @_Z15__spirv_ocl_sinDv1_f(<1 x float> %x)
  ret <1 x float> %r
}

; The scalar relational variant returns bool, the vector one 0 or 1 in i8.
; CHECK-LABEL: define spir_func <5 x i8> @relational(
; CHECK:         [[R:%.*]] = call spir_func i1 @_Z13__spirv_IsNanf(float
; CHECK:         [[E:%.*]] = zext i1 [[R]] to i8
; CHECK:         insertelement <5 x i8> poison, i8 [[E]], i32 0
; CHECK-COUNT-4: call spir_func i1 @_Z13__spirv_IsNanf(
define spir_func <5 x i8> @relational(<5 x float> %x) {
  %r = call spir_func <5 x i8> @_Z13__spirv_IsNanDv5_f(<5 x float> %x)
  ret <5 x i8> %r
}

; CHECK-LABEL: define spir_func <5 x float> @pointer_arg(
; CHECK:         [[P0:%.*]] = getelementptr inbounds float, ptr addrspace(1) %p, i32 0
; CHECK:         call spir_func float @_Z17__spirv_ocl_fractfPU3AS1f(float {{%.*}}, ptr addrspace(1) [[P0]])
; CHECK:         [[P4:%.*]] = getelementptr inbounds float, ptr addrspace(1) %p, i32 4
; CHECK:         call spir_func float @_Z17__spirv_ocl_fractfPU3AS1f(float {{%.*}}, ptr addrspace(1) [[P4]])
define spir_func <5 x float> @pointer_arg(<5 x float> %x, ptr addrspace(1) %p) {
  %r = call spir_func <5 x float> @_Z17__spirv_ocl_fractDv5_fPU3AS1S_(<5 x float> %x, ptr addrspace(1) %p)
  ret <5 x float> %r
}

; CHECK-LABEL: define spir_func <5 x i8> @return_suffix(
; CHECK-COUNT-5: call spir_func i8 @_Z29__spirv_ConvertFToS_Rchar_satf(float
define spir_func <5 x i8> @return_suffix(<5 x float> %x) {
  %r = call spir_func <5 x i8> @_Z30__spirv_ConvertFToS_Rchar5_satDv5_f(<5 x float> %x)
  ret <5 x i8> %r
}

; The return type suffix of a bfloat result is `DF16b`, not an OpenCL type name.
; CHECK-LABEL: define spir_func <5 x bfloat> @bfloat_return_suffix(
; CHECK-COUNT-5: call spir_func bfloat @_Z22__spirv_ocl_nan_RDF16bt(i16
define spir_func <5 x bfloat> @bfloat_return_suffix(<5 x i16> %x) {
  %r = call spir_func <5 x bfloat> @_Z23__spirv_ocl_nan_RDF16b5Dv5_t(<5 x i16> %x)
  ret <5 x bfloat> %r
}

declare spir_func <5 x float> @_Z16__spirv_ocl_fmaxDv5_ff(<5 x float>, float)
declare spir_func <1 x float> @_Z15__spirv_ocl_sinDv1_f(<1 x float>)
declare spir_func <5 x i8> @_Z13__spirv_IsNanDv5_f(<5 x float>)
declare spir_func <5 x float> @_Z17__spirv_ocl_fractDv5_fPU3AS1S_(<5 x float>, ptr addrspace(1))
declare spir_func <5 x i8> @_Z30__spirv_ConvertFToS_Rchar5_satDv5_f(<5 x float>)
declare spir_func <5 x bfloat> @_Z23__spirv_ocl_nan_RDF16b5Dv5_t(<5 x i16>)

; CHECK-NOT: declare {{.*}}Dv1_f(
