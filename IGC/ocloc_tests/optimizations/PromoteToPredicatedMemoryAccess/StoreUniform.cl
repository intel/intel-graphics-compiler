/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test verifies uniform predicated store

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --implicit-check-not goto

// CHECK: (P{{[0-9\.]+}}.any) lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  {{.*}}:d32t

__kernel void uniform_store(__global float* out, const int predicate) {
    if (predicate)
        out[0] = 3;
}
