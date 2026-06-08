/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, bmg-supported, llvm-17-plus
// UNSUPPORTED: system-windows

// Environment variables
// RUN: not env IGC_LibClangOverride=libdoes-not-exist.so ocloc compile -file %s -device bmg 2>&1 | FileCheck %s --check-prefix=CHECK-MISSING-ENV
// RUN: env IGC_LibClangOverride=libopencl-clang2.so ocloc compile -file %s -device bmg 2>&1 | FileCheck %s --check-prefix=CHECK-PASS

// CHECK-MISSING-ENV: Error: LibClangOverride library 'libdoes-not-exist.so' failed to load.
// CHECK-PASS: Build succeeded

kernel void test(global int *out) { out[get_global_id(0)] = 1; }
