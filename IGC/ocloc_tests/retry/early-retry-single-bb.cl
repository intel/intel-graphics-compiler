/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys
// RUN: ocloc compile -file %s -options "-igc_opts 'EarlyRetryDefaultGRFThreshold=5,EarlyRetryLargeGRFThreshold=5'" \
// RUN: -device mtl | FileCheck %s --check-prefix=CHECK

// a kernel with 1 BB, should not be recompiled even in case of high register pressure
// even higher than existing early retry thresholds

// CHECK-NOT: [RetryManager] Start recompilation of the kernel

__kernel void add_mul_pressure(__global const float* in, __global float* out) {
    int gid = get_global_id(0);
    float a = in[gid];
    float b = a + 1.0f;
    float c = b * 2.0f;
    float d = c + 3.0f;
    float e = d * 4.0f;
    float f = e + 5.0f;
    float g = f * 6.0f;
    float h = g + 7.0f;
    float i = h * 8.0f;
    float j = i + 9.0f;
    float k = j * 10.0f;
    float l = k + 11.0f;
    // Use all variables to ensure register pressure
    float result = a + b + c + d + e + f + g + h + i + j + k + l;
    out[gid] = result;
}
