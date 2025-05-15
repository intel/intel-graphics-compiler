/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test verifies uniform predicated store

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM:     (W&f{{[0-9\.]+}}) store.ugm.d32x1t.a64 (1|M0)  [r{{[0-9:]+}}]    r{{[0-9:]+}}

__kernel void uniform_store(__global float* out, const int predicate) {
    if (predicate)
        out[0] = 3;
}
