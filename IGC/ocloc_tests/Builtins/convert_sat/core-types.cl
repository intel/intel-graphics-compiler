/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device dg2 \
// RUN: -options "-I %S -cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck --enable-var-scope %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-LLVM22%} %else %{CHECK-PRE-LLVM22%}

// LLVM17 clamps NaN first (select before the min/max clamp); LLVM22 canonicalizes
// the same saturating conversion by sinking the NaN select to the end (min/max
// clamp first, then the select, then the int cast/range clamp). Both are
// equivalent; only the ordering / operand wiring of the clamp block differs, so
// those lines are version-split below.

#include "test_convert_sat_helper.h"

/*////////////////
/// Half tests
*/////////////////

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
// CHECK-LABEL: define spir_kernel void @test_convert_char_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
                                                                                /* -128 */
                                                                                /* 127 */
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xHD800)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call half @llvm.minnum.f16(half %[[CLAMP_MIN]], half 0xH57F0)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptosi half %[[CLAMP_MAX]] to i8
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[FP_SRC]], half 0xHD800)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call half @llvm.minnum.f16(half %[[CLAMP_MIN]], half 0xH57F0)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[CLAMP_MAX]], half 0xH0000
// CHECK-LLVM22: %[[CONV:.+]] = fptosi half %[[CLAMP_NAN]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(char, half)
// CHECK-LABEL: define spir_kernel void @test_convert_uchar_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
//
                                                                                /* 256 */
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xH0000)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call half @llvm.minnum.f16(half %[[CLAMP_MIN]], half 0xH5BF8)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_MAX]] to i8
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[FP_SRC]], half 0xH0000)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call half @llvm.minnum.f16(half %[[CLAMP_MIN]], half 0xH5BF8)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[CLAMP_MAX]], half 0xH0000
// CHECK-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_NAN]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uchar, half)
// CHECK-LABEL: define spir_kernel void @test_convert_short_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
//
                                                                                /* -32768 */
                                                               /* 32767 */
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xHF800)
// CHECK-PRE-LLVM22-DAG: %[[CONV:.+]] = fptosi half %[[CLAMP_MIN]] to i16
// CHECK-PRE-LLVM22-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge half %[[CLAMP_MIN]], 0xH7800
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = select i1 %[[INT_MAX_CMP]], i16 32767, i16 %[[CONV]]
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[FP_SRC]], half 0xHF800)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[CLAMP_MIN]], half 0xH0000
// CHECK-LLVM22-DAG: %[[CONV:.+]] = fptosi half %[[CLAMP_NAN]] to i16
// CHECK-LLVM22-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge half %[[CLAMP_NAN]], 0xH7800
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = select i1 %[[INT_MAX_CMP]], i16 32767, i16 %[[CONV]]
//
// CHECK: store i16 %[[CLAMP_MAX]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(short, half)
// CHECK-LABEL: define spir_kernel void @test_convert_ushort_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
//
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xH0000)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_MIN]] to i16
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[FP_SRC]], half 0xH0000)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[CLAMP_MIN]], half 0xH0000
// CHECK-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_NAN]] to i16
// CHECK: %[[POS_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xH7C00
// CHECK: %[[CLAMP_MAX:.+]] = select i1 %[[POS_INF_CMP]], i16 -1, i16 %[[CONV]]
//
// CHECK: store i16 %[[CLAMP_MAX]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ushort, half)
// CHECK-LABEL: define spir_kernel void @test_convert_int_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK-DAG: %[[CONV:.+]] = fptosi half %[[FP_SRC]] to i32
// CHECK-DAG: %[[NAN_CMP:.+]] = fcmp une half %[[FP_SRC]], %[[FP_SRC]]
// CHECK-DAG: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], i32 0, i32 %[[CONV]]
//
// CHECK-DAG: %[[NEG_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xHFC00
// CHECK-DAG: %[[CLAMP_MIN:.+]] = select i1 %[[NEG_INF_CMP]], i32 -2147483648, i32 %[[CLAMP_NAN]]
// CHECK-DAG: %[[POS_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xH7C00
// CHECK: %[[CLAMP_MAX:.+]] = select i1 %[[POS_INF_CMP]], i32 2147483647, i32 %[[CLAMP_MIN]]
//
// CHECK: store i32 %[[CLAMP_MAX]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(int, half)
// CHECK-LABEL: define spir_kernel void @test_convert_uint_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
//
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xH0000)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_MIN]] to i32
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[FP_SRC]], half 0xH0000)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[CLAMP_MIN]], half 0xH0000
// CHECK-LLVM22: %[[CONV:.+]] = fptoui half %[[CLAMP_NAN]] to i32
// CHECK: %[[POS_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xH7C00
// CHECK: %[[CLAMP_MAX:.+]] = select i1 %[[POS_INF_CMP]], i32 -1, i32 %[[CONV]]
//
// CHECK: store i32 %[[CLAMP_MAX]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uint, half)

/*////////////////
/// Float tests
*/////////////////

// CHECK-LABEL: define spir_kernel void @test_convert_char_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float -1.280000e+02)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 1.270000e+02)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_MAX]] to i8
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float -1.280000e+02)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 1.270000e+02)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_NAN]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(char, float)
// CHECK-LABEL: define spir_kernel void @test_convert_uchar_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0.000000e+00)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 2.550000e+02)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_MAX]] to i8
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float 0.000000e+00)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 2.550000e+02)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_NAN]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uchar, float)
// CHECK-LABEL: define spir_kernel void @test_convert_short_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float -3.276800e+04)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 3.276700e+04)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_MAX]] to i16
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float -3.276800e+04)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 3.276700e+04)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_NAN]] to i16
// CHECK: store i16 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(short, float)
// CHECK-LABEL: define spir_kernel void @test_convert_ushort_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0.000000e+00)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 6.553500e+04)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_MAX]] to i16
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float 0.000000e+00)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 6.553500e+04)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_NAN]] to i16
// CHECK: store i16 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ushort, float)
// CHECK-LABEL: define spir_kernel void @test_convert_int_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
                                                                                   /* -2.147483648e+09 */
                                                                                   /* 2.147483647e+09 */
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0xC1E0000000000000)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 0x41E0000000000000)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_MAX]] to i32
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float 0xC1E0000000000000)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 0x41E0000000000000)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptosi float %[[CLAMP_NAN]] to i32
// CHECK: store i32 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(int, float)
// CHECK-LABEL: define spir_kernel void @test_convert_uint_float
// CHECK-LABEL: define spir_kernel void @test_convert_int_float
// CHECK: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
                                                                                   /* 4.294967295e+09 */
// CHECK-PRE-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
// CHECK-PRE-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0.000000e+00)
// CHECK-PRE-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 0x41F0000000000000)
// CHECK-PRE-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_MAX]] to i32
// CHECK-LLVM22: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[FP_SRC]], float 0.000000e+00)
// CHECK-LLVM22: %[[CLAMP_MAX:.+]] = call float @llvm.minnum.f32(float %[[CLAMP_MIN]], float 0x41F0000000000000)
// CHECK-LLVM22: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[CLAMP_MAX]], float 0.000000e+00
// CHECK-LLVM22: %[[CONV:.+]] = fptoui float %[[CLAMP_NAN]] to i32
// CHECK: store i32 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uint, float)
