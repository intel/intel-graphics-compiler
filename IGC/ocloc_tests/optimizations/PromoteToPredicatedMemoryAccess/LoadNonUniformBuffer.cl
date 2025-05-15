/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test verifies predicated load for non-uniform buffer

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s  --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM-DAG:         cmp (32|M0)   (lt)[[F:f[0-9\.]+]]   null<1>:d     r{{[\.;,0-9<>:a-z]+}}
// CHECK-ASM-DAG:         mov (32|M0)              [[L:r[0-9]+]].0<1>:f    0x0:f
// CHECK-ASM:    ([[F]])  load.ugm.d32.a64 (32|M0)  [[L]]:2         [{{[a-z0-9:]+}}]
// CHECK-ASM:             store.ugm.d32.a64 (32|M0)  [{{[a-z0-9:]+}}]      [[L]]:2

__kernel void test(__global const float* buffer1,
                   __global const float* buffer2,
                   __global float* outputBuffer,
                   const int predicate) {
    __global const float* buffers[2] = {buffer1, buffer2};
    int gid = get_global_id(0);
    int bufferIndex = gid % 2;
    __global const float* buffer = buffers[bufferIndex];

    float val = 0;
    if (gid < predicate)
        val = buffer[gid];
    outputBuffer[gid] = val;
}
