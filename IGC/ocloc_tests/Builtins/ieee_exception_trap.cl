/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if a call to __builtin_IB_enable_ieee_exception_trap enables
// bit 9 (IEEE Exception Trap Enable) in CR0.0

// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM
// RUN: ocloc compile -file %s -options " -igc_opts 'EnableIEEEFloatExceptionTrap=1 VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM-FLAG
// RUN: ocloc compile -file %s -options "-cl-intel-enable-ieee-float-exception-trap -igc_opts 'VISAOptions=-asmToConsole'" -device pvc | FileCheck %s --check-prefix=CHECK-ASM-OPTION

// CHECK-ASM-LABEL: .kernel test_builtin_enable
// CHECK-ASM: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x4C0:uw
// CHECK-ASM: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x200:uw
void __builtin_IB_enable_ieee_exception_trap();
kernel void test_builtin_enable() {
    __builtin_IB_enable_ieee_exception_trap();
}

// CHECK-ASM-LABEL: .kernel test_builtin_disable
// CHECK-ASM: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x4C0:uw
// CHECK-ASM: (W) and (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0xFFFFFDFF:ud
void __builtin_IB_disable_ieee_exception_trap();
kernel void test_builtin_disable() {
    __builtin_IB_disable_ieee_exception_trap();
}

// CHECK-ASM-LABEL: .kernel test_no_debug_flag
// CHECK-ASM: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x4C0:uw
kernel void test_no_debug_flag(global int* out) {
    out[0] = 0;
}

// CHECK-ASM-FLAG-LABEL: .kernel test_debug_flag
// CHECK-ASM-FLAG: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x6C0:uw
kernel void test_debug_flag(global int* out) {
    out[0] = 0;
}

// CHECK-ASM-OPTION-LABEL: .kernel test_option
// CHECK-ASM-OPTION: (W) or (1|M0) cr0.0<1>:ud   cr0.0<0;1,0>:ud  0x6C0:uw
kernel void test_option(global int* out) {
    out[0] = 0;
}
