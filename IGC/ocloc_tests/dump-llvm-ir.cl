// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'PrintToConsole=1,PrintBefore=BuiltinsConverterFunction'" -device dg2 2>&1 | FileCheck %s

// Check that correct metadata is dumped, e.g. FuncMD isn't empty.

// CHECK: = !{!"FuncMD", !{{.*}}, !{{.*}}}

kernel void foo() {}
