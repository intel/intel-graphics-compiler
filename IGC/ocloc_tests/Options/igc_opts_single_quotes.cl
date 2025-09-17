// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1" -device dg2 | FileCheck %s

// XFAIL: *
// CHECK: Missing single quotes for -igc_opts
__kernel void foo(int a, int b, __global int *res) { *res = a + b; }
