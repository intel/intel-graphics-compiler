/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// This test checks if StatelessToStateful optimisation did not occur,
// when Efficient64bit path is enabled.
//
// The test uses "-cl-intel-force-disable-4GB-buffer" to ensure that
// "-cl-intel-greater-than-4GB-buffer-required" is not the one to disable StatelessToStateful,
// but logic that checks for Efficient64bit.

// UNSUPPORTED: sys32
// REQUIRES: cri-supported, opaque-pointers

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-cl-std=CL3.0 -igc_opts 'EnableEfficient64b=1 PrintToConsole=1 PrintAfter=EmitPass'" \
// RUN: -internal_options "-cl-intel-force-disable-4GB-buffer" \
// RUN: -out_dir /dev/null 2>&1 | FileCheck %s

// CHECH: {{.*}} = load i32, ptr addrspace(1) {{.*}}
// CHECK: store i32 {{.*}} ptr addrspace(1) {{.*}}
kernel void test_stateless_to_stateful(global int *out) {
  int lid = get_local_id(0);
  out[lid % 2] = out[lid];
}
