/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,bmg-supported,llvm-14-plus

// Verify that local IDs are removed when unused.

// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'RemoveUnusedIdImplicitLocalIDs=0, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-DONT-REMOVE
// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'RemoveUnusedIdImplicitLocalIDs=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-REMOVE

// When removing is not enabled, all three local IDs are always present if at least one is used.
//
// CHECK-DONT-REMOVE-LABEL: - name: unused_id
// CHECK-DONT-REMOVE-NOT:     per_thread_payload_arguments:
// CHECK-DONT-REMOVE-NOT:       - arg_type: local_id
// CHECK-DONT-REMOVE-LABEL: - name: used_x
// CHECK-DONT-REMOVE:         per_thread_payload_arguments:
// CHECK-DONT-REMOVE-NEXT:      - arg_type: local_id
// CHECK-DONT-REMOVE-NEXT:        offset: 0
// CHECK-DONT-REMOVE-NEXT:        size: 192
// CHECK-DONT-REMOVE-LABEL: - name: used_y
// CHECK-DONT-REMOVE:         per_thread_payload_arguments:
// CHECK-DONT-REMOVE-NEXT:      - arg_type: local_id
// CHECK-DONT-REMOVE-NEXT:        offset: 0
// CHECK-DONT-REMOVE-NEXT:        size: 192
// CHECK-DONT-REMOVE-LABEL: - name: used_z
// CHECK-DONT-REMOVE:         per_thread_payload_arguments:
// CHECK-DONT-REMOVE-NEXT:      - arg_type: local_id
// CHECK-DONT-REMOVE-NEXT:        offset: 0
// CHECK-DONT-REMOVE-NEXT:        size: 192

// When removing is enabled, all local IDs up to highest used dimension are used.
//
// CHECK-REMOVE-LABEL:      - name: unused_id
// CHECK-REMOVE-NOT:          per_thread_payload_arguments:
// CHECK-REMOVE-NOT:            - arg_type: local_id
// CHECK-REMOVE-LABEL:      - name: used_x
// CHECK-REMOVE:              per_thread_payload_arguments:
// CHECK-REMOVE-NEXT:           - arg_type: local_id
// CHECK-REMOVE-NEXT:             offset: 0
// CHECK-REMOVE-NEXT:             size: 64
// CHECK-REMOVE-LABEL:      - name: used_y
// CHECK-REMOVE:              per_thread_payload_arguments:
// CHECK-REMOVE-NEXT:           - arg_type: local_id
// CHECK-REMOVE-NEXT:             offset: 0
// CHECK-REMOVE-NEXT:             size: 128
// CHECK-REMOVE-LABEL:      - name: used_z
// CHECK-REMOVE:              per_thread_payload_arguments:
// CHECK-REMOVE-NEXT:           - arg_type: local_id
// CHECK-REMOVE-NEXT:             offset: 0
// CHECK-REMOVE-NEXT:             size: 192

kernel void unused_id(global int *values) {
    *values = 39;
}

kernel void used_x(global long *a1, global int *values) {
    const int gid = get_global_id(0);
    values[gid] = get_global_id(0);
}

kernel void used_y(global int *values) {
    const int gid = get_global_id(1);
    values[gid] = get_global_id(1);
}

kernel void used_z(global int *values) {
    const int gid = get_global_id(2);
    values[gid] = get_global_id(2);
}
