;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: cri-supported, llvm-spirv, llvm-below-17

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=16' -cl-intel-library-compilation" | FileCheck %s
; COM: Execute ocloc second time, this time without DumpVISAASMToConsole flag, to ensure that E2E compilation does not crash.
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16' -cl-intel-library-compilation"

; Modifications to the SPIR-V Specification, Version 1.6
; Validation Rules
; Add validation rules to section 2.16.1 Universal Validation Rules from:
;
; - Variables with a type that is or includes a floating-point type with the BFloat16KHR encoding must only be
;   used with the following instructions, if BFloat16ArithmeticINTEL is declared:
;
;    - Relational and Logical Instructions (https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#_relational_and_logical_instructions)

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: .function "OpIsNan
define spir_func i1 @OpIsNan(bfloat %data1) {
  ; CHECK: cmp.ne (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func i1 @_Z13__spirv_IsNanDF16b(bfloat %data1)
  ret i1 %result
}

; CHECK-LABEL: .function "OpIsInf
define spir_func i1 @OpIsInf(bfloat %data1) {
  ; CHECK: cmp.eq (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x7f800000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func i1 @_Z13__spirv_IsInfDF16b(bfloat %data1)
  ret i1 %result
}

; CHECK-LABEL: .function "OpIsFinite
define spir_func i1 @OpIsFinite(bfloat %data1) {
  ; CHECK: cmp.lt (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x7f800000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func i1 @_Z16__spirv_IsFiniteDF16b(bfloat %data1)
  ret i1 %result
}

; CHECK-LABEL: .function "OpIsNormal
define spir_func i1 @OpIsNormal(bfloat %data1) {
  ; CHECK: cmp.ge (M1, 16) {{.*}} [[SRC:.*]](0,0)<1;1,0> 0x800000:f
  ; CHECK: cmp.lt (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> 0x7f800000:f
  ; CHECK: and (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func i1 @_Z16__spirv_IsNormalDF16b(bfloat %data1)
  ret i1 %result
}

; CHECK-LABEL: .function "OpSignBitSet
define spir_func i1 @OpSignBitSet(bfloat %data1) {
  ; CHECK: shr (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0xf:w
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=uw
  %result = call spir_func i1 @_Z18__spirv_SignBitSetDF16b(bfloat %data1)
  ret i1 %result
}

; CHECK-LABEL: .function "OpLessOrGreater
define spir_func i1 @OpLessOrGreater(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.ne (M1, 16) {{.*}} [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func i1 @_Z21__spirv_LessOrGreaterDF16bDF16b(bfloat %data1, bfloat %data2)
  ret i1 %result
}

; CHECK-LABEL: .function "OpOrdered
define spir_func i1 @OpOrdered(bfloat %data1, bfloat %data2) {
  ; CHECK-DAG: cmp.eq (M1, 16) {{.*}} [[SRC:.*]](1,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: cmp.eq (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK: and (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ord bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpUnordered
define spir_func i1 @OpUnordered(bfloat %data1, bfloat %data2) {
  ; CHECK-DAG: cmp.ne (M1, 16) {{.*}} [[SRC:.*]](1,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: cmp.ne (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK: or (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp uno bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpSelect
define spir_func bfloat @OpSelect(i1 %cond, bfloat %data1, bfloat %data2) {
  ; CHECK: sel (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](2,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = select i1 %cond, bfloat %data1, bfloat %data2
  ret bfloat %result
}

; CHECK-LABEL: .function "OpFOrdEqual
define spir_func i1 @OpFOrdEqual(bfloat %data1, bfloat %data2) {
  ; CHECK-DAG: cmp.eq (M1, 16) {{.*}} [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp oeq bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordEqual
define spir_func i1 @OpFUnordEqual(bfloat %data1, bfloat %data2) {
  ; CHECK-DAG: cmp.ne (M1, 16) {{.*}} [[SRC:.*]](1,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: cmp.ne (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK: or (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK: cmp.eq (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: or (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ueq bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFOrdNotEqual
define spir_func i1 @OpFOrdNotEqual(bfloat %data1, bfloat %data2) {
  ; CHECK-DAG: cmp.eq (M1, 16) {{.*}} [[SRC:.*]](1,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: cmp.eq (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK: and (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK: cmp.ne (M1, 16) {{.*}} [[SRC]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: and (M1, 16) {{.*}} {{P[0-9]+}} {{P[0-9]+}}
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp one bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordNotEqual
define spir_func i1 @OpFUnordNotEqual(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.ne (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp une bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFOrdLessThan
define spir_func i1 @OpFOrdLessThan(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.lt (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp olt bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordLessThan
define spir_func i1 @OpFUnordLessThan(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.ge (M1, 16) [[PRED:P[0-9]+]] [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: not (M1_NM, 16) {{.*}} [[PRED]]
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ult bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFOrdGreaterThan
define spir_func i1 @OpFOrdGreaterThan(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.gt (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-NOT: not
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ogt bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordGreaterThan
define spir_func i1 @OpFUnordGreaterThan(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.le (M1, 16) [[PRED:P[0-9]+]] [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: not (M1_NM, 16) {{.*}} [[PRED]]
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ugt bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFOrdLessThanEqual
define spir_func i1 @OpFOrdLessThanEqual(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.le (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-NOT: not
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ole bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordLessThanEqual
define spir_func i1 @OpFUnordLessThanEqual(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.gt (M1, 16) [[PRED:P[0-9]+]] [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: not (M1_NM, 16) {{.*}} [[PRED]]
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp ule bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFOrdGreaterThanEqual
define spir_func i1 @OpFOrdGreaterThanEqual(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.ge (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-NOT: not
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp oge bfloat %data1, %data2
  ret i1 %result
}

; CHECK-LABEL: .function "OpFUnordGreaterThanEqual
define spir_func i1 @OpFUnordGreaterThanEqual(bfloat %data1, bfloat %data2) {
  ; CHECK: cmp.lt (M1, 16) [[PRED:P[0-9]+]] [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK: not (M1_NM, 16) {{.*}} [[PRED]]
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = fcmp uge bfloat %data1, %data2
  ret i1 %result
}

declare spir_func i1 @_Z13__spirv_IsNanDF16b(bfloat)
declare spir_func i1 @_Z13__spirv_IsInfDF16b(bfloat)
declare spir_func i1 @_Z16__spirv_IsFiniteDF16b(bfloat)
declare spir_func i1 @_Z16__spirv_IsNormalDF16b(bfloat)
declare spir_func i1 @_Z18__spirv_SignBitSetDF16b(bfloat)
declare spir_func i1 @_Z21__spirv_LessOrGreaterDF16bDF16b(bfloat, bfloat)
