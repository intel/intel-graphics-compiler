/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// An input whose offset lies beyond the kernel's GRF file must be rejected
// with a diagnostic instead of being bound to a non-existent physical GRF
// (which used to corrupt the declaration and crash later in the finalizer).
// With 256 GRFs of 64 bytes, offset 32704 maps to r511.

// REQUIRES: regkeys, cri-supported
// RUN: not ocloc compile -file %s -device cri -options "-cl-intel-256-GRF-per-thread" 2>&1 | FileCheck %s
// RUN: ocloc compile -file %s -device cri 2>&1 | FileCheck %s --check-prefix=CHECK-512

// CHECK: parsing vISA inline assembly failed:
// CHECK: input PSEUDO_OUT_OF_RANGE at byte offset 32704 (r511, 1 GRF(s)) exceeds the 256-GRF register file of this kernel
// CHECK: failed to create input variable

// The same input is valid when the kernel has 512 GRFs.
// CHECK-512-NOT: parsing vISA inline assembly failed
// CHECK-512-NOT: exceeds the {{[0-9]+}}-GRF register file

kernel void pseudo_input_out_of_range(global uint *out) {
    uint value;
    __asm__ volatile(
        ".decl PSEUDO_OUT_OF_RANGE v_type=G type=ud num_elts=1\n"
        ".implicit_PSEUDO_INPUT PSEUDO_OUT_OF_RANGE offset=32704 size=4\n"
        "mov (M1_NM, 1) PSEUDO_OUT_OF_RANGE(0,0)<1> 42:ud\n"
        "mov (M1_NM, 1) %0(0,0)<1> PSEUDO_OUT_OF_RANGE(0,0)<0;1,0>\n"
        : "=rw"(value));
    out[0] = value;
}
