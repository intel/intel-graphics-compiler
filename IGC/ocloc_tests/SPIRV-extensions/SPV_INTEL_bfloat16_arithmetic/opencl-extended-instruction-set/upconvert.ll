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
; RUN: ocloc compile -spirv_input -file %t.spv -device cri -options "-igc_opts 'PrintAfter=BIImport,PrintToConsole=1'" > %t.out 2>&1

; Running FileCheck for each function tested below, because LLVM linker
; links builtins in different order than they are defined in this file.

; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ACOS %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ACOSH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ACOSPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ASIN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ASINH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ASINPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ATAN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ATAN2 %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ATANH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ATANPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ATAN2PI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-CBRT %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-COPYSIGN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-COSH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-COSPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ERFC %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ERF %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-EXP %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-EXP10 %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-EXPM1 %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LOG %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LOG10 %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FDIM %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FMAX %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FMIN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FMOD %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-HYPOT %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ILOGB %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LDEXP %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LGAMMA %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LOG1P %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LOGB %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-MAXMAG %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-MINMAG %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-POW %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-POWN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-POWR %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-REMAINDER %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-RINT %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ROOTN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-ROUND %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-SINH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-SINPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-TAN %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-TANPI %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-TGAMMA %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-NATIVE_POWR %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FCLAMP %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FMAX_COMMON %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-FMIN_COMMON %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-CROSS %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-DISTANCE %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-LENGTH %s
; RUN: FileCheck --input-file=%t.out --check-prefix=CHECK-NORMALIZE %s

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
; [TODO] 2.6. Vector Data Load/Store Instructions
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

; CHECK-ACOS-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_acosDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ACOS-NOT: }
; CHECK-ACOS-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_acos(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_acosDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ACOSH-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_acoshDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ACOSH-NOT: }
; CHECK-ACOSH-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_acosh(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_acoshDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ACOSPI-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_acospiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ACOSPI-NOT: }
; CHECK-ACOSPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_acospi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_acospiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ASIN-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_asinDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ASIN-NOT: }
; CHECK-ASIN-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_asin(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_asinDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ASINH-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_asinhDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ASINH-NOT: }
; CHECK-ASINH-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_asinh(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_asinhDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ASINPI-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_asinpiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ASINPI-NOT: }
; CHECK-ASINPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_asinpi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_asinpiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ATAN-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_atanDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ATAN-NOT: }
; CHECK-ATAN-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_atan(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_atanDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ATAN2-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_atan2DF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-ATAN2-NOT: }
; CHECK-ATAN2-DAG: fpext bfloat [[ARG0]] to float
; CHECK-ATAN2-NOT: }
; CHECK-ATAN2-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_atan2(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_atan2DF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-ATANH-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_atanhDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ATANH-NOT: }
; CHECK-ATANH-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_atanh(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_atanhDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ATANPI-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_atanpiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ATANPI-NOT: }
; CHECK-ATANPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_atanpi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_atanpiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ATAN2PI-DAG: define {{.*}}bfloat @_Z19__spirv_ocl_atan2piDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-ATAN2PI-NOT: }
; CHECK-ATAN2PI-DAG: fpext bfloat [[ARG0]] to float
; CHECK-ATAN2PI-NOT: }
; CHECK-ATAN2PI-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_atan2pi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z19__spirv_ocl_atan2piDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-CBRT-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_cbrtDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-CBRT-NOT: }
; CHECK-CBRT-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_cbrt(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_cbrtDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-COPYSIGN-DAG: define {{.*}}bfloat @_Z20__spirv_ocl_copysignDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-COPYSIGN-NOT: }
; CHECK-COPYSIGN-DAG: fpext bfloat [[ARG0]] to float
; CHECK-COPYSIGN-NOT: }
; CHECK-COPYSIGN-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_copysign(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z20__spirv_ocl_copysignDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-COSH-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_coshDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-COSH-NOT: }
; CHECK-COSH-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_cosh(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_coshDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-COSPI-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_cospiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-COSPI-NOT: }
; CHECK-COSPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_cospi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_cospiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ERFC-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_erfcDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ERFC-NOT: }
; CHECK-ERFC-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_erfc(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_erfcDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ERF-DAG: define {{.*}}bfloat @_Z15__spirv_ocl_erfDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ERF-NOT: }
; CHECK-ERF-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_erf(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z15__spirv_ocl_erfDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-EXP-DAG: define {{.*}}bfloat @_Z15__spirv_ocl_expDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-EXP-NOT: }
; CHECK-EXP-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_exp(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z15__spirv_ocl_expDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-EXP10-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_exp10DF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-EXP10-NOT: }
; CHECK-EXP10-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_exp10(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_exp10DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-EXPM1-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_expm1DF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-EXPM1-NOT: }
; CHECK-EXPM1-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_expm1(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_expm1DF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LOG-DAG: define {{.*}}bfloat @_Z15__spirv_ocl_logDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LOG-NOT: }
; CHECK-LOG-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_log(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z15__spirv_ocl_logDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LOG10-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_log10DF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LOG10-NOT: }
; CHECK-LOG10-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_log10(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_log10DF16b(bfloat %data1)
  ret bfloat %result
}

; TODO: Below check are commented out for now as clang generate bfloat instructions even though
; the builtin is implemented by upconversion:

; ; Function Attrs: alwaysinline mustprogress nofree norecurse nosync nounwind willreturn memory(none)
; define dso_local spir_func bfloat @_Z16__spirv_ocl_fdimDF16bDF16b(bfloat noundef %0, bfloat noundef %1) #2 !igc_bif !382 {
;   %3 = load i8, i8 addrspace(2)* @__bif_flag_FastRelaxedMath, align 1, !tbaa !383, !range !387, !noundef !388
;   %4 = icmp ne i8 %3, 0
;   %5 = fcmp ord bfloat %0, %1
;   %6 = or i1 %5, %4
;   %7 = fcmp ogt bfloat %0, %1
;   %8 = fsub bfloat %0, %1
;   %9 = select i1 %7, bfloat %8, bfloat 0xR0000
;   %10 = select i1 %6, bfloat %9, bfloat 0xR7FFF
;   ret bfloat %10
; }

; CHECK-FDIM-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_fdimDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; COM: CHECK-FDIM-NOT: }
; COM: CHECK-FDIM-DAG: fpext bfloat [[ARG0]] to float
; COM: CHECK-FDIM-NOT: }
; COM: CHECK-FDIM-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fdim(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_fdimDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-FMAX-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_fmaxDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-FMAX-NOT: }
; CHECK-FMAX-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FMAX-NOT: }
; CHECK-FMAX-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fmax(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_fmaxDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-FMIN-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_fminDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-FMIN-NOT: }
; CHECK-FMIN-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FMIN-NOT: }
; CHECK-FMIN-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fmin(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_fminDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-FMOD-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_fmodDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-FMOD-NOT: }
; CHECK-FMOD-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FMOD-NOT: }
; CHECK-FMOD-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fmod(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_fmodDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-HYPOT-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_hypotDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-HYPOT-NOT: }
; CHECK-HYPOT-DAG: fpext bfloat [[ARG0]] to float
; CHECK-HYPOT-NOT: }
; CHECK-HYPOT-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_hypot(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_hypotDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-ILOGB-DAG: define {{.*}}i32 @_Z17__spirv_ocl_ilogbDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ILOGB-NOT: }
; CHECK-ILOGB-DAG: fpext bfloat [[ARG]] to float
define spir_func i32 @test_spirv_ocl_ilogb(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func i32 @_Z17__spirv_ocl_ilogbDF16b(bfloat %data1)
  ret i32 %result
}

; CHECK-LDEXP-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_ldexpDF16bi(bfloat{{.*}}[[ARG:%.*]], i32 {{.*}}) {{.*}} {
; CHECK-LDEXP-NOT: }
; CHECK-LDEXP-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_ldexp(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_ldexpDF16bi(bfloat %data1, i32 2)
  ret bfloat %result
}

; CHECK-LGAMMA-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_lgammaDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LGAMMA-NOT: }
; CHECK-LGAMMA-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_lgamma(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_lgammaDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LOG1P-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_log1pDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LOG1P-NOT: }
; CHECK-LOG1P-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_log1p(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_log1pDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-LOGB-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_logbDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LOGB-NOT: }
; CHECK-LOGB-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_logb(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_logbDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-MAXMAG-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_maxmagDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-MAXMAG-NOT: }
; CHECK-MAXMAG-DAG: fpext bfloat [[ARG0]] to float
; CHECK-MAXMAG-NOT: }
; CHECK-MAXMAG-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_maxmag(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_maxmagDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-MINMAG-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_minmagDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-MINMAG-NOT: }
; CHECK-MINMAG-DAG: fpext bfloat [[ARG0]] to float
; CHECK-MINMAG-NOT: }
; CHECK-MINMAG-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_minmag(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_minmagDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-POW-DAG: define {{.*}}bfloat @_Z15__spirv_ocl_powDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-POW-NOT: }
; CHECK-POW-DAG: fpext bfloat [[ARG0]] to float
; CHECK-POW-NOT: }
; CHECK-POW-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_pow(bfloat %data1, bfloat %data2, bfloat %data3) {

  ; Intentionally no CHECKs here as POW is implemented via upconversion to float,
  ; which results in a complex sequence of instructions. The testing is intended to confirm that it
  ; compiles without crash.
  %result = call spir_func bfloat @_Z15__spirv_ocl_powDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-POWN-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_pownDF16bi(bfloat{{.*}}[[ARG:%.*]], i32 {{.*}}) {{.*}} {
; CHECK-POWN-NOT: }
; CHECK-POWN-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_pown(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_pownDF16bi(bfloat %data1, i32 2)
  ret bfloat %result
}

; CHECK-POWR-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_powrDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-POWR-NOT: }
; CHECK-POWR-DAG: fpext bfloat [[ARG0]] to float
; CHECK-POWR-NOT: }
; CHECK-POWR-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_powr(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_powrDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-REMAINDER-DAG: define {{.*}}bfloat @_Z21__spirv_ocl_remainderDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-REMAINDER-NOT: }
; CHECK-REMAINDER-DAG: fpext bfloat [[ARG0]] to float
; CHECK-REMAINDER-NOT: }
; CHECK-REMAINDER-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_remainder(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z21__spirv_ocl_remainderDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-RINT-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_rintDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-RINT-NOT: }
; CHECK-RINT-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_rint(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_rintDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-ROOTN-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_rootnDF16bi(bfloat{{.*}}[[ARG:%.*]], i32 {{.*}}) {{.*}} {
; CHECK-ROOTN-NOT: }
; CHECK-ROOTN-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_rootn(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_rootnDF16bi(bfloat %data1, i32 2)
  ret bfloat %result
}

; CHECK-ROUND-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_roundDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-ROUND-NOT: }
; CHECK-ROUND-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_round(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_roundDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-SINH-DAG: define {{.*}}bfloat @_Z16__spirv_ocl_sinhDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-SINH-NOT: }
; CHECK-SINH-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_sinh(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z16__spirv_ocl_sinhDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-SINPI-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_sinpiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-SINPI-NOT: }
; CHECK-SINPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_sinpi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_sinpiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-TAN-DAG: define {{.*}}bfloat @_Z15__spirv_ocl_tanDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-TAN-NOT: }
; CHECK-TAN-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_tan(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z15__spirv_ocl_tanDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-TANPI-DAG: define {{.*}}bfloat @_Z17__spirv_ocl_tanpiDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-TANPI-NOT: }
; CHECK-TANPI-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_tanpi(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z17__spirv_ocl_tanpiDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-TGAMMA-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_tgammaDF16b(bfloat{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-TGAMMA-NOT: }
; CHECK-TGAMMA-DAG: fpext bfloat [[ARG]] to float
define spir_func bfloat @test_spirv_ocl_tgamma(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_tgammaDF16b(bfloat %data1)
  ret bfloat %result
}

; CHECK-NATIVE_POWR-DAG: define {{.*}}bfloat @_Z23__spirv_ocl_native_powrDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-NATIVE_POWR-NOT: }
; CHECK-NATIVE_POWR-DAG: fpext bfloat [[ARG0]] to float
; CHECK-NATIVE_POWR-NOT: }
; CHECK-NATIVE_POWR-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_native_powr(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z23__spirv_ocl_native_powrDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-FCLAMP-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_fclampDF16bDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]], bfloat{{.*}}[[ARG2:%.*]]) {{.*}} {
; CHECK-FCLAMP-NOT: }
; CHECK-FCLAMP-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FCLAMP-NOT: }
; CHECK-FCLAMP-DAG: fpext bfloat [[ARG1]] to float
; CHECK-FCLAMP-NOT: }
; CHECK-FCLAMP-DAG: fpext bfloat [[ARG2]] to float
define spir_func bfloat @test_spirv_ocl_fclamp(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_fclampDF16bDF16bDF16b(bfloat %data1, bfloat %data2, bfloat %data3)
  ret bfloat %result
}

; CHECK-FMAX_COMMON-DAG: define {{.*}}bfloat @_Z23__spirv_ocl_fmax_commonDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-FMAX_COMMON-NOT: }
; CHECK-FMAX_COMMON-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FMAX_COMMON-NOT: }
; CHECK-FMAX_COMMON-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fmax_common(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z23__spirv_ocl_fmax_commonDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-FMIN_COMMON-DAG: define {{.*}}bfloat @_Z23__spirv_ocl_fmin_commonDF16bDF16b(bfloat{{.*}}[[ARG0:%.*]], bfloat{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-FMIN_COMMON-NOT: }
; CHECK-FMIN_COMMON-DAG: fpext bfloat [[ARG0]] to float
; CHECK-FMIN_COMMON-NOT: }
; CHECK-FMIN_COMMON-DAG: fpext bfloat [[ARG1]] to float
define spir_func bfloat @test_spirv_ocl_fmin_common(bfloat %data1, bfloat %data2, bfloat %data3) {
  %result = call spir_func bfloat @_Z23__spirv_ocl_fmin_commonDF16bDF16b(bfloat %data1, bfloat %data2)
  ret bfloat %result
}

; CHECK-CROSS-DAG: define {{.*}}<3 x bfloat> @_Z17__spirv_ocl_crossDv3_DF16bS_(<3 x bfloat>{{.*}}[[ARG0:%.*]], <3 x bfloat>{{.*}}[[ARG1:%.*]]) {{.*}} {
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG0_E0_GEP:%.*]] = extractelement <3 x bfloat> [[ARG0]], i64 0
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG0_E0_GEP]] to float
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG0_E1_GEP:%.*]] = extractelement <3 x bfloat> [[ARG0]], i64 1
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG0_E1_GEP]] to float
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG0_E2_GEP:%.*]] = extractelement <3 x bfloat> [[ARG0]], i64 2
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG0_E2_GEP]] to float
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG1_E0_GEP:%.*]] = extractelement <3 x bfloat> [[ARG1]], i64 0
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG1_E0_GEP]] to float
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG1_E1_GEP:%.*]] = extractelement <3 x bfloat> [[ARG1]], i64 1
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG1_E1_GEP]] to float
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: [[ARG1_E2_GEP:%.*]] = extractelement <3 x bfloat> [[ARG1]], i64 2
; CHECK-CROSS-NOT: }
; CHECK-CROSS-DAG: fpext bfloat [[ARG1_E2_GEP]] to float
define spir_func <3 x bfloat> @test_spirv_ocl_cross(bfloat %data1, bfloat %data2, bfloat %data3) {
  %vec3_1 = insertelement <3 x bfloat> undef, bfloat %data1, i32 0
  %vec3_2 = insertelement <3 x bfloat> %vec3_1, bfloat %data2, i32 1
  %vec3_3 = insertelement <3 x bfloat> %vec3_2, bfloat %data3, i32 2
  %result = call spir_func <3 x bfloat> @_Z17__spirv_ocl_crossDv3_DF16bS_(<3 x bfloat> %vec3_3, <3 x bfloat> %vec3_3)
  ret <3 x bfloat> %result
}

; TODO: __spirv_ocl_distance is implemented via __spirv_ocl_length which is so far implemented using upconversion.
;       Once __spirv_Dot for bfloat is implemented, we could implement __spirv_ocl_length directly using bfloat instructions.
; CHECK-DISTANCE-DAG: define {{.*}}bfloat @_Z20__spirv_ocl_distanceDv2_DF16bS_(<2 x bfloat>{{.*}}[[ARG0:%.*]], <2 x bfloat>{{.*}}[[ARG1:%.*]]) {{.*}} {
define spir_func bfloat @test_spirv_ocl_distance(<2 x bfloat> %data1, <2 x bfloat> %data2) {
  %result = call spir_func bfloat @_Z20__spirv_ocl_distanceDv2_DF16bS_(<2 x bfloat> %data1, <2 x bfloat> %data2)
  ret bfloat %result
}

; TODO: Once __spirv_Dot for bfloat is implemented, we could implement __spirv_ocl_length directly using bfloat instructions.
; CHECK-LENGTH-DAG: define {{.*}}bfloat @_Z18__spirv_ocl_lengthDv2_DF16b(<2 x bfloat>{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-LENGTH-NOT: }
; CHECK-LENGTH-DAG: [[ARG_E0_GEP:%.*]] = extractelement <2 x bfloat> [[ARG]], i64 0
; CHECK-LENGTH-NOT: }
; CHECK-LENGTH-DAG: fpext bfloat [[ARG_E0_GEP]] to float
; CHECK-LENGTH-NOT: }
; CHECK-LENGTH-DAG: [[ARG_E1_GEP:%.*]] = extractelement <2 x bfloat> [[ARG]], i64 1
; CHECK-LENGTH-NOT: }
; CHECK-LENGTH-DAG: fpext bfloat [[ARG_E1_GEP]] to float
define spir_func bfloat @test_spirv_ocl_length(<2 x bfloat> %data1) {
  %result = call spir_func bfloat @_Z18__spirv_ocl_lengthDv2_DF16b(<2 x bfloat> %data1)
  ret bfloat %result
}

; TODO: Once __spirv_Dot for bfloat is implemented, we could implement __spirv_ocl_normalize directly using bfloat instructions.
; CHECK-NORMALIZE-DAG: define {{.*}}<2 x bfloat> @_Z21__spirv_ocl_normalizeDv2_DF16b(<2 x bfloat>{{.*}}[[ARG:%.*]]) {{.*}} {
; CHECK-NORMALIZE-NOT: }
; CHECK-NORMALIZE-DAG: [[ARG_E0_GEP:%.*]] = extractelement <2 x bfloat> [[ARG]], i64 0
; CHECK-NORMALIZE-NOT: }
; CHECK-NORMALIZE-DAG: fpext bfloat [[ARG_E0_GEP]] to float
; CHECK-NORMALIZE-NOT: }
; CHECK-NORMALIZE-DAG: [[ARG_E1_GEP:%.*]] = extractelement <2 x bfloat> [[ARG]], i64 1
; CHECK-NORMALIZE-NOT: }
; CHECK-NORMALIZE-DAG: fpext bfloat [[ARG_E1_GEP]] to float
define spir_func <2 x bfloat> @test_spirv_ocl_normalize(<2 x bfloat> %data1) {
  %result = call spir_func <2 x bfloat> @_Z21__spirv_ocl_normalizeDv2_DF16b(<2 x bfloat> %data1)
  ret <2 x bfloat> %result
}

; 2.1. Math extended instructions:
declare spir_func bfloat @_Z16__spirv_ocl_acosDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_acoshDF16b(bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_acospiDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_asinDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_asinhDF16b(bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_asinpiDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_atanDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_atan2DF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_atanhDF16b(bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_atanpiDF16b(bfloat)
declare spir_func bfloat @_Z19__spirv_ocl_atan2piDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_cbrtDF16b(bfloat)
declare spir_func bfloat @_Z20__spirv_ocl_copysignDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_coshDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_cospiDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_erfcDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_erfDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_expDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_exp10DF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_expm1DF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_logDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_log10DF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_fdimDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_fmaxDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_fminDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_fmodDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_hypotDF16bDF16b(bfloat, bfloat)
declare spir_func i32 @_Z17__spirv_ocl_ilogbDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_ldexpDF16bi(bfloat, i32)
declare spir_func bfloat @_Z18__spirv_ocl_lgammaDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_log1pDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_logbDF16b(bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_maxmagDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_minmagDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_powDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_pownDF16bi(bfloat, i32)
declare spir_func bfloat @_Z16__spirv_ocl_powrDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z21__spirv_ocl_remainderDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_rintDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_rootnDF16bi(bfloat, i32)
declare spir_func bfloat @_Z17__spirv_ocl_roundDF16b(bfloat)
declare spir_func bfloat @_Z16__spirv_ocl_sinhDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_sinpiDF16b(bfloat)
declare spir_func bfloat @_Z15__spirv_ocl_tanDF16b(bfloat)
declare spir_func bfloat @_Z17__spirv_ocl_tanpiDF16b(bfloat)
declare spir_func bfloat @_Z18__spirv_ocl_tgammaDF16b(bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_native_powrDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z22__spirv_ocl_native_tanDF16b(bfloat)

; 2.3. Common instructions:
declare spir_func bfloat @_Z18__spirv_ocl_fclampDF16bDF16bDF16b(bfloat, bfloat, bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_fmax_commonDF16bDF16b(bfloat, bfloat)
declare spir_func bfloat @_Z23__spirv_ocl_fmin_commonDF16bDF16b(bfloat, bfloat)

; 2.4. Geometric instructions:
declare spir_func <3 x bfloat> @_Z17__spirv_ocl_crossDv3_DF16bS_(<3 x bfloat>, <3 x bfloat>)
declare spir_func bfloat @_Z20__spirv_ocl_distanceDv2_DF16bS_(<2 x bfloat>, <2 x bfloat>)
declare spir_func bfloat @_Z18__spirv_ocl_lengthDv2_DF16b(<2 x bfloat>)
declare spir_func <2 x bfloat> @_Z21__spirv_ocl_normalizeDv2_DF16b(<2 x bfloat>)
