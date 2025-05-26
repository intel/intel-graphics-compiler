/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, pvc-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device pvc \
// RUN: -options "-I %S -cl-std=CL3.0 -igc_opts 'EnableOpaquePointersBackend=1 PrintToConsole=1 PrintBefore=EmitPass'" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck --enable-var-scope %s

#include "test_convert_sat_helper.h"

/*////////////////
/// i8 tests
*/////////////////

// CHECK-LABEL: define spir_kernel void @test_convert_char_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double -1.280000e+02)
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 1.270000e+02)
// CHECK: %[[CONV:.+]] = fptosi double %[[CLAMP_MAX]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(char, double)
// CHECK-LABEL: define spir_kernel void @test_convert_uchar_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0.000000e+00)
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 2.550000e+02)
// CHECK: %[[CONV:.+]] = fptoui double %[[CLAMP_MAX]] to i8
// CHECK: store i8 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uchar, double)

/*////////////////
/// i16 tests
*/////////////////

// CHECK-LABEL: define spir_kernel void @test_convert_short_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double -3.276800e+04)
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 3.276700e+04)
// CHECK: %[[CONV:.+]] = fptosi double %[[CLAMP_MAX]] to i16
// CHECK: store i16 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(short, double)
// CHECK-LABEL: define spir_kernel void @test_convert_ushort_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0.000000e+00)
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 6.553500e+04)
// CHECK: %[[CONV:.+]] = fptoui double %[[CLAMP_MAX]] to i16
// CHECK: store i16 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(ushort, double)

/*////////////////
/// i32 tests
*/////////////////

// CHECK-LABEL: define spir_kernel void @test_convert_int_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
                                                                                   /* -2.147483648e+09 */
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0xC1E0000000000000)
                                                                                   /* 2.147483647e+09 */
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 0x41DFFFFFFFC00000)
// CHECK: %[[CONV:.+]] = fptosi double %[[CLAMP_MAX]] to i32
// CHECK: store i32 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(int, double)
// CHECK-LABEL: define spir_kernel void @test_convert_uint_double
// CHECK-LABEL: define spir_kernel void @test_convert_int_double
// CHECK: %[[FP_SRC:.+]] = load double, ptr addrspace(1) %src
// CHECK: %[[NAN_CMP:.+]] = fcmp oeq double %[[FP_SRC]], %[[FP_SRC]]
// CHECK: %[[CLAMP_NAN:.+]] = select i1 %[[NAN_CMP]], double %[[FP_SRC]], double 0.000000e+00
// CHECK: %[[CLAMP_MIN:.+]] = call double @llvm.maxnum.f64(double %[[CLAMP_NAN]], double 0.000000e+00)
                                                                                   /* 4.294967295e+09 */
// CHECK: %[[CLAMP_MAX:.+]] = call double @llvm.minnum.f64(double %[[CLAMP_MIN]], double 0x41EFFFFFFFE00000)
// CHECK: %[[CONV:.+]] = fptoui double %[[CLAMP_MAX]] to i32
// CHECK: store i32 %[[CONV]], ptr addrspace(1) %dst
test_convert_sat_intty_to_fpty(uint, double)
