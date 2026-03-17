;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; COM: Khronos SPIRV-LLVM Translator incorrectly requires SPV_INTEL_bfloat16_arithmetic extension to be enabled if bfloat conversions are present.
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s
; COM: Execute ocloc second time, this time without DumpVISAASMToConsole flag, to ensure that E2E compilation does not crash.
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16' -cl-intel-library-compilation"

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .function "OpPhi
define spir_func void @OpPhi(bfloat %data1, bfloat %data2) {
  br label %blockA
blockA:
  br label %phi
blockB:
  br label %phi
phi:
  %OpPhi = phi bfloat [ %data1, %blockA ], [ %data2, %blockB ]
  ret void
}

; CHECK-LABEL: .function "OpReturnValue
define spir_func bfloat @OpReturnValue(bfloat %OpReturnValue) {
  ret bfloat %OpReturnValue
}

; Below conversions are tested in IGC/ocloc_tests/features/bfloat/test_bfloat_unary.ll
; This test only confirms that they don't crash the compilation.

; CHECK-LABEL: .function "OpConvertFToU
define spir_func i32 @OpConvertFToU(bfloat %data) {
  %result = fptoui bfloat %data to i32
  ret i32 %result
}

; CHECK-LABEL: .function "OpConvertFToS
define spir_func i32 @OpConvertFToS(bfloat %data) {
  %result = fptosi bfloat %data to i32
  ret i32 %result
}

; CHECK-LABEL: .function "OpConvertSToF
define spir_func bfloat @OpConvertSToF(i32 %data) {
  %result = sitofp i32 %data to bfloat
  ret bfloat %result
}

; CHECK-LABEL: .function "OpConvertUToF
define spir_func bfloat @OpConvertUToF(i32 %data) {
  %result = uitofp i32 %data to bfloat
  ret bfloat %result
}
