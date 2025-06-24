/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if we return compilation error, when the subregister passed
// as an argument to __builtin_IB_set_sr0 is not a compile-time constant.

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK-LABEL: .kernel wrongkernel
// CHECK: error: Expected constant operand for GenISA_setSR0 intrinsic.
// XFAIL: *
int __builtin_IB_set_sr0(uint SubReg, uint Value);
__kernel void wrongkernel(int idx,
                          __global int *output)
{
    output[0] = __builtin_IB_set_sr0(idx, 0x12345678);
}
