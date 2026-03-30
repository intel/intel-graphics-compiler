/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: pvc-supported, regkeys, llvm-14-plus

// RUN: ocloc compile -file %s -internal_options "-cl-ext=-all,+cl_khr_kernel_clock" -options "-igc_opts 'DumpVISAASMToConsole=1'" -device pvc | FileCheck %s

// CHECK-LABEL: .kernel "test_clock_read_device"
__kernel void test_clock_read_device(__global ulong *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_device();
}

// CHECK-LABEL: .kernel "test_clock_read_work_group"
__kernel void test_clock_read_work_group(__global ulong *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_work_group();
}

// CHECK-LABEL: .kernel "test_clock_read_sub_group"
__kernel void test_clock_read_sub_group(__global ulong *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_sub_group();
}

// CHECK-LABEL: .kernel "test_clock_read_hilo_device"
__kernel void test_clock_read_hilo_device(__global uint2 *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_hilo_device();
}

// CHECK-LABEL: .kernel "test_clock_read_hilo_work_group"
__kernel void test_clock_read_hilo_work_group(__global uint2 *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_hilo_work_group();
}

// CHECK-LABEL: .kernel "test_clock_read_hilo_sub_group"
__kernel void test_clock_read_hilo_sub_group(__global uint2 *p) {
  // CHECK: mov {{.*}} %tsc
  *p = clock_read_hilo_sub_group();
}
