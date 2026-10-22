/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// RUN: %if pvc-supported %{ ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintAfter=igc-error-check'" 2>&1 | FileCheck %s --implicit-check-not=__stgamma %}
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -options "-cl-std=CL2.0 -igc_opts 'PrintToConsole=1 PrintAfter=igc-error-check'" 2>&1 | FileCheck %s --implicit-check-not=__stgamma %}

// float lgamma_r must take its value from lgamma (IMF __slgamma tables), not from
// log(|tgamma(x)|): tgamma overflows for x > ~35 although lgamma is finite up to ~4e36.
// Checked after GlobalDCE, which drops the BiF tables that are not used.

// CHECK: @__slgamma_nofp64

kernel void test_scalar(global float *out, global float *in, global int *sign) {
  size_t i = get_global_id(0);
  out[i] = lgamma_r(in[i], &sign[i]);
}

kernel void test_vec16(global float16 *out, global float16 *in, local int16 *sign) {
  size_t i = get_global_id(0);
  out[i] = lgamma_r(in[i], &sign[get_local_id(0)]);
}
