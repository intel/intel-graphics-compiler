/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// Verify that implicit kernel arguments are removed when unused.

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'RemoveUnusedIdImplicitArguments=0, ShortImplicitPayloadHeader=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-DONT-REMOVE
// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'RemoveUnusedIdImplicitArguments=1, ShortImplicitPayloadHeader=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-REMOVE

// CHECK-DONT-REMOVE:      payload_arguments:
// CHECK-DONT-REMOVE-NEXT:   - arg_type:        global_id_offset
// CHECK-DONT-REMOVE-NEXT:     offset:          0
// CHECK-DONT-REMOVE-NEXT:     size:            12
// CHECK-DONT-REMOVE-NEXT:   - arg_type:        arg_bypointer
// CHECK-DONT-REMOVE-NEXT:     offset:          16
// CHECK-DONT-REMOVE-NEXT:     size:            8
// CHECK-DONT-REMOVE-NEXT:     arg_index:       0
// CHECK-DONT-REMOVE-NEXT:     addrmode:        stateless
// CHECK-DONT-REMOVE-NEXT:     addrspace:       global
// CHECK-DONT-REMOVE-NEXT:     access_type:     readwrite

// CHECK-REMOVE:           payload_arguments:
// CHECK-REMOVE-NEXT:        - arg_type:        arg_bypointer
// CHECK-REMOVE-NEXT:          offset:          0
// CHECK-REMOVE-NEXT:          size:            8
// CHECK-REMOVE-NEXT:          arg_index:       0
// CHECK-REMOVE-NEXT:          addrmode:        stateless
// CHECK-REMOVE-NEXT:          addrspace:       global
// CHECK-REMOVE-NEXT:          access_type:     readwrite

kernel void test(global float* ptr) {}
