/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'DumpVISAASMToConsole=1,AddVISADumpDeclarationsToEnd=1,ForceOCLSIMDWidth=8'" | FileCheck %s

// CHECK-LABEL: .kernel "foo"
// CHECK: add (M1_NM, 1) [[DST:.*]](0,0)<1> [[SRC_A:.*]](0,0)<0;1,0> [[SRC_B:.*]](0,0)<0;1,0>
// CHECK: lsc_store{{.*}} (M1_NM, 1) {{.*}} [[DST]]
// CHECK-DAG: // .decl [[SRC_A]] v_type=G type=d num_elts=1
// CHECK-DAG: // .decl [[SRC_B]] v_type=G type=d num_elts=1
// CHECK-DAG: // .decl [[DST]] v_type=G type=d num_elts=1

__kernel void foo(int a, int b, __global int *res) {
    *res = a + b;
}
