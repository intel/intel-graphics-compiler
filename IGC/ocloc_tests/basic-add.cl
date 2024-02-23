// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1'" -device dg2 | FileCheck %s --check-prefix=CHECK-VISA
// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-ASM
// RUN: ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1 PrintBefore=EmitPass'" -device dg2 2>&1 | FileCheck %s --check-prefix=CHECK-LLVM

// CHECK-VISA: add (M1_NM, 1) add_(0,0)<1> a(0,0)<0;1,0> b(0,0)<0;1,0>
// CHECK-ASM: (W) add (1|M0) r{{[0-9]*}}.{{[0-9]*}}<1>:d r{{[0-9]*}}.{{[0-9]*}}<0;1,0>:d r{{[0-9]*}}.{{[0-9]*}}<0;1,0>:d
// CHECK-LLVM: %add = add nsw i32 %a, %b
__kernel void foo(int a, int b, __global int *res) { *res = a + b; }
