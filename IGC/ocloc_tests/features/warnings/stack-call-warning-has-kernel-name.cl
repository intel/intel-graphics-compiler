/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that the "Stack call has been detected" warning includes the kernel
// name, so users can tell which kernel triggered the warning when compiling
// multiple kernels for multiple architectures.

// REQUIRES: dg2-supported
// RUN: ocloc compile -file %s -device dg2 2>&1 | FileCheck %s

// CHECK: warning: in kernel 'test_kernel': Stack call has been detected

int fact(int n) {
    return n < 2 ? 1 : n * fact(n - 1);
}

kernel void test_kernel(global int* out, int n) {
    out[get_global_id(0)] = fact(n);
}
