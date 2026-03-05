/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, pvc-supported

// Verify that register pressure estimator correctly handles intel_reqd_sub_group_size metadata
// being overridden with ForceOCLSIMDWidth flag.

// RUN: rm -rf %t && mkdir %t
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpRegPressureEstimate=1, DumpRegPressureEstimateFilter=no_inline_func, DumpToCustomDir=%t, EnableStackCallFuncCall=1, ForceOCLSIMDWidth=16'" -device pvc
// RUN: cat %t/*_no_inline_func_final_RegEst.ll | FileCheck %s --check-prefix=CHECK

// CHECK: SIMD: 16, external pressure: 0

__attribute__((noinline))
double no_inline_func(double a) {
    return a + 39.0f;
}
__attribute__((intel_reqd_sub_group_size(32)))
kernel void foo(global double *a1, global double *a2) {
    int tid = get_global_id(0);
    a1[tid] = no_inline_func(a1[tid]);
}
