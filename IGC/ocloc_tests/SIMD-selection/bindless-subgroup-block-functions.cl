/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported
// RUN: ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" \
// RUN:       -internal_options "-cl-intel-use-bindless-mode -cl-intel-use-bindless-advanced-mode" -device dg2 2>&1 \
// RUN: | FileCheck %s

// CHECK: .kernel_attr SimdSize=16

#pragma OPENCL EXTENSION cl_intel_subgroups : enable

kernel void test(global uint* in, global uint* out)
{
    uint8 v = intel_sub_group_block_read8(in);
    intel_sub_group_block_write8(out, v);
}
