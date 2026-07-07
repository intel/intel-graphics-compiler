/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,bmg-supported,llvm-14-plus

// Verify that local IDs are not removed when subroutines/stackcalls are used.

// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'EnableStackCallFuncCall=0, RemoveUnusedIdImplicitLocalIDs=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-SUBROUTINE
// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'EnableStackCallFuncCall=1, RemoveUnusedIdImplicitLocalIDs=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL

// CHECK-SUBROUTINE-LABEL: - name: test_kernel
// CHECK-SUBROUTINE:         per_thread_payload_arguments:
// CHECK-SUBROUTINE-NEXT:      - arg_type: local_id
// CHECK-SUBROUTINE-NEXT:        offset: 0
// CHECK-SUBROUTINE-NEXT:        size: 192

// CHECK-STACKCALL-LABEL:  - name: test_kernel
// CHECK-STACKCALL:          per_thread_payload_arguments:
// CHECK-STACKCALL-NEXT:       - arg_type: local_id
// CHECK-STACKCALL-NEXT:         offset: 0
// CHECK-STACKCALL-NEXT:         size: 192

__attribute__((noinline))
void noinline_function(global float* ptr)
{
    const int gid = get_global_id(0);
    ptr[gid] = get_global_id(0);
}

kernel void test_kernel(global float *ptr)
{
    noinline_function(ptr);
}
