/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options "-igc_opts 'DumpToCurrentDir=1 DumpTimeStatsPerPass=1'" -device pvc
// RUN: FileCheck %s --input-file=TimeStatPerPass_Shaders.csv --check-prefixes=CHECK
// CHECK: Frequency

// This test checks that DumpTimeStatsPerPass option works correctly.
__kernel void foo(int a, int b, __global int *res) { *res = a + b; }