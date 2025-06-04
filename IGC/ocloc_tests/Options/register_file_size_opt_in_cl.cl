/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if -ze-exp-register-file-size flag is respected for OpenCL-C kernels

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options " -ze-exp-register-file-size 64 -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK64

// RUN: ocloc compile -file %s -device pvc -options " -ze-exp-register-file-size 128 -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK128

// CHECK64: //Build option: {{.*}} -TotalGRFNum 64
// CHECK128: //Build option: {{.*}} -TotalGRFNum 128

__kernel void foo(int a, int b, __global int *res)
{
    *res = a + b;
}
