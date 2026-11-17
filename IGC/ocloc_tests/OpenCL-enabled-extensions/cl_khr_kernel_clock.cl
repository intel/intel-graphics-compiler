/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: bmg-supported

// RUN: ocloc compile -file %s -device bmg -internal_options "-cl-ext=-all,+cl_khr_kernel_clock" -options "-cl-std=CL1.2 -Werror"
// RUN: ocloc compile -file %s -device bmg -internal_options "-cl-ext=-all,+cl_khr_kernel_clock,+__opencl_c_kernel_clock_scope_device,+__opencl_c_kernel_clock_scope_work_group,+__opencl_c_kernel_clock_scope_sub_group" -options "-cl-std=CL3.0 -Werror"

kernel void test(global ulong *out) {
    out[0] = clock_read_device();
    out[1] = clock_read_work_group();
    out[2] = clock_read_sub_group();
    out[3] = as_ulong(clock_read_hilo_device());
    out[4] = as_ulong(clock_read_hilo_work_group());
    out[5] = as_ulong(clock_read_hilo_sub_group());
}
