/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, tgl-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device tgl | FileCheck %s --check-prefix=CHECK-ASM

// This test verifies that the sg_id value was generated as simd1 value.
__kernel void test1(__global int* output_buffer) {
    int sg_id = get_sub_group_id();
    // CHECK-ASM: (W) add (1|M0) r{{[0-9]*}}.{{[0-9]*}}<1>:d r{{[0-9]*}}.{{[0-9]*}}<0;1,0>:d 222:w
    int res = sg_id + 222;
    output_buffer[sg_id] = res;
}

__attribute__((intel_reqd_sub_group_size(16)))
__kernel void test2(__global int* output_buffer) {
    int sg_id = get_sub_group_id();
    // CHECK-ASM: (W) add (1|M0) r{{[0-9]*}}.{{[0-9]*}}<1>:d r{{[0-9]*}}.{{[0-9]*}}<0;1,0>:d 223:w
    int res = sg_id + 223;
    output_buffer[sg_id] = res;
}