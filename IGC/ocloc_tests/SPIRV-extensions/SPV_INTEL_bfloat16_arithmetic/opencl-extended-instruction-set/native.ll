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
; COM: Execute ocloc second time, this time without DumpVISAASMToConsole flag, to ensure that E2E compilation does not crash.
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'ForceOCLSIMDWidth=16' -cl-intel-library-compilation"

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; The following is quoted from the SPV_INTEL_bfloat16_arithmetic extension specification:

; "Modifications to the SPIR-V Specification, Version 1.6
; Validation Rules
; Add validation rules to section 2.16.1 Universal Validation Rules from:
;
; - Variables with a type that is or includes a floating-point type with the BFloat16KHR encoding can be
;   used with OpExtInst instruction with the opcodes from the OpenCL extended instruction set."


; Below is a list OpenCL.std functions that operate on floating-point types.
; The SPV_INTEL_bfloat16_arithmetic extension requires these functions to
; accept and correctly process bfloat16 inputs/outputs wherever they are
; defined for floating-point.
;
; Implementation classification used in tests:
; - NATIVE: Implemented via a single bfloat16 HW instruction or a sequence
;   of bfloat16 HW instructions without upconverting to float.
; - UPCONVERT: Implemented by upconverting bfloat16 inputs to float,
;   executing the float variant, and (where applicable) converting results
;   back to bfloat16.

; ==============================================================================
; ===================== BEGIN: Floating-Point OpenCL.std Functions =============
; ==============================================================================

; --------------------------------------------------------------------------
; 2.1. Math Extended Instructions
; --------------------------------------------------------------------------
;   NATIVE:
;     ceil, cos, exp2, fabs, floor, log2, mad, rint, round,
;     rsqrt, sin, sqrt, tan, tanh, trunc,
;     native_cos, native_divide, native_exp, native_exp2, native_exp10,
;     native_log, native_log2, native_log10, native_recip, native_rsqrt,
;     native_sin, native_sqrt, native_tan
;
;   UPCONVERT:
;     acos, acosh, acospi, asin, asinh, asinpi, atan, atan2, atanh, atanpi,
;     atan2pi, cbrt, copysign, cosh, cospi, erfc, erf, exp, exp10, expm1,
;     fdim, fma, fmax, fmin, fmod, fract, frexp, hypot, ilogb, ldexp,
;     lgamma, lgamma_r, log, log10, log1p, logb, maxmag, minmag, nan,
;     nextafter, pow, pown, powr, remainder, remquo, rootn, sincos, sinh,
;     sinpi, tanpi, tgamma, native_powr

; --------------------------------------------------------------------------
; 2.3. Common Instructions
; --------------------------------------------------------------------------
;   NATIVE:
;     mix, radians, step, sign
;
;   UPCONVERT:
;     fclamp, degrees, fmax_common, fmin_common, smoothstep

; --------------------------------------------------------------------------
; 2.4. Geometric Instructions
; --------------------------------------------------------------------------
;   UPCONVERT:
;     cross, distance, length, normalize
;
;   Note: The following are only available for float type (not gentypef) in OpenCL C:
;     fast_distance, fast_length, fast_normalize

; --------------------------------------------------------------------------
; 2.5. Relational Instructions
; --------------------------------------------------------------------------
;   NATIVE:
;     select, bitselect

; --------------------------------------------------------------------------
; 2.6. Vector Data Load/Store Instructions
; --------------------------------------------------------------------------
;     vloadn, vstoren

; --------------------------------------------------------------------------
; [TODO] 2.7. Miscellaneous Vector Instructions
; --------------------------------------------------------------------------
;     shuffle, shuffle2

; --------------------------------------------------------------------------
; [TODO] 2.8. Misc Instructions
; --------------------------------------------------------------------------
;     printf, prefetch

; ==============================================================================
; ====================== END: Floating-Point OpenCL.std Functions ==============
; ==============================================================================

; CHECK-LABEL: .function "test_spirv_ocl_ceil
define spir_func bfloat @test_spirv_ocl_ceil(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: rndu (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: mov (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=f
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_ceilDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_cos
define spir_func bfloat @test_spirv_ocl_cos(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: cos (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z15__spirv_ocl_cosDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_exp2
define spir_func bfloat @test_spirv_ocl_exp2(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK-NOT: mul (M1, 16)
  ; CHECK: exp (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_exp2DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_fabs
define spir_func bfloat @test_spirv_ocl_fabs(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: and (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x7fff:w
  %result = call spir_func bfloat @_Z16__spirv_ocl_fabsDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_floor
define spir_func bfloat @test_spirv_ocl_floor(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: rndd (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: mov (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=f
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z17__spirv_ocl_floorDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_log2
define spir_func bfloat @test_spirv_ocl_log2(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: log (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_log2DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_mad
define spir_func bfloat @test_spirv_ocl_mad(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mad (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](1,0)<1;1,0> [[SRC]](0,0)<1;1,0> [[SRC]](2,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func bfloat @_Z15__spirv_ocl_madDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_rsqrt
define spir_func bfloat @test_spirv_ocl_rsqrt(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; TODO: rsqrt is currently handled by SQRT + INV. MatchRsqrt() in PatternMatchPass.cpp needs to be updated to produce bfloat RSQRT.
  ; CHECK: sqrt (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: inv (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z17__spirv_ocl_rsqrtDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_sin
define spir_func bfloat @test_spirv_ocl_sin(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: sin (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z15__spirv_ocl_sinDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_sqrt
define spir_func bfloat @test_spirv_ocl_sqrt(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: sqrt (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_sqrtDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_tanh
define spir_func bfloat @test_spirv_ocl_tanh(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: tanh (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_tanhDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_trunc
define spir_func bfloat @test_spirv_ocl_trunc(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: rndz (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: mov (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=f
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z17__spirv_ocl_truncDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_cos
define spir_func bfloat @test_spirv_ocl_native_cos(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: cos (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z22__spirv_ocl_native_cosDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_divide
define spir_func bfloat @test_spirv_ocl_native_divide(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: inv (M1, 16) [[DST0:.*]](0,0)<1> [[SRC0:.*]]({{.*}})<1;1,0>
  ; CHECK: mul (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]]({{.*}})<1;1,0> [[SRC1:.*]]({{.*}})<1;1,0>
  ; CHECK-DAG: .decl [[SRC0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[SRC1]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z25__spirv_ocl_native_divideDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_exp
define spir_func bfloat @test_spirv_ocl_native_exp(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mul (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x3fb90000:f
  ; CHECK: exp (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z22__spirv_ocl_native_expDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_exp10
define spir_func bfloat @test_spirv_ocl_native_exp10(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mul (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x40550000:f
  ; CHECK: exp (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z24__spirv_ocl_native_exp10DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_exp2
define spir_func bfloat @test_spirv_ocl_native_exp2(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK-NOT: mul (M1, 16)
  ; CHECK: exp (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z23__spirv_ocl_native_exp2DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_log
define spir_func bfloat @test_spirv_ocl_native_log(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: log (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: mul (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0> 0x3f310000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z22__spirv_ocl_native_logDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_log10
define spir_func bfloat @test_spirv_ocl_native_log10(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: log (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: mul (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0> 0x3e9a0000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z24__spirv_ocl_native_log10DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_log2
define spir_func bfloat @test_spirv_ocl_native_log2(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: log (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z23__spirv_ocl_native_log2DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_recip
define spir_func bfloat @test_spirv_ocl_native_recip(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: inv (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z24__spirv_ocl_native_recipDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_rsqrt
define spir_func bfloat @test_spirv_ocl_native_rsqrt(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; TODO: rsqrt is currently handled by SQRT + INV. MatchRsqrt() in PatternMatchPass.cpp needs to be updated to produce bfloat RSQRT.
  ; CHECK: sqrt (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK: inv (M1, 16) [[DST1:.*]](0,0)<1> [[DST0]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z24__spirv_ocl_native_rsqrtDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_sin
define spir_func bfloat @test_spirv_ocl_native_sin(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: sin (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z22__spirv_ocl_native_sinDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_sqrt
define spir_func bfloat @test_spirv_ocl_native_sqrt(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: sqrt (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z23__spirv_ocl_native_sqrtDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_native_tan
define spir_func bfloat @test_spirv_ocl_native_tan(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; Tangens is implemented by float sin/cos sequence.
  ; CHECK: sin (M1, 16) {{.*}}(0,0)<1> {{.*}}(0,0)<1;1,0>
  ; CHECK: cos (M1, 16) {{.*}}(0,0)<1> {{.*}}(0,0)<1;1,0>
  ; CHECK: inv (M1, 16) {{.*}}(0,0)<1> {{.*}}(0,0)<1;1,0>
  ; CHECK: mul (M1, 16) {{.*}}(0,0)<1> {{.*}}(0,0)<1;1,0> {{.*}}(0,0)<1;1,0>
  ; CHECK: mov (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=f
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z22__spirv_ocl_native_tanDF16b(bfloat %data1)
  ret bfloat %result
}

; Not implemented until SPIRV-LLVM Translator support generation of __spirv_ocl_nan_Rbfloat
;define spir_func bfloat @test_spirv_ocl_nan(i16 %data1) {
;  %result = call spir_func bfloat @_Z15__spirv_ocl_nans(i16 %data1)
;  ret bfloat %result
;}

; CHECK-LABEL: .function "test_spirv_ocl_nextafter
define spir_func bfloat @test_spirv_ocl_nextafter(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; This instruction is not upconverted to float. It is handled via a sequence of short instructions.
  ; Not adding CHECKs here. Just confirming that it compiles without a crash.
  %result = call spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_fma
define spir_func bfloat @test_spirv_ocl_fma(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mad (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](1,0)<1;1,0> [[SRC]](0,0)<1;1,0> [[SRC]](2,0)<1;1,0>
  ; CHECK: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func bfloat @_Z15__spirv_ocl_fmaDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_mix
define spir_func bfloat @test_spirv_ocl_mix(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: add (M1, 16) [[DST0:.*]](0,0)<1> [[SRC:.*]](1,0)<1;1,0> (-)[[SRC]](0,0)<1;1,0>
  ; CHECK: mad (M1, 16) [[DST1:.*]](0,0)<1> [[SRC]](2,0)<1;1,0> [[DST0]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST0]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST1]] v_type=G type=bf
  %result = call spir_func bfloat @_Z15__spirv_ocl_mixDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_radians
define spir_func bfloat @test_spirv_ocl_radians(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mul (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x3c8f0000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z19__spirv_ocl_radiansDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_step
define spir_func bfloat @test_spirv_ocl_step(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: cmp.lt (M1, 16) [[PRED:P[0-9]+]] [[SRC:.*]](1,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK: ([[PRED]]) sel (M1, 16) [[DST:.*]](0,0)<1> 0x0:f 0x3f800000:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_stepDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

define spir_func bfloat @test_spirv_ocl_smoothstep(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; This instruction is NOT fully upconverted to float. It is handled via float clamp and bfloat mad and muls.
  ; Not adding CHECKs here. Just confirming that it compiles without a crash.
  %result = call spir_func bfloat @_Z22__spirv_ocl_smoothstepDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_sign
define spir_func bfloat @test_spirv_ocl_sign(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK-DAG: cmp.gt (M1, 16) [[PRED0:P[0-9]+]] [[SRC:.*]](0,0)<1;1,0> 0x0:f
  ; CHECK-DAG: ([[PRED0]]) sel (M1, 16) [[DST:.*]](0,0)<1> 0x3f800000:f [[SRC]](0,0)<1;1,0>
  ; CHECK-DAG: cmp.lt (M1, 16) [[PRED1:P[0-9]+]] [[SRC]](0,0)<1;1,0> 0x0:f
  ; CHECK-DAG: ([[PRED1]]) sel (M1, 16) [[DST]](0,0)<1> 0xbf800000:f [[DST]](0,0)<1;1,0>
  ; CHECK-DAG: cmp.eq (M1, 16) [[PRED2:P[0-9]+]] [[SRC]](0,0)<1;1,0> [[SRC]](0,0)<1;1,0>
  ; CHECK-DAG: ([[PRED2]]) sel (M1, 16) [[DST]](0,0)<1> [[DST]](0,0)<1;1,0> 0x0:f
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z16__spirv_ocl_signDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_degrees
define spir_func bfloat @test_spirv_ocl_degrees(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; CHECK: mul (M1, 16) {{.*}}(0,0)<1> [[SRC:.*]](0,0)<1;1,0> 0x42650000:f
  ; CHECK: .decl [[SRC]] v_type=G type=bf
  %result = call spir_func bfloat @_Z19__spirv_ocl_degreesDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_select
define spir_func bfloat @test_spirv_ocl_select(bfloat %data1, bfloat %data2, bfloat %data3, i16 %cond) {
  ; CHECK: ({{P[0-9]+}}) sel (M1, 16) [[DST:.*]](0,0)<1> [[SRC:.*]](0,0)<1;1,0> [[SRC]](1,0)<1;1,0>
  ; CHECK-DAG: .decl [[SRC]] v_type=G type=bf
  ; CHECK-DAG: .decl [[DST]] v_type=G type=bf
  %result = call spir_func bfloat @_Z18__spirv_ocl_selectDF16bDF16bs(bfloat %data1, bfloat %data2, i16 %cond)
  ret bfloat %result
}

; CHECK-LABEL: .function "test_spirv_ocl_bitselect
define spir_func bfloat @test_spirv_ocl_bitselect(bfloat %data1, bfloat %data2, bfloat %data3) {
  ; bitselect is implemented using bitcast to integer type
  ; CHECK: bfn.xd8 (M1, 16) [[DST:.*]](0,0)<1> [[SRC0:.*]]({{.*}})<1;1,0> [[SRC1:.*]]({{.*}})<1;1,0> [[SRC2:.*]]({{.*}})<1;1,0>
  ; CHECK-DAG: .decl [[SRC0]] v_type=G type=w
  ; CHECK-DAG: .decl [[DST]] v_type=G type=w
  %result = call spir_func bfloat @_Z21__spirv_ocl_bitselectDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; Tests for vloadn/vstoren builtins for bfloat are commented out until SPIRV-LLVM Translator supports them properly.

; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vload2
; COM: define spir_func <2 x bfloat> @test_spirv_ocl_vload2(i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   %result = call spir_func <2 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat2lPU3AS1DF16bi(i64 %offset, bfloat addrspace(1)* %ptr, i32 2)
; COM:   ret <2 x bfloat> %result
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vload3
; COM: define spir_func <3 x bfloat> @test_spirv_ocl_vload3(i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   %result = call spir_func <3 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat3lPU3AS1DF16bi(i64 %offset, bfloat addrspace(1)* %ptr, i32 3)
; COM:   ret <3 x bfloat> %result
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vload4
; COM: define spir_func <4 x bfloat> @test_spirv_ocl_vload4(i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   %result = call spir_func <4 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat4lPU3AS1DF16bi(i64 %offset, bfloat addrspace(1)* %ptr, i32 4)
; COM:   ret <4 x bfloat> %result
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vload8
; COM: define spir_func <8 x bfloat> @test_spirv_ocl_vload8(i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   %result = call spir_func <8 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat8lPU3AS1DF16bi(i64 %offset, bfloat addrspace(1)* %ptr, i32 8)
; COM:   ret <8 x bfloat> %result
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vload16
; COM: define spir_func <16 x bfloat> @test_spirv_ocl_vload16(i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   %result = call spir_func <16 x bfloat> @_Z28__spirv_ocl_vloadn_Rbfloat16lPU3AS1DF16bi(i64 %offset, bfloat addrspace(1)* %ptr, i32 16)
; COM:   ret <16 x bfloat> %result
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vstore2
; COM: define spir_func void @test_spirv_ocl_vstore2(<2 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   call spir_func void @_Z19__spirv_ocl_vstorenDv2_DF16blPU3AS1DF16b(<2 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr)
; COM:   ret void
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vstore3
; COM: define spir_func void @test_spirv_ocl_vstore3(<3 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   call spir_func void @_Z19__spirv_ocl_vstorenDv3_DF16blPU3AS1DF16b(<3 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr)
; COM:   ret void
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vstore4
; COM: define spir_func void @test_spirv_ocl_vstore4(<4 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   call spir_func void @_Z19__spirv_ocl_vstorenDv4_DF16blPU3AS1DF16b(<4 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr)
; COM:   ret void
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vstore8
; COM: define spir_func void @test_spirv_ocl_vstore8(<8 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   call spir_func void @_Z19__spirv_ocl_vstorenDv8_DF16blPU3AS1DF16b(<8 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr)
; COM:   ret void
; COM: }
; COM:
; COM: ; CHECK-LABEL: .function "test_spirv_ocl_vstore16
; COM: define spir_func void @test_spirv_ocl_vstore16(<16 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr) {
; COM:   call spir_func void @_Z19__spirv_ocl_vstorenDv16_DF16blPU3AS1DF16b(<16 x bfloat> %data, i64 %offset, bfloat addrspace(1)* %ptr)
; COM:   ret void
; COM: }

; 2.1. Math extended instructions:
declare spir_func bfloat @_Z16__spirv_ocl_ceilDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_cosDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_exp2DF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_fabsDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_floorDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_log2DF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_madDF16bDF16bDF16b(bfloat, bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_rintDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_roundDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_rsqrtDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_sinDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_sinhDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_sinpiDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_sqrtDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_tanDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_tanhDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_truncDF16b(bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_cosDF16b(bfloat)
declare spir_func bfloat @_Z25__spirv_ocl_native_divideDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_expDF16b(bfloat)
declare spir_func bfloat @_Z24__spirv_ocl_native_exp10DF16b(bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_native_exp2DF16b(bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_logDF16b(bfloat)
declare spir_func bfloat @_Z24__spirv_ocl_native_log10DF16b(bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_native_log2DF16b(bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_native_powrDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z24__spirv_ocl_native_recipDF16b(bfloat)
declare spir_func bfloat @_Z24__spirv_ocl_native_rsqrtDF16b(bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_sinDF16b(bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_native_sqrtDF16b(bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_tanDF16b(bfloat)
;declare spir_func bfloat @_Z15__spirv_ocl_nans(i16)
declare spir_func bfloat @_Z21__spirv_ocl_nextafterDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_fmaDF16bDF16bDF16b(bfloat, bfloat, bfloat)

; 2.3. Common instructions:
declare spir_func bfloat @_Z19__spirv_ocl_degreesDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_mixDF16bDF16bDF16b(bfloat, bfloat, bfloat)
declare spir_func bfloat @_Z19__spirv_ocl_radiansDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_stepDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_smoothstepDF16bDF16bDF16b(bfloat, bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_signDF16b(bfloat)

; 2.5. Relational instructions:
declare spir_func bfloat @_Z18__spirv_ocl_selectDF16bDF16bs(bfloat, bfloat, i16)
declare spir_func bfloat @_Z21__spirv_ocl_bitselectDF16bDF16bDF16b(bfloat, bfloat, bfloat)

; 2.6. Vector Data Load/Store Instructions:
declare spir_func <2 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat2lPU3AS1DF16bi(i64, bfloat addrspace(1)*, i32)
declare spir_func <3 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat3lPU3AS1DF16bi(i64, bfloat addrspace(1)*, i32)
declare spir_func <4 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat4lPU3AS1DF16bi(i64, bfloat addrspace(1)*, i32)
declare spir_func <8 x bfloat> @_Z27__spirv_ocl_vloadn_Rbfloat8lPU3AS1DF16bi(i64, bfloat addrspace(1)*, i32)
declare spir_func <16 x bfloat> @_Z28__spirv_ocl_vloadn_Rbfloat16lPU3AS1DF16bi(i64, bfloat addrspace(1)*, i32)

declare spir_func void @_Z19__spirv_ocl_vstorenDv2_DF16blPU3AS1DF16b(<2 x bfloat>, i64, bfloat addrspace(1)*)
declare spir_func void @_Z19__spirv_ocl_vstorenDv3_DF16blPU3AS1DF16b(<3 x bfloat>, i64, bfloat addrspace(1)*)
declare spir_func void @_Z19__spirv_ocl_vstorenDv4_DF16blPU3AS1DF16b(<4 x bfloat>, i64, bfloat addrspace(1)*)
declare spir_func void @_Z19__spirv_ocl_vstorenDv8_DF16blPU3AS1DF16b(<8 x bfloat>, i64, bfloat addrspace(1)*)
declare spir_func void @_Z19__spirv_ocl_vstorenDv16_DF16blPU3AS1DF16b(<16 x bfloat>, i64, bfloat addrspace(1)*)
