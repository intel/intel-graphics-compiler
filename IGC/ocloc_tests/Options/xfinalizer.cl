/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-Xfinalizer -enableBarrierWA -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: (W)     send.ugm (1|M0)          [[R:[A-z0-9]*]]       r0      null:0  0x0            0x0210141F
// CHECK-ASM: (W)     mov (8|M0)               null<1>:ud    [[R]].{{[0-9]*}}<1;1,0>:ud

__kernel void foo(int a, int b, __global int *res)
{
    *res = a + b;
}
