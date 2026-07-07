/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that the register spill warning includes the kernel name, so users
// can tell which kernel triggered the spilling when compiling multiple kernels
// for multiple architectures.
//
// TotalGRFNum=32 forces an artificially low register file, guaranteeing spills.
// ForceSIMDRPELimit=2000 prevents the compiler from avoiding spills by
// switching to a smaller SIMD width.
// The spill warning format is: "compiled SIMD<N> allocated <X> regs and spilled around <Y>"

// REQUIRES: pvc-supported, regkeys
// RUN: ocloc compile -file %s -options "-igc_opts 'TotalGRFNum=32, ForceSIMDRPELimit=2000'" -device pvc 2>&1 | FileCheck %s

// CHECK: warning: in kernel 'spill_kernel': compiled SIMD{{.*}} allocated {{.*}} regs and spilled around {{.*}}

#define def(N) float16 v##N = {1+i, 2+i, 3+i, 4+i, 5+i, 6+i, 7+i, 8+i, 9+i, 10+i, 11+i, 12+i, 13+i, 14+i, 15+i, 16+i};
#define inc(N) v##N += (float16){16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
#define wrt(N) result[N] = v##N;

kernel void spill_kernel(float16 global *result) {
    int i = get_global_id(0);

    def(0); def(1); def(2); def(3); def(4); def(5); def(6); def(7);

    #pragma nounroll
    for (int j = 0; j < 1500; j++) {
        inc(0); inc(1); inc(2); inc(3); inc(4); inc(5); inc(6); inc(7);
    }

    wrt(0); wrt(1); wrt(2); wrt(3); wrt(4); wrt(5); wrt(6); wrt(7);
}
