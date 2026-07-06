/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Tests verify predicated store for subDW types

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM:     (f{{[0-9\.]+}})  store.ugm.d8u32.a64 (32|M0)  [r{{[0-9:]+}}]    r{{[0-9:]+}}

__kernel void basic(__global const char* in, __global char* out, const int predicate) {
    int gid = get_global_id(0);
    char val = in[gid];
    if (gid < predicate)
        out[gid] = val;
}
