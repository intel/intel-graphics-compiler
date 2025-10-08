/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: bmg-supported

// RUN: ocloc compile -file %s -device bmg -options "-cl-std=CL1.2" 2>&1 | FileCheck %s --check-prefixes=CHECK-CL12
// RUN: ocloc compile -file %s -device bmg -options "-cl-std=CL2.0" 2>&1 | FileCheck %s --check-prefixes=CHECK-CL20
// RUN: ocloc compile -file %s -device bmg -options "-cl-std=CL3.0" 2>&1 | FileCheck %s --check-prefixes=CHECK-CL30

// CHECK-CL12-NOT: error: use of undeclared identifier 'dot_acc_sat'
// CHECK-CL20-NOT: error: use of undeclared identifier 'dot_acc_sat'
// CHECK-CL30-NOT: error: use of undeclared identifier 'dot_acc_sat'

kernel void test(uchar4 a, uchar4 b, uint acc, global uint* output) {
    uint result = dot_acc_sat(a, b, acc);
    uint id = get_global_id(0);
    output[id] = result;
}
