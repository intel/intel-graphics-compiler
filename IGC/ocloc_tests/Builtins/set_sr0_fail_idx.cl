/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if we return compilation error, when the subregister passed
// as an argument to __builtin_IB_set_sr0 is not in the valid range.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK-LABEL: .kernel wrongkernel
// CHECK: error: GenISA_setSR0 intrinsic expects operand 0 value in the range [1-3]sss
// XFAIL: *
int __builtin_IB_set_sr0(uint SubReg, uint Value);
__kernel void wrongkernel(__global int *output)
{
    output[0] = __builtin_IB_set_sr0(0, 0x12345678);
}

