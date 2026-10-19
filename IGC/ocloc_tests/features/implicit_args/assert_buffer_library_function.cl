/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// A library function is compiled as a stack call, so it can't get the assert buffer as a kernel
// payload argument. It has to request the buffer with require_assert_buffer in its zeinfo, so that
// the runtime allocates it for kernels linking the function, and read the pointer from the
// implicit args buffer.
// The frontend doesn't accept -cl-intel-library-compilation, so it's passed to IGC with SPIR-V input.
// With -cl-opt-disable the static helper stays a stack call, which isn't exported, so the flag has to be set
// on its exported caller.
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -spv_only -options "-cl-std=CL3.0" -output_no_suffix -output %t-dg2 %}
// RUN: %if dg2-supported %{ ocloc compile -spirv_input -file %t-dg2.spv -device dg2 -options "-cl-intel-library-compilation -igc_opts 'DumpZEInfoToConsole=1'" | FileCheck %s %}
// RUN: %if dg2-supported %{ ocloc compile -spirv_input -file %t-dg2.spv -device dg2 -options "-cl-intel-library-compilation -cl-opt-disable -igc_opts 'DumpZEInfoToConsole=1'" | FileCheck %s %}
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -spv_only -options "-cl-std=CL3.0" -output_no_suffix -output %t-cri %}
// RUN: %if cri-supported %{ ocloc compile -spirv_input -file %t-cri.spv -device cri -options "-cl-intel-library-compilation -igc_opts 'DumpZEInfoToConsole=1'" | FileCheck %s %}
// RUN: %if cri-supported %{ ocloc compile -spirv_input -file %t-cri.spv -device cri -options "-cl-intel-library-compilation -cl-opt-disable -igc_opts 'DumpZEInfoToConsole=1'" | FileCheck %s %}

// CHECK-LABEL: functions:
// CHECK: - name: check_fun
// CHECK-NEXT: execution_env:
// CHECK-NOT: - name:
// CHECK: require_assert_buffer: true
// CHECK: - name: check_via_helper
// CHECK-NEXT: execution_env:
// CHECK-NOT: - name:
// CHECK: require_assert_buffer: true

// Device-side assert entry point implemented in BiF (assert.cl). String literals are __constant, which
// can't be passed as generic pointers, so the message comes in as a global argument.
void __devicelib_assert_fail(char *expr, char *file, int line, char *func, long gid0, long gid1, long gid2, long lid0,
                             long lid1, long lid2);

void check_fun(global int *buffer, int xid, global char *msg) {
  if (buffer[xid] < 0)
    __devicelib_assert_fail(msg, msg, 0, msg, xid, 0, 0, 0, 0, 0);
}

static void check_helper(global int *buffer, int xid, global char *msg) {
  if (buffer[xid] < 0)
    __devicelib_assert_fail(msg, msg, 0, msg, xid, 0, 0, 0, 0, 0);
}

void check_via_helper(global int *buffer, int xid, global char *msg) { check_helper(buffer, xid, msg); }
