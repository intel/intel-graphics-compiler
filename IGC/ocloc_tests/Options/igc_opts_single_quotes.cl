// REQUIRES: regkeys, dg2-supported

// RUN: %if lib-igc-clang %{ not %} ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1" -device dg2 | FileCheck %s

// CHECK: {{Missing single quotes for -igc_opts|Failed to tokenize command line options}}
__kernel void foo(int a, int b, __global int *res) { *res = a + b; }
