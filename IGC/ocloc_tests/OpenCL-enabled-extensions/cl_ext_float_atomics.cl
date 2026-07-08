/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: bmg-supported

// RUN: not ocloc compile -file %s -device bmg -options "-cl-std=CL1.2"
// RUN: ocloc compile -file %s -device bmg -options "-cl-std=CL2.0" 2>&1 | FileCheck %s --check-prefixes=CHECK-CL20
// RUN: ocloc compile -file %s -device bmg -options "-cl-std=CL2.0" 2>&1 | FileCheck %s --check-prefixes=CHECK-CL30

// CHECK-CL20-NOT: error: use of undeclared identifier 'atomic_fetch_add_explicit'
// CHECK-CL30-NOT: error: use of undeclared identifier 'atomic_fetch_add_explicit'

__kernel void atomic_add_example(__global float *data, __global float *result)
{
    // Each work-item loads a float value from the input array
    uint gid = get_global_id(0);
    float value = data[gid];

    // Atomically add 'value' to the shared result
    atomic_fetch_add_explicit(
        (__global atomic_float *)result,
        value,
        memory_order_relaxed,
        memory_scope_device
    );
}
