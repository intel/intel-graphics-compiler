/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if zeinfo "debug_env" and it fields are not set. Runtime does
// not require bti 0 and bindless offset is managed by driver.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" \
// RUN:     -internal_options "-kernel-debug-enable" \
// RUN:     -device pvc | FileCheck %s

// CHECK:      kernels:
// CHECK-NEXT:   - name:            test
// CHECK:          execution_env:
// CHECK:          payload_arguments:
// CHECK:          per_thread_memory_buffers:
// CHECK-NOT:      debug_env:
// CHECK-NOT:        sip_surface_bti:
// CHECK-NOT:        sip_surface_offset:
// CHECK:      kernels_misc_info:

void kernel test(global int* in, global int* out) {
    volatile int tmp = in[0];
    out[0] = tmp;
}
