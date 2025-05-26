/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported, dg2-supported, llvm-16-plus

// Partial i64 emulation
// RUN: ocloc compile -file %s -device pvc \
// RUN: -options "-I %S -cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck --enable-var-scope %s --check-prefixes=CHECK,CHECK-PARTIAL-EMU

// In full i64 emu, fp32/64 ftoi casts involve complex emulation sequences
// that we shouldn't really be testing here. Disable those tests via additional FE macro
// RUN: ocloc compile -file %s -device dg2 \
// RUN: -options "-I %S -cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass' -DCHECK_HALF_ONLY" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck --enable-var-scope %s --check-prefixes=CHECK,CHECK-FULL-EMU

#include "test_convert_sat_helper.h"

/*////////////////
/// Half tests
*/////////////////

#pragma OPENCL EXTENSION cl_khr_fp16 : enable
// CHECK-LABEL: define spir_kernel void @test_convert_long_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptosi half %[[FP_SRC]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-FULL-EMU-DAG: %[[CONV_LO:.+]] = fptosi half %[[FP_SRC]] to i32
// CHECK-FULL-EMU-DAG: %[[CONV_HI:.+]] = ashr i32 %[[CONV_LO]], 31
//
// CHECK-DAG: %[[NAN_CMP:.+]] = fcmp une half %[[FP_SRC]], %[[FP_SRC]]
// CHECK-DAG: %[[CLAMP_NAN_LO:.+]] = select i1 %[[NAN_CMP]], i32 0, i32 %[[CONV_LO]]
// CHECK-DAG: %[[CLAMP_NAN_HI:.+]] = select i1 %[[NAN_CMP]], i32 0, i32 %[[CONV_HI]]
// CHECK-DAG: %[[NEG_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xHFC00
// CHECK-DAG: %[[CLAMP_MIN_LO:.+]] = select i1 %[[NEG_INF_CMP]], i32 0, i32 %[[CLAMP_NAN_LO]]
// CHECK-DAG: %[[CLAMP_MIN_HI:.+]] = select i1 %[[NEG_INF_CMP]], i32 -2147483648, i32 %[[CLAMP_NAN_HI]]
// CHECK-DAG: %[[POS_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xH7C00
// CHECK-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[POS_INF_CMP]], i32 -1, i32 %[[CLAMP_MIN_LO]]
// CHECK-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[POS_INF_CMP]], i32 2147483647, i32 %[[CLAMP_MIN_HI]]
//
// CHECK: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(long, half)
// CHECK-LABEL: define spir_kernel void @test_convert_ulong_half
// CHECK: %[[FP_SRC:.+]] = load half, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq half %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], half %[[FP_SRC]], half 0xH0000
// CHECK: %[[CLAMP_MIN:.+]] = call half @llvm.maxnum.f16(half %[[CLAMP_NAN]], half 0xH0000)
//
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptoui half %[[CLAMP_MIN]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-FULL-EMU-DAG: %[[CONV_LO:.+]] = fptoui half %[[CLAMP_MIN]] to i32
//
// CHECK: %[[POS_INF_CMP:.+]] = fcmp oeq half %[[FP_SRC]], 0xH7C00
// CHECK-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[POS_INF_CMP]], i32 -1, i32 %[[CONV_LO]]
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[POS_INF_CMP]], i32 -1, i32 %[[CONV_HI]]
// COM: In full emu, MSBs will be simply zero-filled for normal (non-INF) case
// CHECK-FULL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[POS_INF_CMP]], i32 -1, i32 0
//
// CHECK: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ulong, half)

/*////////////////
/// Float tests
*/////////////////

#ifndef CHECK_HALF_ONLY
// CHECK-PARTIAL-EMU-LABEL: define spir_kernel void @test_convert_long_float
// CHECK-PARTIAL-EMU: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK-PARTIAL-EMU: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PARTIAL-EMU: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
//
                                                                                              /* -2^63 */
// CHECK-PARTIAL-EMU: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0xC3E0000000000000)
                                                                            /* 2^63 - 1 */
// CHECK-PARTIAL-EMU-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge float %[[CLAMP_MIN]], 0x43E0000000000000
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptosi float %[[CLAMP_MIN]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_LO]]
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[INT_MAX_CMP]], i32 2147483647, i32 %[[CONV_HI]]
//
// CHECK-PARTIAL-EMU: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK-PARTIAL-EMU: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK-PARTIAL-EMU: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(long, float)
// CHECK-PARTIAL-EMU-LABEL: define spir_kernel void @test_convert_ulong_float
// CHECK-PARTIAL-EMU: %[[FP_SRC:.+]] = load float, ptr addrspace(1) %src
// CHECK-PARTIAL-EMU: %[[NAN_CMP:.+]] = fcmp oeq float %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PARTIAL-EMU: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], float %[[FP_SRC]], float 0.000000e+00
//
// CHECK-PARTIAL-EMU: %[[CLAMP_MIN:.+]] = call float @llvm.maxnum.f32(float %[[CLAMP_NAN]], float 0.000000e+00)
                                                                            /* 2^64 - 1 */
// CHECK-PARTIAL-EMU-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge float %[[CLAMP_MIN]], 0x43F0000000000000
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptoui float %[[CLAMP_MIN]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_LO]]
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_HI]]
//
// CHECK-PARTIAL-EMU: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK-PARTIAL-EMU: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK-PARTIAL-EMU: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ulong, float)

/*////////////////
/// Double tests
*/////////////////

// CHECK-PARTIAL-EMU-LABEL: define spir_kernel void @test_convert_long_double
// CHECK-PARTIAL-EMU: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK-PARTIAL-EMU: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PARTIAL-EMU: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
//
                                                                                              /* -2^63 */
// CHECK-PARTIAL-EMU: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0xC3E0000000000000)
                                                                            /* 2^63 - 1 */
// CHECK-PARTIAL-EMU-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge double %[[CLAMP_MIN]], 0x43E0000000000000
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptosi double %[[CLAMP_MIN]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_LO]]
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[INT_MAX_CMP]], i32 2147483647, i32 %[[CONV_HI]]
//
// CHECK-PARTIAL-EMU: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK-PARTIAL-EMU: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK-PARTIAL-EMU: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(long, double)
// CHECK-PARTIAL-EMU-LABEL: define spir_kernel void @test_convert_ulong_double
// CHECK-PARTIAL-EMU: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK-PARTIAL-EMU: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK-PARTIAL-EMU: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
//
// CHECK-PARTIAL-EMU: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0.000000e+00)
                                                                            /* 2^64 - 1 */
// CHECK-PARTIAL-EMU-DAG: %[[INT_MAX_CMP:.+]] = fcmp oge double %[[CLAMP_MIN]], 0x43F0000000000000
// CHECK-PARTIAL-EMU-DAG: %[[CONV:.+]] = fptoui double %[[CLAMP_MIN]] to i64
// CHECK-PARTIAL-EMU-DAG: %[[CONV_CAST:.+]] = bitcast i64 %[[CONV]] to <2 x i32>
// CHECK-PARTIAL-EMU-DAG: %[[CONV_LO:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 0
// CHECK-PARTIAL-EMU-DAG: %[[CONV_HI:.+]] = extractelement <2 x i32> %[[CONV_CAST]], i32 1
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_LO:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_LO]]
// CHECK-PARTIAL-EMU-DAG: %[[CLAMP_MAX_HI:.+]] = select i1 %[[INT_MAX_CMP]], i32 -1, i32 %[[CONV_HI]]
//
// CHECK: %[[CONV_SAT_LO:.+]] = insertelement <2 x i32> undef, i32 %[[CLAMP_MAX_LO]], i32 0
// CHECK: %[[CONV_SAT_VEC:.+]] = insertelement <2 x i32> %[[CONV_SAT_LO]], i32 %[[CLAMP_MAX_HI]], i32 1
// CHECK-PARTIAL-EMU: store <2 x i32> %[[CONV_SAT_VEC]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ulong, double)
#endif // CHECK_HALF_ONLY
