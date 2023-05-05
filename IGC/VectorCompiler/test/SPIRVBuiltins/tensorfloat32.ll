;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=XeHPC -S < %s | FileCheck %s

declare spir_func float @_Z27__spirv_RoundFToTF32INTELf(float) #0
declare spir_func <16 x float> @_Z27__spirv_RoundFToTF32INTELDv16_f(<16 x float>) #0

define float @scalar_to(float %arg) {
  ; CHECK: [[ROUND:%[^ ]+]] = call i32 @llvm.vc.internal.round.to.tf32.i32.f32(float %arg)
  ; CHECK: %res = bitcast i32 [[ROUND]] to float
  %res = call float @_Z27__spirv_RoundFToTF32INTELf(float %arg)
  ret float %res
}

define <16 x float> @vector_to(<16 x float> %arg) {
  ; CHECK: [[VROUND:%[^ ]+]] = call <16 x i32> @llvm.vc.internal.round.to.tf32.v16i32.v16f32(<16 x float> %arg)
  ; CHECK: %res = bitcast <16 x i32> [[VROUND]] to <16 x float>
  %res = call <16 x float> @_Z27__spirv_RoundFToTF32INTELDv16_f(<16 x float> %arg)
  ret <16 x float> %res
}

attributes #0 = { nounwind }
