/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported

// RUN: not ocloc compile -file %s -device dg2 -options "-cl-fp64-gen-conv-emu -igc_opts 'PrintToConsole=1'" -internal_options "-cl-ext=-all,+cl_khr_fp64" 2>&1 | FileCheck %s --check-prefix=CHECK-ERR

// CHECK-ERR: Double arithmetic operation is not supported on this platform with FP64 conversion emulation mode (poison FP64 kernels is disabled).

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void conversion_kernel(__global double* inA, __global uint* out)
{
    size_t id = get_global_id(0);
    out[id] = convert_int(sqrt(inA[id]));
}
