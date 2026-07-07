/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify RemoveImplicitScratchPointerInstThreshold.

// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, RemoveImplicitScratchPointerInstThreshold=1000'" -device cri | FileCheck %s --check-prefix=CHECK-BELOW-THRESHOLD
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpZEInfoToConsole=1, RemoveUnusedIdImplicitArguments=1, RemoveImplicitScratchPointer=1, RemoveImplicitScratchPointerInstThreshold=10'"   -device cri | FileCheck %s --check-prefix=CHECK-ABOVE-THRESHOLD

// CHECK-BELOW-THRESHOLD:      payload_arguments:
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        indirect_data_pointer
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          0
// CHECK-BELOW-THRESHOLD-NEXT:     size:            8
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        global_id_offset
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          8
// CHECK-BELOW-THRESHOLD-NEXT:     size:            12
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        enqueued_local_size
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          20
// CHECK-BELOW-THRESHOLD-NEXT:     size:            12
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          32
// CHECK-BELOW-THRESHOLD-NEXT:     size:            8
// CHECK-BELOW-THRESHOLD-NEXT:     arg_index:       0
// CHECK-BELOW-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-BELOW-THRESHOLD-NEXT:     addrspace:       global
// CHECK-BELOW-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          40
// CHECK-BELOW-THRESHOLD-NEXT:     size:            8
// CHECK-BELOW-THRESHOLD-NEXT:     arg_index:       1
// CHECK-BELOW-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-BELOW-THRESHOLD-NEXT:     addrspace:       global
// CHECK-BELOW-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          48
// CHECK-BELOW-THRESHOLD-NEXT:     size:            8
// CHECK-BELOW-THRESHOLD-NEXT:     arg_index:       2
// CHECK-BELOW-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-BELOW-THRESHOLD-NEXT:     addrspace:       global
// CHECK-BELOW-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-BELOW-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-BELOW-THRESHOLD-NEXT:     offset:          56
// CHECK-BELOW-THRESHOLD-NEXT:     size:            8
// CHECK-BELOW-THRESHOLD-NEXT:     arg_index:       3
// CHECK-BELOW-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-BELOW-THRESHOLD-NEXT:     addrspace:       global
// CHECK-BELOW-THRESHOLD-NEXT:     access_type:     readwrite
//
// CHECK-BELOW-THRESHOLD-NOT:  scratch_pointer
// CHECK-BELOW-THRESHOLD-NOT:  Start recompilation of the kernel

// CHECK-ABOVE-THRESHOLD:      payload_arguments:
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        indirect_data_pointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          0
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        scratch_pointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          8
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        global_id_offset
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          16
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            12
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        enqueued_local_size
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          28
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            12
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          40
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:     arg_index:       0
// CHECK-ABOVE-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-ABOVE-THRESHOLD-NEXT:     addrspace:       global
// CHECK-ABOVE-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          48
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:     arg_index:       1
// CHECK-ABOVE-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-ABOVE-THRESHOLD-NEXT:     addrspace:       global
// CHECK-ABOVE-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          56
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:     arg_index:       2
// CHECK-ABOVE-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-ABOVE-THRESHOLD-NEXT:     addrspace:       global
// CHECK-ABOVE-THRESHOLD-NEXT:     access_type:     readwrite
// CHECK-ABOVE-THRESHOLD-NEXT:   - arg_type:        arg_bypointer
// CHECK-ABOVE-THRESHOLD-NEXT:     offset:          64
// CHECK-ABOVE-THRESHOLD-NEXT:     size:            8
// CHECK-ABOVE-THRESHOLD-NEXT:     arg_index:       3
// CHECK-ABOVE-THRESHOLD-NEXT:     addrmode:        stateless
// CHECK-ABOVE-THRESHOLD-NEXT:     addrspace:       global
// CHECK-ABOVE-THRESHOLD-NEXT:     access_type:     readwrite

kernel void foo(global float* a1, global float* a2, global float* a3, global float* a4) {
    int  i = get_global_id(0);
    a1[i+1] = a1[i];
    a2[i+2] = a2[i];
    a3[i+3] = a3[i];
}
