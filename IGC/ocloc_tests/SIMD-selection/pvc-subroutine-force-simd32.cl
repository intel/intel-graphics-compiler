/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// Verify that ForceOCLSIMDWidth=32 is respected on PVC for a kernel that
// calls a subroutine (FunctionControl=2).

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'ForceOCLSIMDWidth=32,FunctionControl=2,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

// CHECK: .kernel_attr SimdSize=32

int compute(__global int *src, int id)
{
    return src[id] + 1;
}

__kernel void test(__global int *dst, __global int *src)
{
    int id = get_global_id(0);
    dst[id] = compute(src, id);
}
