/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'SubroutineThreshold=1,KernelTotalSizeThreshold=1,ControlInlineTinySize=1,PrintControlKernelTotalSize=15'" 2>&1 | FileCheck %s --check-prefix=CHECK-DEFAULT
// RUN: ocloc compile -file %s -device dg2 -options "-ze-opt-static-profile-guided-trimming -igc_opts 'SubroutineThreshold=1,KernelTotalSizeThreshold=1,ControlInlineTinySize=1,ControlInlineTinySizeForSPGT=150,PrintControlKernelTotalSize=15'" 2>&1 | FileCheck %s --check-prefix=CHECK-SPGT

// CHECK-DEFAULT: Good to trim (Big enough > 1), bar3, Function Attribute: Best effort innline, Function size: 13, Freq: 0.0
// CHECK-SPGT: Can't trim (Low weight < 0.03218650818), bar3, Function Attribute: Best effort innline, Function size: 163, Size after collapsing: 163, Size contribution: 652, Freq: 327680.0, Weight: 0.002581326365

int bar3(__global int *c) {
    int k = 10;
    for (int i = 0 ; i < 100 ; i++) {
        *c += k * i;
    }
    return k;
}

int bar2(__global int *b) {
    int k = 10;
    for (int i = 0 ; i < 100 ; i++) {
        *b += k * bar3(b);
    }
    return k;
}

int bar1(__global int *a) {
    int k = 10;
    for (int i = 0 ; i < 100 ; i++) {
        *a += k * bar2(a);
        *a += k * bar3(a);
    }
    return k;
}

__kernel void foo(int __global *p) {
    int a = 0;
    for (int i = 0; i < 100; i++) {
        a += bar1(p);
        a += bar2(p);
        a += bar3(p);
    }
    for (int i = 300; i < 500000; i++) {
        a += *p;
    }
    for (int i = 300; i < 500000; i++) {
        a += *p;
    }
    for (int i = 300; i < 500000; i++) {
        a += *p;
    }
    for (int i = 300; i < 500000; i++) {
        a += *p;
    }
    for (int i = 300; i < 500000; i++) {
        a += *p;
    }
    *p = a;
}