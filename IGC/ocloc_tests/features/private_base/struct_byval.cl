/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys
// UNSUPPORTED: system-windows

// Verify that if kernel uses struct byval argument, the private_base implicit argument is not added.

// RUN: %if bmg-supported %{ ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1'" -device bmg | FileCheck %s --check-prefixes=CHECK %}

// CHECK:      payload_arguments:
// CHECK-NEXT: - arg_type:        global_id_offset
// CHECK-NEXT:   offset:          0
// CHECK-NEXT:   size:            12
// CHECK-NEXT: - arg_type:        enqueued_local_size
// CHECK-NEXT:   offset:          12
// CHECK-NEXT:   size:            12
// CHECK-NEXT: - arg_type:        arg_bypointer
// CHECK-NEXT:   offset:          24
// CHECK-NEXT:   size:            8
// CHECK-NEXT:   arg_index:       0
// CHECK-NEXT:   addrmode:        stateless
// CHECK-NEXT:   addrspace:       global
// CHECK-NEXT:   access_type:     readwrite
// CHECK-NEXT: - arg_type:        arg_byvalue
// CHECK-NEXT:   offset:          32
// CHECK-NEXT:   size:            4
// CHECK-NEXT:   arg_index:       1
// CHECK-NEXT:   source_offset:   0
//
// CHECK-NOT:    private_base_stateless

struct Foo { int a; };

kernel void foo(global int *p, struct Foo foo) {
    int id = get_global_id(0);
    p[id] = foo.a + 39;
}
