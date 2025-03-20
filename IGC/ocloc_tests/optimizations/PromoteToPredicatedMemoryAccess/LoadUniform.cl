/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test verifies uniform predicated load

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM-DAG: (W)       cmp (32|M0)             (ne)[[F:f[0-9\.]+]]          null<1>:d r{{[\.;,0-9<>:a-z]+}} 0:w
// CHECK-ASM-DAG: (W)       mov (1|M0)                  [[L:r[0-9]+]].0<1>:f     0x0:f
// CHECK-ASM:     (W&[[F]]) load.ugm.d32x1t.a64 (1|M0)  [[L]]:1                  [{{[a-z0-9:]+}}]

__kernel void uniform_load(__global const float* in, __global float* out, const int predicate) {
    int gid = get_global_id(0);
    float val = 0;
    if (predicate)
        val = in[0];
    out[gid] = val;
}
