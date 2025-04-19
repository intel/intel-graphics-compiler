/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM: kernel store_split_vector
// CHECK-ASM:         cmp (32|M0)   (eq)[[F1:f[0-9\.]+]]   null<1>:d
// CHECK-ASM: ([[F1]])  store.ugm.d32x4.a64 (32|M0)
// CHECK-ASM: ([[F1]])  store.ugm.d32x4.a64 (32|M0)
// CHECK-ASM: ([[F1]])  store.ugm.d32x4.a64 (32|M0)
// CHECK-ASM: ([[F1]])  store.ugm.d32x4.a64 (32|M0)

// Test verifies predicated stores with splitting large vectors
__kernel void store_split_vector(__global int16* out, const int predicate) {
    int gid = get_global_id(0);
    int16 val = (int16){gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid, gid};
    if (predicate == gid)
        out[gid] = val;
}

// CHECK-ASM: kernel store_process_vector
// CHECK-ASM:         cmp (32|M0)   (le)[[F2:f[0-9\.]+]]   null<1>:d
// CHECK-ASM: ([[F2]])  store.ugm.d32x2.a64 (32|M0)

// Test verifies that predicated load was updated like <8xi8> -> <2 x i32>
__kernel void store_process_vector(__global char8* out, const int predicate) {
    int gid = get_global_id(0);
    char8 val = (char8){gid + 1, gid + 2, gid + 3, gid + 4, gid + 5, gid + 6, gid + 7, gid + 8};
    if (gid <= predicate)
        out[gid] = val;
}
