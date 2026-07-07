/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported
// RUN: ocloc compile -file %s -options "  -ze-opt-large-register-file -igc_opts 'EnableEfficient64b=1,VISAOptions=-asmToConsole'" -device cri | FileCheck %s --check-prefix=CHECK-ASM

// CHECK-ASM: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
__kernel void foo() { }
