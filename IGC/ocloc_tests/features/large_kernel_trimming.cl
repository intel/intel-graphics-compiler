/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported

// Test checks that in case, kernel is above the large kernel kernel threshold small functions will be trimmed as well.
// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'SubroutineThreshold=1,KernelTotalSizeThreshold=1,ControlInlineTinySize=15,PrintControlKernelTotalSize=15'" 2>&1 | FileCheck %s

// CHECK: is much larger than threshold, trimming small functions as well.
// CHECK: Trim the function, bar2
// CHECK: Trim the function, bar3

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