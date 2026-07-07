/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s --check-prefixes=GRFBUMP2
// RUN: ocloc compile -file %s -device dg2 -options "-igc_opts 'DumpVISAASMToConsole=1,VISAGRFBumpUpNumber=3'" | FileCheck %s --check-prefixes=GRFBUMP3

// GRFBUMP2: -GRFBumpUpNumber 2
// GRFBUMP3: -GRFBumpUpNumber 3

__kernel void foo(int a, int b, __global int *res) {
    *res = a + b;
}
