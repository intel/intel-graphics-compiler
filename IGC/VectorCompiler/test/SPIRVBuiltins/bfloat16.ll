;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s

declare spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16) #0
declare spir_func <16 x float> @_Z27__spirv_ConvertBF16ToFINTELDv16_s(<16 x i16>) #0

declare spir_func i16 @_Z27__spirv_ConvertFToBF16INTELf(float) #0
declare spir_func <16 x i16> @_Z27__spirv_ConvertFToBF16INTELDv16_f(<16 x float>) #0

define float @scalar_from(i16 %arg) {
  ; CHECK: %res = call float @llvm.vc.internal.cast.from.bf16.f32.i16(i16 %arg)
  %res = call float @_Z27__spirv_ConvertBF16ToFINTELs(i16 %arg)
  ret float %res
}

define <16 x float> @vectorr_from(<16 x i16> %arg) {
  ; CHECK: %res = call <16 x float> @llvm.vc.internal.cast.from.bf16.v16f32.v16i16(<16 x i16> %arg)
  %res = call <16 x float> @_Z27__spirv_ConvertBF16ToFINTELDv16_s(<16 x i16> %arg)
  ret <16 x float> %res
}

define i16 @scalar_to(float %arg) {
  ; CHECK: %res = call i16 @llvm.vc.internal.cast.to.bf16.i16.f32(float %arg)
  %res = call i16 @_Z27__spirv_ConvertFToBF16INTELf(float %arg)
  ret i16 %res
}

define <16 x i16> @vector_to(<16 x float> %arg) {
  ; CHECK: %res = call <16 x i16> @llvm.vc.internal.cast.to.bf16.v16i16.v16f32(<16 x float> %arg)
  %res = call <16 x i16> @_Z27__spirv_ConvertFToBF16INTELDv16_f(<16 x float> %arg)
  ret <16 x i16> %res
}

attributes #0 = { nounwind }
