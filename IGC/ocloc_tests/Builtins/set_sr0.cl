/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to __builtin_IB_set_sr0 creates a mov to a proper subregister.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK-LABEL: .kernel test
// CHECK: (W) mov (1|M0) sr0.1<1>:ud 0x9ABCDEF0:ud
// CHECK: (W) mov (1|M0) sr0.2<1>:ud 0x12345678:ud
void __builtin_IB_set_sr0(uint SubReg, uint Value);
__kernel void test()
{
    __builtin_IB_set_sr0(1, 0x9ABCDEF0);
    __builtin_IB_set_sr0(2, 0x12345678);
}
