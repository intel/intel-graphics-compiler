/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options " -igc_opts 'ForceOCLSIMDWidth=8'" 2>&1 | FileCheck %s

// CHECK: SIMD size of 8 has been forced when SIMD size of at least 16 is required on this platform
// XFAIL: *

__kernel void foo(int a, int b, __global int *res)
{
    *res = a + b;
}
