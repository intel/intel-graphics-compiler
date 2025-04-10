/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to __builtin_IB_enable_ieee_exception_trap enables
// bit 9 (IEEE Exception Trap Enable) in CR0.0

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM
// CHECK-ASM: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud   0x200:uw

void __builtin_IB_enable_ieee_exception_trap();
kernel void test() {
    __builtin_IB_enable_ieee_exception_trap();
}
