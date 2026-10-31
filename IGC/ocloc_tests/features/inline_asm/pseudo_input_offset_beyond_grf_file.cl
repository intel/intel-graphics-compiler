/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Pseudo inputs describe compiler-generated memory ranges, not payload
// registers. Their offsets may exceed the physical GRF file, so the finalizer
// must not bind them to a physical GRF. With 256 GRFs of 64 bytes, offset
// 32704 would otherwise map to GRF 511 and corrupt the declaration.

// REQUIRES: regkeys, cri-supported
// RUN: ocloc compile -file %s -device cri -options "-cl-intel-256-GRF-per-thread -igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

// CHECK: .decl PSEUDO_OUT_OF_RANGE v_type=G type=ud num_elts=1
// CHECK: .implicit_PSEUDO_INPUT PSEUDO_OUT_OF_RANGE offset=32704 size=4
// CHECK: mov (M1_NM, 1) PSEUDO_OUT_OF_RANGE(0,0)<1> 0x2a:ud
// CHECK: mov (M1_NM, 1) {{.*}}(0,0)<1> PSEUDO_OUT_OF_RANGE(0,0)<0;1,0>

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
