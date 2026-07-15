/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// A loop whose body is atomicraw operations must be runtime-unrolled. This
// requires the atomicraw intrinsics to be WillReturn so ScalarEvolution can
// compute the trip count; without it the loop stays rolled and none of the
// unroll-remainder blocks below are emitted.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'PrintToConsole=1, PrintAfter=EmitPass'" 2>&1 | FileCheck %s

// CHECK: define spir_kernel void @main_g1n
// Main loop unrolled by 4: 4 * 32 = 128 atomics, then the loop back-edge.
// CHECK-COUNT-128: call i32 @llvm.genx.GenISA.intatomicraw
// CHECK: br i1
// Epilogue remainder loop: one source iteration = 32 atomics, then its back-edge.
// CHECK-COUNT-32: call i32 @llvm.genx.GenISA.intatomicraw
// CHECK: br i1

// clang-format off
__kernel void main_g1n(__global int *inp, __global int *out,
                        __local int *loc, int it, int idx, int strd)
{
    __local volatile int *tmp = loc;
    int p;
    int i = (get_local_id(0) * strd);
    int il = (i % 8192);

    p = inp[i];
    loc[il] = inp[i + idx];

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    for (int n = 0; n < it; n += 32)
    {
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
        atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]); atomic_inc(&tmp[i]);
    }

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    out[i] = p + loc[il];
}
// clang-format on
