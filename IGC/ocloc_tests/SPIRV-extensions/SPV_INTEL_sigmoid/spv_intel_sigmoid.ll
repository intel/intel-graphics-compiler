;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, cri-supported, llvm-16-plus

; RUN: llvm-as %s -opaque-pointers -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --opaque-pointers --spirv-ext=+SPV_INTEL_sigmoid,+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic
; RUN: llvm-spirv %t.spv -o %t.spt --opaque-pointers --to-text
; RUN: FileCheck < %t.spt %s --check-prefix=CHECK-SPIRV
; RUN: ocloc -device cri -spirv_input -file %t.spv -options " -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

; CHECK-SPIRV: Capability SigmoidINTEL
; CHECK-SPIRV: Extension "SPV_INTEL_sigmoid"
; CHECK-SPIRV: TypeFloat [[#FP32Ty:]] 32
; CHECK-SPIRV: TypeVector [[#FP32v2Ty:]] [[#FP32Ty]] 2
; CHECK-SPIRV: TypeVector [[#FP32v4Ty:]] [[#FP32Ty]] 4
; CHECK-SPIRV: TypeVector [[#FP32v8Ty:]] [[#FP32Ty]] 8
; CHECK-SPIRV: TypeVector [[#FP32v16Ty:]] [[#FP32Ty]] 16

; CHECK-SPIRV: TypeFloat [[#FP16Ty:]] 16
; CHECK-SPIRV: TypeVector [[#FP16v2Ty:]] [[#FP16Ty]] 2
; CHECK-SPIRV: TypeVector [[#FP16v4Ty:]] [[#FP16Ty]] 4
; CHECK-SPIRV: TypeVector [[#FP16v8Ty:]] [[#FP16Ty]] 8
; CHECK-SPIRV: TypeVector [[#FP16v16Ty:]] [[#FP16Ty]] 16

; CHECK-SPIRV: TypeFloat [[#BF16Ty:]] 16 0
; CHECK-SPIRV: TypeVector [[#BF16v2Ty:]] [[#BF16Ty]] 2
; CHECK-SPIRV: TypeVector [[#BF16v4Ty:]] [[#BF16Ty]] 4
; CHECK-SPIRV: TypeVector [[#BF16v8Ty:]] [[#BF16Ty]] 8
; CHECK-SPIRV: TypeVector [[#BF16v16Ty:]] [[#BF16Ty]] 16

; CHECK-SPIRV: FSigmoidINTEL [[#FP32Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP32v2Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP32v4Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP32v8Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP32v16Ty]]

; CHECK-ASM: math.sigm {{.*}}:f {{.*}}:f

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

define spir_kernel void @sigmoid_float(float %a,
           <2 x float> %in2,
           <4 x float> %in4,
           <8 x float> %in8,
           <16 x float> %in16,
           float* %out,
           <2 x float>* %out2,
           <4 x float>* %out4,
           <8 x float>* %out8,
           <16 x float>* %out16) {
  %s0 = tail call spir_func float @_Z21__spirv_FSigmoidINTELf(float %a)
  %s2 = tail call spir_func <2 x float>  @_Z21__spirv_FSigmoidINTELDv2_f(<2 x float> %in2)
  %s4 = tail call spir_func <4 x float>  @_Z21__spirv_FSigmoidINTELDv4_f(<4 x float> %in4)
  %s8 = tail call spir_func <8 x float>  @_Z21__spirv_FSigmoidINTELDv8_f(<8 x float> %in8)
  %s16 = tail call spir_func <16 x float> @_Z21__spirv_FSigmoidINTELDv16_f(<16 x float> %in16)
  store float %s0,         ptr %out,    align 4
  store <2 x float> %s2,   ptr %out2,   align 4
  store <4 x float> %s4,   ptr %out4,   align 4
  store <8 x float> %s8,   ptr %out8,   align 4
  store <16 x float> %s16, ptr %out16,  align 4
  ret void
}



; CHECK-SPIRV: FSigmoidINTEL [[#FP16Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP16v2Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP16v4Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP16v8Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#FP16v16Ty]]

; CHECK-ASM: math.sigm {{.*}}:hf {{.*}}:hf

define spir_kernel void @sigmoid_half(half %a,
           <2 x half> %in2,
           <4 x half> %in4,
           <8 x half> %in8,
           <16 x half> %in16,
           half* %out,
           <2 x half>* %out2,
           <4 x half>* %out4,
           <8 x half>* %out8,
           <16 x half>* %out16) {
  %s0 = tail call spir_func half @_Z21__spirv_FSigmoidINTELDh(half %a)
  %s2 = tail call spir_func <2 x half>  @_Z21__spirv_FSigmoidINTELDv2_Dh(<2 x half> %in2)
  %s4 = tail call spir_func <4 x half>  @_Z21__spirv_FSigmoidINTELDv4_Dh(<4 x half> %in4)
  %s8 = tail call spir_func <8 x half>  @_Z21__spirv_FSigmoidINTELDv8_Dh(<8 x half> %in8)
  %s16 = tail call spir_func <16 x half> @_Z21__spirv_FSigmoidINTELDv16_Dh(<16 x half> %in16)
  store half %s0,          ptr %out,    align 2
  store <2 x half> %s2,    ptr %out2,   align 2
  store <4 x half> %s4,    ptr %out4,   align 2
  store <8 x half> %s8,    ptr %out8,   align 2
  store <16 x half> %s16,  ptr %out16,  align 2
  ret void
}

; CHECK-SPIRV: FSigmoidINTEL [[#BF16Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#BF16v2Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#BF16v4Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#BF16v8Ty]]
; CHECK-SPIRV: FSigmoidINTEL [[#BF16v16Ty]]

; CHECK-ASM: math.sigm {{.*}}:bf {{.*}}:bf

define spir_kernel void @sigmoid_bfloat(bfloat %a,
           <2 x bfloat> %in2,
           <4 x bfloat> %in4,
           <8 x bfloat> %in8,
           <16 x bfloat> %in16,
           bfloat* %out,
           <2 x bfloat>* %out2,
           <4 x bfloat>* %out4,
           <8 x bfloat>* %out8,
           <16 x bfloat>* %out16) {
  %s0 = tail call spir_func bfloat @_Z21__spirv_FSigmoidINTELDF16b(bfloat %a)
  %s2 = tail call spir_func <2 x bfloat>  @_Z21__spirv_FSigmoidINTELDv2_DF16b(<2 x bfloat> %in2)
  %s4 = tail call spir_func <4 x bfloat>  @_Z21__spirv_FSigmoidINTELDv4_DF16b(<4 x bfloat> %in4)
  %s8 = tail call spir_func <8 x bfloat>  @_Z21__spirv_FSigmoidINTELDv8_DF16b(<8 x bfloat> %in8)
  %s16 = tail call spir_func <16 x bfloat> @_Z21__spirv_FSigmoidINTELDv16_DF16b(<16 x bfloat> %in16)
  store bfloat %s0,          ptr %out,    align 2
  store <2 x bfloat> %s2,    ptr %out2,   align 2
  store <4 x bfloat> %s4,    ptr %out4,   align 2
  store <8 x bfloat> %s8,    ptr %out8,   align 2
  store <16 x bfloat> %s16,  ptr %out16,  align 2
  ret void
}

declare spir_func float @_Z21__spirv_FSigmoidINTELf(float)
declare spir_func <2 x float>  @_Z21__spirv_FSigmoidINTELDv2_f(<2 x float>)
declare spir_func <4 x float>  @_Z21__spirv_FSigmoidINTELDv4_f(<4 x float>)
declare spir_func <8 x float>  @_Z21__spirv_FSigmoidINTELDv8_f(<8 x float>)
declare spir_func <16 x float> @_Z21__spirv_FSigmoidINTELDv16_f(<16 x float>)

declare spir_func half @_Z21__spirv_FSigmoidINTELDh(half)
declare spir_func <2 x half>  @_Z21__spirv_FSigmoidINTELDv2_Dh(<2 x half>)
declare spir_func <4 x half>  @_Z21__spirv_FSigmoidINTELDv4_Dh(<4 x half>)
declare spir_func <8 x half>  @_Z21__spirv_FSigmoidINTELDv8_Dh(<8 x half>)
declare spir_func <16 x half> @_Z21__spirv_FSigmoidINTELDv16_Dh(<16 x half>)

declare spir_func bfloat @_Z21__spirv_FSigmoidINTELDF16b(bfloat)
declare spir_func <2 x bfloat>  @_Z21__spirv_FSigmoidINTELDv2_DF16b(<2 x bfloat>)
declare spir_func <4 x bfloat>  @_Z21__spirv_FSigmoidINTELDv4_DF16b(<4 x bfloat>)
declare spir_func <8 x bfloat>  @_Z21__spirv_FSigmoidINTELDv8_DF16b(<8 x bfloat>)
declare spir_func <16 x bfloat> @_Z21__spirv_FSigmoidINTELDv16_DF16b(<16 x bfloat>)

!opencl.spir.version = !{!0}
!spirv.Source = !{!1}

!0 = !{i32 1, i32 2}
!1 = !{i32 4, i32 100000}
