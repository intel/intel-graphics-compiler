/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This tests checks for xe3p we drop to SIMD16 instead increasing to LargeGRF.
// UNSUPPORTED: system-windows
// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t, AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=90'"
// RUN: ls -al %t | FileCheck %s

// CHECK: OCL_asm{{.*}}_simd16_entry_0001.asm
// CHECK-NOT: OCL_asm{{.*}}_simd32_entry_0001.asm

#define def(N) float4 float_var_##N = {1+tid, 2+tid, 3+tid, 4+tid};
#define incf4(N) float_var_##N += (float4){4, 3, 2, 1};
#define wrt(N) result[N] = float_var_##N;

__kernel void foo(float4 __global *result) {
    int  tid = get_global_id(0);

    def(1); def(2); def(3); def(4); def(5); def(6); def(7); def(8);
    def(11); def(12); def(13); def(14); def(15); def(16); def(17); def(18);
    def(21); def(22); def(23); def(24); def(25); def(26); def(27); def(28);

    #pragma nounroll
    for (int i = 0; i < 1000; i++)
    {
        incf4(1); incf4(2); incf4(3); incf4(4); incf4(5); incf4(6); incf4(7); incf4(8);
        incf4(11); incf4(12); incf4(13); incf4(14); incf4(15); incf4(16); incf4(17); incf4(18);
        incf4(21); incf4(22); incf4(23); incf4(24); incf4(25); incf4(26); incf4(27); incf4(28);
    }

    wrt(1); wrt(2); wrt(3); wrt(4); wrt(5); wrt(6); wrt(7); wrt(8);
    wrt(11); wrt(12); wrt(13); wrt(14); wrt(15); wrt(16); wrt(17); wrt(18);
    wrt(21); wrt(22); wrt(23); wrt(24); wrt(25); wrt(26); wrt(27); wrt(28);
}
