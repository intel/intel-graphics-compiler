/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported, llvm-22-plus

// RUN: ocloc compile -file %s -device dg2 2>&1 | FileCheck %s --check-prefix=WARN
// RUN: not ocloc compile -file %s -device dg2 -options "-igc_opts 'EnableIncompatiblePointerTypesError=1'" 2>&1 | FileCheck %s --check-prefix=ERROR

// WARN: warning: incompatible pointer types
// WARN: Build succeeded

// ERROR: error: incompatible pointer types
// ERROR: Build failed

kernel void test(global int *dst, global float *src) { dst = src; }
