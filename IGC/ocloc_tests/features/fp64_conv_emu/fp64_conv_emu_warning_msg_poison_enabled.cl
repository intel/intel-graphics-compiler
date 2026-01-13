/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The change adds the test that verifies if a warning is emitted when the platform
// can emulate DP conv operations, but DP arithmetic operations are used
// in the kernel and poisonFP64Kernels is enabled.

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-cl-poison-unsupported-fp64-kernels -cl-fp64-gen-conv-emu -igc_opts 'PrintToConsole=1'" -internal_options "-cl-ext=-all,+cl_khr_fp64" 2>&1 | FileCheck %s --check-prefix=CHECK

// CHECK: Double arithmetic operation is not supported on this platform with FP64 conversion emulation mode (poison FP64 kernels is enabled).

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void add_kernel(__global double* inA, __global double* inB, __global uint* out)
{
    size_t id = get_global_id(0);
    out[id] = inA[id] + inB[id];
}
