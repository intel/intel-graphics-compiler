/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-Xfinalizer -enableBarrierWA -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: (W)     load.ugm.d32x1t.a64 (1|M0)  r{{[0-9:]+}}:1        [[ADDR:\[r[0-9]+:[0-9]+\]]]
// CHECK-ASM: sync.allrd
// CHECK-ASM: (W)     store.ugm.d32x1t.a64 (1|M0)  [[ADDR]]     r{{[0-9:]+}}:1
// CHECK-ASM-NOT: fence.ugm.evict.tile
// CHECK-ASM: EOT

__kernel void foo(int a, int b, __global int *res)
{
    *res = a + b;
}
