/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DROUNDING=0" \
// RUN: | FileCheck %s --check-prefix=CHECK-RNE-1

// CHECK-RNE-1-NOT:  xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
// CHECK-RNE-1:      sqrtm (M1, 16) {{.*}} {{.*}}
// CHECK-RNE-1-NOT:  xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> {{.*}}
// CHECK-RNE-1:      ret {{.*}}

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DROUNDING=1" \
// RUN: | FileCheck %s --check-prefix=CHECK-RTP-1

// CHECK-RTP-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
// CHECK-RTP-1: sqrtm (M1, 16) {{.*}} {{.*}}
// CHECK-RTP-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x10:ud {{.*}}
// CHECK-RTP-1: ret {{.*}}

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DROUNDING=2" \
// RUN: | FileCheck %s --check-prefix=CHECK-RTN-1

// CHECK-RTN-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
// CHECK-RTN-1: sqrtm (M1, 16) {{.*}} {{.*}}
// CHECK-RTN-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud {{.*}}
// CHECK-RTN-1: ret {{.*}}

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DROUNDING=3" \
// RUN: | FileCheck %s --check-prefix=CHECK-RTZ-1

// CHECK-RTZ-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
// CHECK-RTZ-1: sqrtm (M1, 16) {{.*}} {{.*}}
// CHECK-RTZ-1: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x30:ud {{.*}}
// CHECK-RTZ-1: ret {{.*}}


float   __builtin_IB_ieee_sqrt_rm(float, int) __attribute__((const));

kernel void test_div(global float* a, global float* b) {
    size_t gid = get_global_id(0);
    b[gid] = __builtin_IB_ieee_sqrt_rm(a[gid], ROUNDING);
}
