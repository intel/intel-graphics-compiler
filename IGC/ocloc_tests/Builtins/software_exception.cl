/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to __builtin_IB_software_exception enables
// bit 29 (Software Exception Control) in CR0.1

// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device dg2 | FileCheck %s --check-prefix=CHECK-ASM
// CHECK-ASM: (W) or (1|M0) cr0.1<1>:ud   cr0.1<0;1,0>:ud   0x20000000:ud

void __builtin_IB_software_exception();
kernel void test() {
    __builtin_IB_software_exception();
}
