/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to __builtin_IB_get_sr0 returns proper sr0 subregister.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK-LABEL: .kernel test
// CHECK: (W) mov (1|M0) {{.*}} sr0.0<0;1,0>:ud
// CHECK: (W) mov (1|M0) {{.*}} sr0.1<0;1,0>:ud
int __builtin_IB_get_sr0(int DWNumber);
__kernel void test(__global int *output)
{
    output[0] = __builtin_IB_get_sr0(0);
    output[1] = __builtin_IB_get_sr0(1);
}
