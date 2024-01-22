/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if zeinfo attribute "private_size" is properly set.
// "private_size" represents a total private memory used by a kernel per HW thread.
// In the below test, private memory is only used to keep `tmp` variable.
// Since `tmp` variable's size is 4 bytes, "private_size" is expected to be
// set to 4 * 32 = 128.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" \
// RUN:     -device pvc | FileCheck %s

// CHECK:     private_size:    128

__attribute__((intel_reqd_sub_group_size(32)))
void kernel test(global int* in, global int* out) {
    volatile int tmp = in[0];
    out[0] = tmp;
}
