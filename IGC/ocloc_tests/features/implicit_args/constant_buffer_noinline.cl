/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// Verify that implicit kernel arguments are not removed for kernels with subroutines.
// Arguments can be removed for stackcalls.

// RUN: ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'EnableStackCallFuncCall=0, ShortImplicitPayloadHeader=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-SUBROUTINE
// RUN: ocloc compile -file %s -device pvc -options "-cl-std=CL2.0 -igc_opts 'EnableStackCallFuncCall=1, ShortImplicitPayloadHeader=1, DumpZEInfoToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-STACKCALL

// CHECK-SUBROUTINE:      name:            kernel_that_must_have_args
// CHECK-SUBROUTINE:        payload_arguments:
// CHECK-SUBROUTINE-NEXT:     - arg_type:        global_id_offset
// CHECK-SUBROUTINE-NEXT:       offset:          0
// CHECK-SUBROUTINE-NEXT:       size:            12
// CHECK-SUBROUTINE-NEXT:     - arg_type:        enqueued_local_size
// CHECK-SUBROUTINE-NEXT:       offset:          12
// CHECK-SUBROUTINE-NEXT:       size:            12
// CHECK-SUBROUTINE-NEXT:     - arg_type:        arg_bypointer
// CHECK-SUBROUTINE-NEXT:       offset:          24
// CHECK-SUBROUTINE-NEXT:       size:            8
// CHECK-SUBROUTINE-NEXT:       arg_index:       0
// CHECK-SUBROUTINE-NEXT:       addrmode:        stateless
// CHECK-SUBROUTINE-NEXT:       addrspace:       global
// CHECK-SUBROUTINE-NEXT:       access_type:     readwrite
// CHECK-SUBROUTINE-NEXT:   per_thread_payload_arguments:
// CHECK-SUBROUTINE-NEXT:     - arg_type:        local_id
// CHECK-SUBROUTINE-NEXT:       offset:          0
// CHECK-SUBROUTINE-NEXT:       size:            192
//
// CHECK-SUBROUTINE:      name:            kernel_that_can_skip_args
// CHECK-SUBROUTINE:        payload_arguments:
// CHECK-SUBROUTINE-NEXT:     - arg_type:        arg_bypointer
// CHECK-SUBROUTINE-NEXT:       offset:          0
// CHECK-SUBROUTINE-NEXT:       size:            8
// CHECK-SUBROUTINE-NEXT:       arg_index:       0
// CHECK-SUBROUTINE-NEXT:       addrmode:        stateless
// CHECK-SUBROUTINE-NEXT:       addrspace:       global
// CHECK-SUBROUTINE-NEXT:       access_type:     readwrite

// CHECK-STACKCALL:      name:            kernel_that_must_have_args
// CHECK-STACKCALL:        payload_arguments:
// CHECK-STACKCALL-NEXT:     - arg_type:        arg_bypointer
// CHECK-STACKCALL-NEXT:       offset:          0
// CHECK-STACKCALL-NEXT:       size:            8
// CHECK-STACKCALL-NEXT:       arg_index:       0
// CHECK-STACKCALL-NEXT:       addrmode:        stateless
// CHECK-STACKCALL-NEXT:       addrspace:       global
// CHECK-STACKCALL-NEXT:       access_type:     readwrite
// CHECK-STACKCALL-NEXT:     - arg_type:        private_base_stateless
// CHECK-STACKCALL-NEXT:       offset:          8
// CHECK-STACKCALL-NEXT:       size:            8
// CHECK-STACKCALL-NEXT:   per_thread_payload_arguments:
// CHECK-STACKCALL-NEXT:     - arg_type:        local_id
// CHECK-STACKCALL-NEXT:       offset:          0
// CHECK-STACKCALL-NEXT:       size:            192
//
// CHECK-STACKCALL:      name:            kernel_that_can_skip_args
// CHECK-STACKCALL:        payload_arguments:
// CHECK-STACKCALL-NEXT:     - arg_type:        arg_bypointer
// CHECK-STACKCALL-NEXT:       offset:          0
// CHECK-STACKCALL-NEXT:       size:            8
// CHECK-STACKCALL-NEXT:       arg_index:       0
// CHECK-STACKCALL-NEXT:       addrmode:        stateless
// CHECK-STACKCALL-NEXT:       addrspace:       global
// CHECK-STACKCALL-NEXT:       access_type:     readwrite

__attribute__((noinline))
void noinline_function(global float* ptr)
{
    int i = get_global_id(0);
    ptr[i] = get_enqueued_local_size(0);
}

kernel void kernel_that_must_have_args(global float *ptr)
{
    noinline_function(ptr);
}

kernel void kernel_that_can_skip_args(global float *ptr) {}
