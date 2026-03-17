;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s
; COM: Execute ocloc second time, this time without DumpVISAASMToConsole flag to ensure that E2E compilation does not crash.
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16' -cl-intel-library-compilation"

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; Modifications to the SPIR-V Specification, Version 1.6
; Validation Rules
; Add validation rules to section 2.16.1 Universal Validation Rules from:
;
; - Variables with a type that is or includes a floating-point type with the BFloat16KHR encoding must only be
;   used with the following instructions, if BFloat16ArithmeticINTEL is declared:
;
;    - Arithmetic Instructions:
;       - OpFNegate
;       - OpFAdd
;       - OpFSub
;       - OpFMul
;       - OpFDiv
;       - OpFRem
;       - OpFMod
;       - OpVectorTimesScalar     // TODO

; CHECK-LABEL: .function "OpFNegate
define spir_func bfloat @OpFNegate(bfloat %data) {
  ; CHECK: mov (M1, 16) [[DST:.*]](0,0)<1> (-)[[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = fneg bfloat %data
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFAdd
define spir_func bfloat @OpFAdd(bfloat %data1, bfloat %data2) {
  ; CHECK: add (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = fadd bfloat %data1, %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFSub
define spir_func bfloat @OpFSub(bfloat %data1, bfloat %data2) {
  ; CHECK: add (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> (-)[[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = fsub bfloat %data1, %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFMul
define spir_func bfloat @OpFMul(bfloat %data1, bfloat %data2) {
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = fmul bfloat %data1, %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFDiv
define spir_func bfloat @OpFDiv(bfloat %data1, bfloat %data2) {
  ; IGC generates float MUL + INV sequence for BF16 division. Can we generate bfloat MUL + INV here?
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC0:.*]](0,0)<1;1,0> [[SRC1:.*]](0,0)<1;1,0>
  ; CHECK: inv (M1, 16) {{.*}}(0,0)<1> [[DST]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC0]] v_type=G type=f
  ; CHECK-DAG: .decl [[SRC1]] v_type=G type=f
  ; CHECK-DAG: .decl [[DST]] v_type=G type=f
  %result = fdiv bfloat %data1, %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFRem
define spir_func bfloat @OpFRem(bfloat %data1, bfloat %data2) {
  ; FRem is upconverted to float. Intentionally no CHECKs here. Checking if compialtion does not crash.
  ; Testing is implemented in IGC/Compiler/tests/HandleFRemInstructions/bfloat.ll
  %result = frem bfloat %data1, %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFMod
define spir_func bfloat @OpFMod(bfloat %data1, bfloat %data2) {
  ; FMod is implemented via FRem, therefore it uses upconvertion to float.
  ; Intentionally no CHECKs here. Checking if compialtion does not crash.
  %result = call spir_func bfloat @_Z12__spirv_FModDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; Below crashes in Khronos SPIRVWriter
;define spir_func <2 x bfloat> @OpVectorTimesScalar(<2 x bfloat> %vec, bfloat %scalar) {
;  ; VectorTimesScalar is upconverted to float. Intentionally no CHECKs here. Checking if compialtion does not crash.
;  %result = call spir_func <2 x bfloat> @_Z25__spirv_VectorTimesScalarDv2_DF16bDF16b(<2 x bfloat> %vec, bfloat %scalar)
;  ret <2 x bfloat> %result
;}

declare spir_func bfloat @_Z12__spirv_FModDF16bDF16b(bfloat, bfloat)
;declare spir_func <2 x bfloat> @_Z25__spirv_VectorTimesScalarDv2_DF16bDF16b(<2 x bfloat>, bfloat)
