;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Xe3P -S < %s | FileCheck %s
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Xe3P -S < %s | FileCheck %s

; RUN: %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_TYPED_PTRS% -march=genx64 -mcpu=Xe3P -S < %s | FileCheck %s
; RUN: %opt_new_pm_opaque -passes=GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_BIF_OPAQUE_PTRS% -march=genx64 -mcpu=Xe3P -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"
; COM: datalayout should stay the same
; CHECK: target datalayout = "e-p:64:64-i64:64-n8:16:32"

declare spir_func <7 x float> @_Z23__spirv_ocl_tanhDv7_f(<7 x float>)
declare spir_func <7 x half> @_Z23__spirv_ocl_tanhDv7_h(<7 x half>)

; CHECK-LABEL: @tanh_float
define spir_func <7 x float> @tanh_float(<7 x float> %src) {
  ; CHECK: %res = call <7 x float> @llvm.vc.internal.tanh.v7f32(<7 x float> %src)
  %res = call <7 x float> @_Z23__spirv_ocl_tanhDv7_f(<7 x float> %src)
  ret <7 x float> %res
}

; CHECK-LABEL: @tanh_half
define spir_func <7 x half> @tanh_half(<7 x half> %src) {
  ; CHECK: %res = call <7 x half> @llvm.vc.internal.tanh.v7f16(<7 x half> %src)
  %res = call <7 x half> @_Z23__spirv_ocl_tanhDv7_h(<7 x half> %src)
  ret <7 x half> %res
}
