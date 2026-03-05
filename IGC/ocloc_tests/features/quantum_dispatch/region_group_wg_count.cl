/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

// CHECK-VISA-DAG: .input [[REGION_GROUP_WG_COUNT:.*]] offset=88 size=4

uint __builtin_IB_get_region_group_wg_count();
kernel void test(global uint *out) {
  // CHECK-VISA-DAG:     mov (M1_NM, 1) {{.*}}(0,0)<1> [[REGION_GROUP_WG_COUNT]](0,0)<0;1,0>
  out[0] = __builtin_IB_get_region_group_wg_count();
}

// CHECK-VISA-DAG: .decl [[REGION_GROUP_WG_COUNT]] v_type=G type=d num_elts=1 align=dword
