/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: not ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc 2>&1 | FileCheck %s

// CHECK: error{{.*}}Incorrect declaration of a __builtin_IB_{{.*}} function

int __builtin_IB_set_sr0(uint SubReg, uint Value);
__kernel void wrongkernel(int idx, __global int *output) { output[0] = __builtin_IB_set_sr0(idx, 0x12345678); }
