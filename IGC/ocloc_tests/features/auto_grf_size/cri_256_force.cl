/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Test that the regkey/option TotalGRFNum is correctly passed to finalizer
// to force a GRF number for the compilation.

// REQUIRES: regkeys, cri-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'TotalGRFNum=256,VISAOptions=-asmToConsole'" -device cri | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
__kernel void foo() {}
