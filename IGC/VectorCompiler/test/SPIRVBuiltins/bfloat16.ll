;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s

declare spir_func float @_Z27__spirv_ConvertBF16ToFINTELs(i16) #0
declare spir_func <16 x float> @_Z27__spirv_ConvertBF16ToFINTELDv16_s(<16 x i16>) #0

declare spir_func i16 @_Z27__spirv_ConvertFToBF16INTELf(float) #0
declare spir_func <16 x i16> @_Z27__spirv_ConvertFToBF16INTELDv16_f(<16 x float>) #0

; CHECK-LABEL: scalar_from
define float @scalar_from(i16 %arg) {
  ; CHECK: [[CAST:%[^ ]+]] = bitcast i16 %arg to bfloat
  ; CHECK: %res = fpext bfloat [[CAST]] to float
  %res = call float @_Z27__spirv_ConvertBF16ToFINTELs(i16 %arg)
  ret float %res
}

; CHECK-LABEL: vector_from
define <16 x float> @vector_from(<16 x i16> %arg) {
  ; CHECK: [[CAST:%[^ ]+]] = bitcast <16 x i16> %arg to <16 x bfloat>
  ; CHECK: %res = fpext <16 x bfloat> [[CAST]] to <16 x float>
  %res = call <16 x float> @_Z27__spirv_ConvertBF16ToFINTELDv16_s(<16 x i16> %arg)
  ret <16 x float> %res
}

; CHECK-LABEL: scalar_to
define i16 @scalar_to(float %arg) {
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc float %arg to bfloat
  ; CHECK: %res = bitcast bfloat [[TRUNC]] to i16
  %res = call i16 @_Z27__spirv_ConvertFToBF16INTELf(float %arg)
  ret i16 %res
}

; CHECK-LABEL: vector_to
define <16 x i16> @vector_to(<16 x float> %arg) {
  ; CHECK: [[TRUNC:%[^ ]+]] = fptrunc <16 x float> %arg to <16 x bfloat>
  ; CHECK: %res = bitcast <16 x bfloat> [[TRUNC]] to <16 x i16>
  %res = call <16 x i16> @_Z27__spirv_ConvertFToBF16INTELDv16_f(<16 x float> %arg)
  ret <16 x i16> %res
}

attributes #0 = { nounwind }
