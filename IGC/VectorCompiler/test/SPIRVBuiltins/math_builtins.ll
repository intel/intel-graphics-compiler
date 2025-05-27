;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 \
; RUN: -S < %s | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 \
; RUN: -S < %s | FileCheck %s

; RUN: %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Gen9 \
; RUN: -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Gen9 \
; RUN: -S < %s | FileCheck %s

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

declare spir_func <2 x float> @_Z23__spirv_ocl_native_log2Dv2_f(<2 x float>);

define spir_func <2 x float> @spirv_native_log2_vector_dbl(<2 x float> %f) {
  %res = call spir_func <2 x float> @_Z23__spirv_ocl_native_log2Dv2_f(<2 x float> %f)
  ret <2 x float> %res
}

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_log_v16f64

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_log10_v16f64

; COM: FIXME: Not much to check there. Remove the case once the switch is done.
; CHECK-NOT: call spir_func <16 x double> @__builtin_spirv_OpenCL_sincos_v16f64_p0v16f64
