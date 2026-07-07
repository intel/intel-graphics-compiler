/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if log2, exp2 and rsqrt calls use native math functions on
// platforms with appropriate accuracy in the hardware.

// RUN: %if xe3p-lpg-supported %{ ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device xe3p-lpg | FileCheck %s --check-prefix=CHECK-XE3PLPG %}
// RUN: %if cri-supported %{ ocloc compile -file %s -options "-igc_opts 'VISAOptions=-asmToConsole'" -device cri | FileCheck %s --check-prefix=CHECK-XE3P %}

kernel void test(global float* out) {
// CHECK-XE3PLPG: {{.*}} math.log {{.*}}
// CHECK-XE3P: {{.*}} math.log {{.*}}
  out[0] = log2(out[0]);
// CHECK-XE3PLPG: {{.*}} math.exp {{.*}}
// CHECK-XE3P: {{.*}} math.exp {{.*}}
  out[1] = exp2(out[0]);
// CHECK-XE3PLPG: {{.*}} math.rsqt {{.*}}
// CHECK-XE3P: {{.*}} math.rsqt {{.*}}
  out[2] = rsqrt(out[0]);
}
