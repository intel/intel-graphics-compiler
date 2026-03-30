/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys
// UNSUPPORTED: system-windows

// Verify that private_base implicit argument is not removed if kernel uses stackcalls.

// RUN: %if bmg-supported %{ ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, EnableStackCallFuncCall=1'" -device bmg | FileCheck %s --check-prefixes=CHECK %}

// CHECK:      has_stack_calls: true
// CHECK:      payload_arguments:
// CHECK:      - arg_type:        private_base_stateless
// CHECK-NEXT:   offset:          40
// CHECK-NEXT:   size:            8

struct Foo { int a; };

__attribute__((noinline))
double no_inline_func(struct Foo foo) {
    return foo.a + 39;
}

kernel void foo(global int *p, struct Foo foo) {
    int id = get_global_id(0);
    p[id] = no_inline_func(foo);
}
