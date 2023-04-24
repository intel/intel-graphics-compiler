;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=%SPV_CHECK_PREFIX%,CHECK

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double>)

define spir_func <16 x double> @spirv_log_vec_dbl(<16 x double> %arg) {
  %res = call spir_func <16 x double> @_Z15__spirv_ocl_logDv16_d(<16 x double> %arg)
  ret <16 x double> %res
}

declare spir_func <16 x double> @_Z17__spirv_ocl_log10Dv16_d(<16 x double>)

define spir_func <16 x double> @spirv_log10_vec_dbl(<16 x double> %arg) {
  %res = call spir_func <16 x double> @_Z17__spirv_ocl_log10Dv16_d(<16 x double> %arg)
  ret <16 x double> %res
}

declare spir_func <16 x double> @_Z18__spirv_ocl_sincosDv16_dPS_(<16 x double>, <16 x double>*)

define spir_func <16 x double> @spirv_sincos_vec_dbl(<16 x double> %arg1,  <16 x double>* %arg2) {
  %res = call spir_func <16 x double> @_Z18__spirv_ocl_sincosDv16_dPS_(<16 x double> %arg1, <16 x double>* %arg2)
  ret <16 x double> %res
}

declare spir_func i32 @_Z20__spirv_ocl_popcounti(i32)

define spir_func i32 @spirv_popcount_scalar_dbl(i32 %arg1) {
  %res = call spir_func i32 @_Z20__spirv_ocl_popcounti(i32 %arg1)
  ret i32 %res
}

declare spir_func i32 @_Z15__spirv_ocl_ctzi(i32)

define spir_func i32 @spirv_ctzi_scalar_dbl(i32 %arg1) {
  %res = call spir_func i32 @_Z15__spirv_ocl_ctzi(i32 %arg1)
  ret i32 %res
}

declare spir_func i64 @_Z15__spirv_ocl_ctzl(i64)

define spir_func i64 @spirv_ctzl_scalar_dbl(i64 %arg1) {
  %res = call spir_func i64 @_Z15__spirv_ocl_ctzl(i64 %arg1)
  ret i64 %res
}

declare spir_func float @_Z15__spirv_ocl_madfff(float, float, float)

define spir_func float @spirv_mad_scalar_dbl(float %f1, float %f2, float %f3) {
  %res = call spir_func float @_Z15__spirv_ocl_madfff(float %f1, float %f2, float %f3)
  ret float %res
}

declare spir_func <16 x float> @_Z15__spirv_ocl_madDv16_fS_S_(<16 x float>, <16 x float>, <16 x float>)

define spir_func <16 x float> @spirv_mad_vector_dbl(<16 x float> %f1, <16 x float> %f2, <16 x float> %f3) {
  %res = call spir_func <16 x float> @_Z15__spirv_ocl_madDv16_fS_S_(<16 x float> %f1, <16 x float> %f2, <16 x float> %f3)
  ret <16 x float> %res
}

declare spir_func <8 x double> @_Z15__spirv_ocl_fmaDv8_dS_S_(<8 x double>, <8 x double>, <8 x double>)

define spir_func <8 x double> @spirv_fma_vector_dbl(<8 x double> %d1, <8 x double> %d2, <8 x double> %d3) {
  %res = call spir_func <8 x double> @_Z15__spirv_ocl_fmaDv8_dS_S_(<8 x double> %d1, <8 x double> %d2, <8 x double> %d3)
  ret <8 x double> %res
}

declare spir_func double @_Z16__spirv_ocl_sqrtd(double);

define spir_func double @spirv_sqrt_scalar_dbl(double %d) {
  %res = call spir_func double @_Z16__spirv_ocl_sqrtd(double %d)
  ret double %res
}

declare spir_func <8 x double> @_Z16__spirv_ocl_fminDv8_dS_(<8 x double>, <8 x double>)

define spir_func <8 x double> @spirv_fmin_vector_dbl(<8 x double> %d1, <8 x double> %d2) {
  %res = call spir_func <8 x double> @_Z16__spirv_ocl_fminDv8_dS_(<8 x double> %d1, <8 x double> %d2)
  ret <8 x double> %res
}

declare spir_func <2 x float> @_Z23__spirv_ocl_native_log2Dv2_f(<2 x float>);

define spir_func <2 x float> @spirv_native_log2_vector_dbl(<2 x float> %f) {
  %res = call spir_func <2 x float> @_Z23__spirv_ocl_native_log2Dv2_f(<2 x float> %f)
  ret <2 x float> %res
}

; CHECK-LEGACY-LABEL: define internal spir_func {{(noundef )?}}<16 x double> @_Z15__spirv_ocl_logDv16_d
; CHECK-LEGACY-SAME: (<16 x double> {{(noundef )?}}[[ARG_LOG:%[^ ]+]])
; CHECK-LEGACY: [[LOG_CALL:%[^ ]+]] = tail call spir_func {{(noundef )?}}<16 x double> @__builtin_spirv_OpenCL_log_v16f64(<16 x double> {{(noundef )?}}[[ARG_LOG]])
; CHECK-LEGACY-NEXT: ret <16 x double> [[LOG_CALL]]

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-KHR-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_log_v16f64

; CHECK-LEGACY-LABEL: define internal spir_func {{(noundef )?}}<16 x double> @_Z17__spirv_ocl_log10Dv16_d
; CHECK-LEGACY-SAME: (<16 x double> {{(noundef )?}}[[LOG10_ARG:%[^ ]+]])
; CHECK-LEGACY: [[LOG10_CALL:%[^ ]+]] = tail call spir_func {{(noundef )?}}<16 x double> @__builtin_spirv_OpenCL_log10_v16f64(<16 x double> {{(noundef )?}}[[LOG10_ARG]])
; CHECK-LEGACY-NEXT: ret <16 x double> [[LOG10_CALL]]

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-KHR-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_log10_v16f64

; CHECK-LEGACY-LABEL: define internal spir_func {{(noundef )?}}<16 x double> @_Z18__spirv_ocl_sincosDv16_dPS_
; CHECK-LEGACY-SAME: (<16 x double> {{(noundef )?}}[[SINCOS_ARG1:%[^ ]+]], <16 x double>* {{(noundef )?}}[[SINCOS_ARG2:%[^ ]+]])
; CHECK-LEGACY: [[SINCOS_CALL:%[^ ]+]] = tail call spir_func {{(noundef )?}}<16 x double> @__builtin_spirv_OpenCL_sincos_v16f64_p0v16f64(<16 x double> {{(noundef )?}}[[SINCOS_ARG1]], <16 x double>* {{(noundef )?}}[[SINCOS_ARG2]])
; CHECK-LEGACY-NEXT: ret <16 x double> [[SINCOS_CALL]]

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-KHR-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_sincos_v16f64_p0v16f64

; CHECK-LABEL: define internal spir_func {{(noundef )?}}i32 @_Z20__spirv_ocl_popcounti
; CHECK-SAME: (i32 {{(noundef )?}}[[POPCOUNT_ARG:%[^ )]+]])
; CHECK: [[CBIT_CALL:%[^ ]+]] = tail call i32 @llvm.genx.cbit.i32.i32(i32 [[POPCOUNT_ARG]])
; CHECK-NEXT: ret i32 [[CBIT_CALL]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}double @_Z16__spirv_ocl_sqrtd
; CHECK-SAME: (double {{(noundef )?}}[[SQRT_ARG:%[^ ]+]])
; CHECK: [[SQRT_CALL:%[^ ]+]] = tail call double @llvm.sqrt.f64(double [[SQRT_ARG]])
; CHECK-NEXT: ret double [[SQRT_CALL]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}i32 @_Z15__spirv_ocl_ctzi
; CHECK-SAME: (i32 {{(noundef )?}}[[CTZI_ARG:%[^ ]+]])
; CHECK: [[REV:%[^ ]+]] = tail call i32 @llvm.genx.bfrev.i32(i32 [[CTZI_ARG]])
; CHECK-NEXT: [[LZD_CALL:%[^ ]+]] = tail call i32 @llvm.genx.lzd.i32(i32 [[REV]])
; CHECK-NEXT: ret i32 [[LZD_CALL]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}i64 @_Z15__spirv_ocl_ctzl
; CHECK-SAME: (i64 {{(noundef )?}}[[CTZL_ARG:%[^ ]+]])
; CHECK: [[CAST:%[^ ]+]] = bitcast i64 [[CTZL_ARG]] to <2 x i32>
; CHECK-NEXT: [[EXT1:%[^ ]+]] = extractelement <2 x i32> [[CAST]], [[INDEX_TYPE:i(16|32|64)]] 0
; CHECK-NEXT: [[BFREV1:%[^ ]+]] = tail call i32 @llvm.genx.bfrev.i32(i32 [[EXT1]])
; CHECK-NEXT: [[LZD1:%[^ ]+]] = tail call i32 @llvm.genx.lzd.i32(i32 [[BFREV1]])
; CHECK-NEXT: [[EXT2:%[^ ]+]] = extractelement <2 x i32> [[CAST]], [[INDEX_TYPE]] 1
; CHECK-NEXT: [[BFREV2:%[^ ]+]] = tail call i32 @llvm.genx.bfrev.i32(i32 [[EXT2]])
; CHECK-NEXT: [[LZD2:%[^ ]+]] = tail call i32 @llvm.genx.lzd.i32(i32 [[BFREV2]])
; CHECK-NEXT: [[CMP_ZERO:%[^ ]+]] = icmp eq i32 [[EXT1]], 0
; CHECK-NEXT: [[SELECT_ZERO:%[^ ]+]] = select i1 [[CMP_ZERO]], i32 [[LZD2]], i32 0
; CHECK-NEXT: [[ADD_ZERO:%[^ ]+]] = add i32 [[SELECT_ZERO]], [[LZD1]]
; CHECK-NEXT: [[ZEXT_RES:%[^ ]+]] = zext i32 [[ADD_ZERO]] to i64
; CHECK-NEXT: ret i64 [[ZEXT_RES]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}<8 x double> @_Z16__spirv_ocl_fminDv8_dS_
; CHECK-SAME: (<8 x double> {{(noundef )?}}[[FMIN_ARG1:%[^ ]+]], <8 x double> {{(noundef )?}}[[FMIN_ARG2:%[^ ]+]]
; CHECK: [[MINNUM_CALL:%[^ ]+]] = tail call <8 x double> @llvm.minnum.v8f64(<8 x double> [[FMIN_ARG1]], <8 x double> [[FMIN_ARG2]]
; CHECK-NEXT: ret <8 x double> [[MINNUM_CALL]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}float @_Z15__spirv_ocl_madfff
; CHECK-SAME: (float {{(noundef )?}}[[MAD_ARG1:%[^ ]+]], float {{(noundef )?}}[[MAD_ARG2:%[^ ]+]], float {{(noundef )?}}[[MAD_ARG3:%[^ ]+]])
; CHECK: [[MAD_CALL_SCL:%[^ ]+]] = tail call float @llvm.fma.f32(float [[MAD_ARG1]], float [[MAD_ARG2]], float [[MAD_ARG3]])
; CHECK-NEXT: ret float [[MAD_CALL_SCL]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}<16 x float> @_Z15__spirv_ocl_madDv16_fS_S_
; CHECK-SAME: (<16 x float> {{(noundef )?}}[[MAD_VEC_ARG1:%[^ ]+]], <16 x float> {{(noundef )?}}[[MAD_VEC_ARG2:%[^ ]+]], <16 x float> {{(noundef )?}}[[MAD_VEC_ARG3:%[^ ]+]])
; CHECK: [[MAD_CALL_VEC:%[^ ]+]] = tail call <16 x float> @llvm.fma.v16f32(<16 x float> [[MAD_VEC_ARG1]], <16 x float> [[MAD_VEC_ARG2]], <16 x float> [[MAD_VEC_ARG3]])
; CHECK-NEXT: ret <16 x float> [[MAD_CALL_VEC]]

; CHECK-LABEL: define internal spir_func {{(noundef )?}}<8 x double> @_Z15__spirv_ocl_fmaDv8_dS_S_
; CHECK-SAME: (<8 x double> {{(noundef )?}}[[FMA_ARG1:%[^ ]+]], <8 x double> {{(noundef )?}}[[FMA_ARG2:%[^ ]+]], <8 x double> {{(noundef )?}}[[FMA_ARG3:%[^ ]+]])
; CHECK: [[MAD_CALL_FMA:%[^ ]+]] = tail call <8 x double> @llvm.fma.v8f64(<8 x double> [[FMA_ARG1]], <8 x double> [[FMA_ARG2]], <8 x double> [[FMA_ARG3]])
; CHECK-NEXT: ret <8 x double> [[MAD_CALL_FMA]]
