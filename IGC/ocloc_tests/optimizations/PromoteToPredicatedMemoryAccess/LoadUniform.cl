/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test verifies uniform predicated load

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM: kernel uniform_load
// CHECK-ASM-DAG: (W)       cmp (32|M0)             (ne)[[F1:f[0-9\.]+]]          null<1>:d r{{[\.;,0-9<>:a-z]+}} 0:w
// CHECK-ASM-DAG: (W)       mov (1|M0)                  [[L1:r[0-9]+]].0<1>:f     0x0:f
// CHECK-ASM:     (W&[[F1]]) load.ugm.d32x1t.a64 (1|M0)  [[L1]]:1                  [{{[a-z0-9:]+}}]

// SIMT1 transposed load - copy merge value before load
__kernel void uniform_load(__global const float* in, __global float* out, const int predicate) {
    int gid = get_global_id(0);
    float val = 0;
    if (predicate)
        val = in[0];
    out[gid] = val;
}

// CHECK-ASM: kernel uniform_load_pred_mov
// CHECK-ASM: (W)     cmp (32|M0)   (ne)[[F2:f[0-9\.]+]]   null<1>:d
// CHECK-ASM: (W&[[F2]]) load.ugm.d32x1t.a64 (1|M0)  [[L2:r[0-9]+]]:1
// CHECK-ASM: (W&[[F2]]) mov (1|M0)              r{{[\.;,0-9<>:a-z]+}}     [[L2]].0<0;1,0>:f

// SIMT1 transposed load - copy merge value with predicate after load
__kernel void uniform_load_pred_mov(__global const float* in, __global float* out, const int predicate) {
    int gid = get_global_id(0);
    float val = predicate - 10;
    if (predicate)
        val = in[0];
    out[gid] = val;
}
