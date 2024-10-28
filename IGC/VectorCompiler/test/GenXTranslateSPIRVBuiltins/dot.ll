;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN:  -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN:  -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN:  -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN:  -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------

declare spir_func float @_Z11__spirv_DotDv16_fS_(<16 x float>, <16 x float>)
declare spir_func float @_Z11__spirv_DotDv15_fS_(<15 x float>, <15 x float>)
declare spir_func double @_Z11__spirv_DotDv4_fS_(<4 x double>, <4 x double>)
declare spir_func bfloat @_Z11__spirv_DotDv7_fS_(<7 x bfloat>, <7 x bfloat>)

; CHECK-LABEL: spir_func float @dotFloatConst
define spir_func float @dotFloatConst() {
; CHECK: [[FMUL:%[^ ]*]] = fmul <16 x float>
; CHECK: [[RES:%[^ ]*]] = call reassoc float @llvm.vector.reduce.fadd.v16f32(float 0.000000e+00, <16 x float> [[FMUL]])
; CHECK: ret float [[RES]]
  %alloc = alloca <16 x float>, align 64
  %src1 = load <16 x float>, <16 x float>* %alloc, align 64
  %dot = call spir_func float @_Z11__spirv_DotDv16_fS_(<16 x float> %src1, <16 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  ret float %dot
}

; CHECK-LABEL: spir_func float @dotFloatVal
define spir_func float @dotFloatVal() {
; CHECK: [[FMUL:%[^ ]*]] = fmul <15 x float>
; CHECK: [[RES:%[^ ]*]] = call reassoc float @llvm.vector.reduce.fadd.v15f32(float 0.000000e+00, <15 x float> [[FMUL]])
; CHECK: ret float [[RES]]
  %alloc = alloca <15 x float>, align 64
  %src1 = load <15 x float>, <15 x float>* %alloc, align 64
  %src2 = load <15 x float>, <15 x float>* %alloc, align 64
  %dot = call spir_func float @_Z11__spirv_DotDv15_fS_(<15 x float> %src1, <15 x float> %src2)
  ret float %dot
}

; CHECK-LABEL: spir_func double @dotDoubleVal
define spir_func double @dotDoubleVal() {
; CHECK: [[FMUL:%[^ ]*]] = fmul <4 x double> {{.*}}
; CHECK: [[RES:%[^ ]*]] = call reassoc double @llvm.vector.reduce.fadd.v4f64(double 0.000000e+00, <4 x double> [[FMUL]])
; CHECK: ret double [[RES]]
  %alloc = alloca <4 x double>, align 128
  %src1 = load <4 x double>, <4 x double>* %alloc, align 128
  %src2 = load <4 x double>, <4 x double>* %alloc, align 128
  %dot = call spir_func double @_Z11__spirv_DotDv4_fS_(<4 x double> %src1, <4 x double> %src2)
  ret double %dot
}

; CHECK-LABEL: spir_func bfloat @dotBFloatVal
define spir_func bfloat @dotBFloatVal() {
; CHECK: [[FMUL:%[^ ]*]] = fmul <7 x bfloat> {{.*}}
; CHECK: [[RES:%[^ ]*]] = call reassoc bfloat @llvm.vector.reduce.fadd.v7bf16(bfloat 0xR0000, <7 x bfloat> [[FMUL]])
; CHECK: ret bfloat [[RES]]
  %alloc = alloca <7 x bfloat>, align 128
  %src1 = load <7 x bfloat>, <7 x bfloat>* %alloc, align 32
  %src2 = load <7 x bfloat>, <7 x bfloat>* %alloc, align 32
  %dot = call spir_func bfloat @_Z11__spirv_DotDv7_fS_(<7 x bfloat> %src1, <7 x bfloat> %src2)
  ret bfloat %dot
}
