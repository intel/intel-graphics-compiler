/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// ForceOCLSIMDWidth must be honored even where the SIMD selection heuristics
// would pick another width. Each RUN line forces a width that differs from the
// heuristic choice for that kernel and platform.

// Stack calls with ForceLowestSIMDForStackCalls, which picks SIMD16 on PVC.
// RUN: %if pvc-supported %{ ocloc compile -file %s -device pvc -options "-igc_opts 'ForceOCLSIMDWidth=32,FunctionControl=3,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix CHECK-SIMD32 %}

// Subroutines, which pick SIMD8 on DG2.
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-igc_opts 'ForceOCLSIMDWidth=16,FunctionControl=2,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix CHECK-SIMD16 %}
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-igc_opts 'ForceOCLSIMDWidth=32,FunctionControl=2,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix CHECK-SIMD32 %}

// Nested stack calls on DG2, which needs CallWA for SIMD32. Forced SIMD32
// disables EU fusion instead of dropping the SIMD size.
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-igc_opts 'ForceOCLSIMDWidth=32,FunctionControl=3,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes CHECK-EU-FUSION,CHECK-SIMD32 %}

// intel_reqd_sub_group_size takes priority over ForceOCLSIMDWidth.
// RUN: %if pvc-supported %{ ocloc compile -file %s -device pvc -options "-DREQD_SUB_GROUP_SIZE=16 -igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix CHECK-SIMD16 %}
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-DREQD_SUB_GROUP_SIZE=16 -igc_opts 'ForceOCLSIMDWidth=32,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix CHECK-SIMD16 %}

// CHECK-SIMD16: .kernel_attr SimdSize=16
// CHECK-SIMD32: .kernel_attr SimdSize=32
// CHECK-EU-FUSION: warning: {{.*}}EU fusion is disabled, it does not work on the current platform if SIMD32 mode specified by forced SIMD size(32)

// Two call levels, so that stack calls are nested calls for CallWA.
__attribute__((noinline)) int load(__global int *src, int id) { return src[id]; }
__attribute__((noinline)) int compute(__global int *src, int id) { return load(src, id) + 1; }

#ifdef REQD_SUB_GROUP_SIZE
__attribute__((intel_reqd_sub_group_size(REQD_SUB_GROUP_SIZE)))
#endif
__kernel void test(__global int *dst, __global int *src) {
  int id = get_global_id(0);
  dst[id] = compute(src, id);
}
