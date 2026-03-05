/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported
// RUN: ocloc compile -file %s -options "  -ze-opt-256-GRF-per-thread -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
__kernel void foo() {

}
