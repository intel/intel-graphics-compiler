/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device cri | FileCheck %s --check-prefix=CHECK-VISA

uint __builtin_IB_get_region_group_size(int dim);
kernel void test(global uint *out, int dim) {
// CHECK-VISA-DAG:    (P1) sel (M1_NM, 1) {{.*}}(0,0)<1> [[REGION_GROUP_SIZE:.*]](0,0)<0;1,0> 0x0:d
// CHECK-VISA-DAG:    bfn.xd8 (M1_NM, 1) {{.*}}(0,0)<1> {{.*}}(0,0)<0;1,0> [[REGION_GROUP_SIZE]](0,1)<0;1,0> {{.*}}(0,0)<0;1,0>
// CHECK-VISA-DAG:    bfn.xd8 (M1_NM, 1) {{.*}}(0,0)<1> {{.*}}(0,0)<0;1,0> [[REGION_GROUP_SIZE]](0,2)<0;1,0> {{.*}}(0,0)<0;1,0>
  out[0] = __builtin_IB_get_region_group_size(dim);
}

// CHECK-VISA-DAG: .input [[REGION_GROUP_SIZE]] offset=92 size=12
